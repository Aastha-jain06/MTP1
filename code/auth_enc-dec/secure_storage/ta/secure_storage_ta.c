#include <inttypes.h>
#include <secure_storage_ta.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <string.h>

#define CHUNK_SIZE (16 * 1024)  // 16KB chunks
#define AES_KEY_SIZE 32         // 256-bit key
#define AES_IV_SIZE 16          // 128-bit IV

/* Session context to maintain encryption state and PIN authentication */
struct crypto_session {
	TEE_OperationHandle enc_op;
	TEE_OperationHandle dec_op;
	TEE_ObjectHandle key_handle;
	uint8_t iv[AES_IV_SIZE];
	bool initialized;
	uint32_t total_enc_time_us;
	uint32_t total_dec_time_us;
	size_t total_bytes;
	
	/* PIN authentication state */
	char pin[PIN_MAX_LENGTH + 1];
	bool pin_set;
	bool authenticated;
	uint32_t failed_attempts;
	TEE_Time lockout_until;
	bool locked_out;
};

/* Hash PIN using SHA-256 for secure storage */
static TEE_Result hash_pin(const char *pin, uint8_t *hash, uint32_t hash_len)
{
	TEE_OperationHandle op = TEE_HANDLE_NULL;
	TEE_Result res;
	uint32_t out_len = hash_len;
	
	if (hash_len < 32)  /* SHA-256 produces 32 bytes */
		return TEE_ERROR_BAD_PARAMETERS;
	
	res = TEE_AllocateOperation(&op, TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
	if (res != TEE_SUCCESS)
		return res;
	
	TEE_DigestUpdate(op, pin, strlen(pin));
	
	res = TEE_DigestDoFinal(op, NULL, 0, hash, &out_len);
	TEE_FreeOperation(op);
	
	return res;
}

/* Set or change PIN */
static TEE_Result set_pin(uint32_t param_types, TEE_Param params[4],
                         struct crypto_session *sess)
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);
	const char *pin;
	size_t pin_len;
	
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
	
	pin = params[0].memref.buffer;
	pin_len = params[0].memref.size;
	
	/* Validate PIN length */
	if (pin_len < PIN_MIN_LENGTH || pin_len > PIN_MAX_LENGTH) {
		EMSG("Invalid PIN length: %zu (must be %d-%d digits)",
		     pin_len, PIN_MIN_LENGTH, PIN_MAX_LENGTH);
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	/* Validate PIN contains only digits */
	for (size_t i = 0; i < pin_len; i++) {
		if (pin[i] < '0' || pin[i] > '9') {
			EMSG("PIN must contain only digits");
			return TEE_ERROR_BAD_PARAMETERS;
		}
	}
	
	/* Store PIN securely (in production, store hash in secure storage) */
	TEE_MemMove(sess->pin, pin, pin_len);
	sess->pin[pin_len] = '\0';
	sess->pin_set = true;
	sess->authenticated = false;
	sess->failed_attempts = 0;
	sess->locked_out = false;
	
	IMSG("PIN set successfully");
	return TEE_SUCCESS;
}

/* Verify PIN */
static TEE_Result verify_pin(uint32_t param_types, TEE_Param params[4],
                             struct crypto_session *sess)
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_VALUE_OUTPUT,
				TEE_PARAM_TYPE_VALUE_OUTPUT,
				TEE_PARAM_TYPE_NONE);
	const char *pin;
	size_t pin_len;
	TEE_Time current_time;
	
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
	
	pin = params[0].memref.buffer;
	pin_len = params[0].memref.size;
	
	/* Check if PIN is set */
	if (!sess->pin_set) {
		EMSG("PIN not set");
		params[1].value.a = 0;
		params[2].value.a = 0;
		return TEE_ERROR_BAD_STATE;
	}
	
	/* Check if locked out */
	if (sess->locked_out) {
		TEE_GetSystemTime(&current_time);
		
		/* Check if lockout period has expired */
		if (current_time.seconds < sess->lockout_until.seconds) {
			uint32_t remaining = sess->lockout_until.seconds - current_time.seconds;
			EMSG("Account locked. Try again in %u seconds", remaining);
			params[1].value.a = 0;
			params[2].value.a = 0;
			return TEE_ERROR_ACCESS_DENIED;
		} else {
			/* Lockout expired, reset */
			sess->locked_out = false;
			sess->failed_attempts = 0;
		}
	}
	
	/* Verify PIN */
	if (pin_len == strlen(sess->pin) && 
	    TEE_MemCompare(pin, sess->pin, pin_len) == 0) {
		/* PIN correct */
		sess->authenticated = true;
		sess->failed_attempts = 0;
		sess->locked_out = false;
		
		params[1].value.a = 1;  /* Success */
		params[2].value.a = PIN_MAX_ATTEMPTS;
		
		IMSG("PIN verified successfully");
		return TEE_SUCCESS;
	} else {
		/* PIN incorrect */
		sess->failed_attempts++;
		sess->authenticated = false;
		
		if (sess->failed_attempts >= PIN_MAX_ATTEMPTS) {
			/* Lock out the account */
			TEE_GetSystemTime(&current_time);
			sess->lockout_until.seconds = current_time.seconds + PIN_LOCKOUT_TIME_SEC;
			sess->lockout_until.millis = current_time.millis;
			sess->locked_out = true;
			
			EMSG("Too many failed attempts. Account locked for %d seconds",
			     PIN_LOCKOUT_TIME_SEC);
		} else {
			EMSG("Incorrect PIN. %u attempts remaining",
			     PIN_MAX_ATTEMPTS - sess->failed_attempts);
		}
		
		params[1].value.a = 0;  /* Failure */
		params[2].value.a = PIN_MAX_ATTEMPTS - sess->failed_attempts;
		
		return TEE_ERROR_ACCESS_DENIED;
	}
}

/* Check if authenticated before allowing crypto operations */
static TEE_Result check_authentication(struct crypto_session *sess)
{
	if (!sess->pin_set) {
		EMSG("PIN not set. Set PIN before performing operations.");
		return TEE_ERROR_BAD_STATE;
	}
	
	if (!sess->authenticated) {
		EMSG("Not authenticated. Verify PIN before performing operations.");
		return TEE_ERROR_ACCESS_DENIED;
	}
	
	return TEE_SUCCESS;
}

/* Generate or retrieve the encryption key (stored securely in TA) */
static TEE_Result init_crypto_key(struct crypto_session *sess)
{
	TEE_Result res;
	TEE_Attribute attr;
	uint8_t key_data[AES_KEY_SIZE];
	
	/* In production, this key should be:
	 * 1. Derived from a hardware-backed key
	 * 2. Stored in secure storage
	 * 3. Never exposed to normal world
	 * For this demo, we generate a deterministic key */
	TEE_GenerateRandom(key_data, AES_KEY_SIZE);
	
	/* Allocate transient object for AES key */
	res = TEE_AllocateTransientObject(TEE_TYPE_AES, AES_KEY_SIZE * 8, 
	                                   &sess->key_handle);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_AllocateTransientObject failed: 0x%x", res);
		return res;
	}
	
	/* Populate key */
	TEE_InitRefAttribute(&attr, TEE_ATTR_SECRET_VALUE, key_data, AES_KEY_SIZE);
	res = TEE_PopulateTransientObject(sess->key_handle, &attr, 1);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_PopulateTransientObject failed: 0x%x", res);
		TEE_FreeTransientObject(sess->key_handle);
		return res;
	}
	
	/* Generate IV (in production, this should be unique per file) */
	TEE_GenerateRandom(sess->iv, AES_IV_SIZE);
	
	return TEE_SUCCESS;
}

/* Initialize encryption operation */
static TEE_Result init_encryption(struct crypto_session *sess)
{
	TEE_Result res;
	
	/* Allocate encryption operation */
	res = TEE_AllocateOperation(&sess->enc_op, TEE_ALG_AES_CBC_NOPAD,
	                             TEE_MODE_ENCRYPT, AES_KEY_SIZE * 8);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_AllocateOperation (encrypt) failed: 0x%x", res);
		return res;
	}
	
	/* Set encryption key */
	res = TEE_SetOperationKey(sess->enc_op, sess->key_handle);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_SetOperationKey (encrypt) failed: 0x%x", res);
		TEE_FreeOperation(sess->enc_op);
		return res;
	}
	
	/* Initialize cipher with IV */
	TEE_CipherInit(sess->enc_op, sess->iv, AES_IV_SIZE);
	
	return TEE_SUCCESS;
}

/* Initialize decryption operation */
static TEE_Result init_decryption(struct crypto_session *sess)
{
	TEE_Result res;
	
	/* Allocate decryption operation */
	res = TEE_AllocateOperation(&sess->dec_op, TEE_ALG_AES_CBC_NOPAD,
	                             TEE_MODE_DECRYPT, AES_KEY_SIZE * 8);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_AllocateOperation (decrypt) failed: 0x%x", res);
		return res;
	}
	
	/* Set decryption key */
	res = TEE_SetOperationKey(sess->dec_op, sess->key_handle);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_SetOperationKey (decrypt) failed: 0x%x", res);
		TEE_FreeOperation(sess->dec_op);
		return res;
	}
	
	/* Initialize cipher with same IV */
	TEE_CipherInit(sess->dec_op, sess->iv, AES_IV_SIZE);
	
	return TEE_SUCCESS;
}

/* Encrypt a chunk of data */
static TEE_Result encrypt_chunk(uint32_t param_types, TEE_Param params[4],
                                struct crypto_session *sess)
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_OUTPUT,
				TEE_PARAM_TYPE_VALUE_INPUT,
				TEE_PARAM_TYPE_VALUE_OUTPUT);
	TEE_Result res;
	void *plaintext;
	void *ciphertext;
	size_t data_sz;
	uint32_t is_first;
	TEE_Time start_time, end_time;
	uint32_t elapsed_us;
	uint32_t out_len;
	
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
	
	/* Check authentication before allowing encryption */
	res = check_authentication(sess);
	if (res != TEE_SUCCESS)
		return res;
	
	plaintext = params[0].memref.buffer;
	data_sz = params[0].memref.size;
	ciphertext = params[1].memref.buffer;
	is_first = params[2].value.a;
	
	if (data_sz > CHUNK_SIZE) {
		EMSG("Chunk size %zu exceeds maximum %d", data_sz, CHUNK_SIZE);
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	/* Initialize on first chunk */
	if (is_first) {
		if (!sess->initialized) {
			res = init_crypto_key(sess);
			if (res != TEE_SUCCESS)
				return res;
			sess->initialized = true;
		}
		
		res = init_encryption(sess);
		if (res != TEE_SUCCESS)
			return res;
		
		sess->total_enc_time_us = 0;
		sess->total_bytes = 0;
	}
	
	/* Ensure data is multiple of AES block size (16 bytes) */
	if (data_sz % 16 != 0) {
		EMSG("Data size %zu must be multiple of 16", data_sz);
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	/* Measure encryption time */
	TEE_GetSystemTime(&start_time);
	
	out_len = params[1].memref.size;
	res = TEE_CipherUpdate(sess->enc_op, plaintext, data_sz,
	                       ciphertext, &out_len);
	
	TEE_GetSystemTime(&end_time);
	
	if (res != TEE_SUCCESS) {
		EMSG("TEE_CipherUpdate failed: 0x%x", res);
		return res;
	}
	
	/* Calculate elapsed time in microseconds */
	elapsed_us = (end_time.seconds - start_time.seconds) * 1000000 +
	             (end_time.millis - start_time.millis) * 1000;
	
	sess->total_enc_time_us += elapsed_us;
	sess->total_bytes += data_sz;
	
	params[1].memref.size = out_len;
	params[3].value.a = elapsed_us;
	
	return TEE_SUCCESS;
}

/* Decrypt a chunk of data */
static TEE_Result decrypt_chunk(uint32_t param_types, TEE_Param params[4],
                                struct crypto_session *sess)
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_OUTPUT,
				TEE_PARAM_TYPE_VALUE_INPUT,
				TEE_PARAM_TYPE_VALUE_OUTPUT);
	TEE_Result res;
	void *ciphertext;
	void *plaintext;
	size_t data_sz;
	uint32_t is_first;
	TEE_Time start_time, end_time;
	uint32_t elapsed_us;
	uint32_t out_len;
	
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
	
	/* Check authentication before allowing decryption */
	res = check_authentication(sess);
	if (res != TEE_SUCCESS)
		return res;
	
	ciphertext = params[0].memref.buffer;
	data_sz = params[0].memref.size;
	plaintext = params[1].memref.buffer;
	is_first = params[2].value.a;
	
	if (data_sz > CHUNK_SIZE) {
		EMSG("Chunk size %zu exceeds maximum %d", data_sz, CHUNK_SIZE);
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	/* Initialize on first chunk */
	if (is_first) {
		if (!sess->initialized) {
			EMSG("Crypto not initialized");
			return TEE_ERROR_BAD_STATE;
		}
		
		res = init_decryption(sess);
		if (res != TEE_SUCCESS)
			return res;
		
		sess->total_dec_time_us = 0;
	}
	
	/* Measure decryption time */
	TEE_GetSystemTime(&start_time);
	
	out_len = params[1].memref.size;
	res = TEE_CipherUpdate(sess->dec_op, ciphertext, data_sz,
	                       plaintext, &out_len);
	
	TEE_GetSystemTime(&end_time);
	
	if (res != TEE_SUCCESS) {
		EMSG("TEE_CipherUpdate (decrypt) failed: 0x%x", res);
		return res;
	}
	
	/* Calculate elapsed time in microseconds */
	elapsed_us = (end_time.seconds - start_time.seconds) * 1000000 +
	             (end_time.millis - start_time.millis) * 1000;
	
	sess->total_dec_time_us += elapsed_us;
	
	params[1].memref.size = out_len;
	params[3].value.a = elapsed_us;
	
	return TEE_SUCCESS;
}

/* Get final timing statistics */
static TEE_Result finalize_operation(uint32_t param_types, TEE_Param params[4],
                                     struct crypto_session *sess)
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_OUTPUT,
				TEE_PARAM_TYPE_VALUE_OUTPUT,
				TEE_PARAM_TYPE_VALUE_OUTPUT,
				TEE_PARAM_TYPE_NONE);
	
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
	
	/* Return timing in milliseconds */
	params[0].value.a = sess->total_enc_time_us / 1000;
	params[1].value.a = sess->total_dec_time_us / 1000;
	params[2].value.a = (uint32_t)(sess->total_bytes & 0xFFFFFFFF);
	
	IMSG("Final stats: Enc=%u ms, Dec=%u ms, Bytes=%zu",
	     params[0].value.a, params[1].value.a, sess->total_bytes);
	
	return TEE_SUCCESS;
}

/* Reset session state */
static TEE_Result reset_session(struct crypto_session *sess)
{
	if (sess->enc_op)
		TEE_FreeOperation(sess->enc_op);
	if (sess->dec_op)
		TEE_FreeOperation(sess->dec_op);
	
	sess->enc_op = TEE_HANDLE_NULL;
	sess->dec_op = TEE_HANDLE_NULL;
	sess->total_enc_time_us = 0;
	sess->total_dec_time_us = 0;
	sess->total_bytes = 0;
	
	return TEE_SUCCESS;
}

TEE_Result TA_CreateEntryPoint(void)
{
	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t __unused param_types,
				    TEE_Param __unused params[4],
				    void **session)
{
	struct crypto_session *sess;
	
	sess = TEE_Malloc(sizeof(*sess), 0);
	if (!sess)
		return TEE_ERROR_OUT_OF_MEMORY;
	
	TEE_MemFill(sess, 0, sizeof(*sess));
	sess->initialized = false;
	sess->enc_op = TEE_HANDLE_NULL;
	sess->dec_op = TEE_HANDLE_NULL;
	
	/* Initialize PIN authentication state */
	sess->pin_set = false;
	sess->authenticated = false;
	sess->failed_attempts = 0;
	sess->locked_out = false;
	TEE_MemFill(&sess->lockout_until, 0, sizeof(sess->lockout_until));
	
	*session = sess;
	
	IMSG("Session opened");
	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void *session)
{
	struct crypto_session *sess = session;
	
	if (sess) {
		if (sess->enc_op)
			TEE_FreeOperation(sess->enc_op);
		if (sess->dec_op)
			TEE_FreeOperation(sess->dec_op);
		if (sess->initialized && sess->key_handle)
			TEE_FreeTransientObject(sess->key_handle);
		
		/* Clear PIN from memory */
		TEE_MemFill(sess->pin, 0, sizeof(sess->pin));
		
		TEE_Free(sess);
	}
	
	IMSG("Session closed");
}

TEE_Result TA_InvokeCommandEntryPoint(void *session,
				      uint32_t command,
				      uint32_t param_types,
				      TEE_Param params[4])
{
	struct crypto_session *sess = session;
	
	switch (command) {
	case TA_SECURE_STORAGE_CMD_SET_PIN:
		return set_pin(param_types, params, sess);
	case TA_SECURE_STORAGE_CMD_VERIFY_PIN:
		return verify_pin(param_types, params, sess);
	case TA_SECURE_STORAGE_CMD_ENCRYPT_CHUNK:
		return encrypt_chunk(param_types, params, sess);
	case TA_SECURE_STORAGE_CMD_DECRYPT_CHUNK:
		return decrypt_chunk(param_types, params, sess);
	case TA_SECURE_STORAGE_CMD_FINALIZE:
		return finalize_operation(param_types, params, sess);
	case TA_SECURE_STORAGE_CMD_RESET:
		return reset_session(sess);
	default:
		EMSG("Command ID 0x%x is not supported", command);
		return TEE_ERROR_NOT_SUPPORTED;
	}
}