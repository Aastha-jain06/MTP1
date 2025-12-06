#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* TA API: UUID and command IDs */
#include <secure_storage_ta.h>

#define CHUNK_SIZE (16 * 1024)  // 16KB - must match TA

/* TEE resources */
struct test_ctx {
	TEEC_Context ctx;
	TEEC_Session sess;
};

void prepare_tee_session(struct test_ctx *ctx)
{
	TEEC_UUID uuid = TA_SECURE_STORAGE_UUID;
	uint32_t origin;
	TEEC_Result res;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx->ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	/* Open a session with the TA */
	res = TEEC_OpenSession(&ctx->ctx, &ctx->sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_OpenSession failed with code 0x%x origin 0x%x",
			res, origin);
}

void terminate_tee_session(struct test_ctx *ctx)
{
	TEEC_CloseSession(&ctx->sess);
	TEEC_FinalizeContext(&ctx->ctx);
}

TEEC_Result delete_secure_object(struct test_ctx *ctx, char *id)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(id);

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE, TEEC_NONE, TEEC_NONE);

	op.params[0].tmpref.buffer = id;
	op.params[0].tmpref.size = id_len;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_SECURE_STORAGE_CMD_DELETE,
				 &op, &origin);

	switch (res) {
	case TEEC_SUCCESS:
	case TEEC_ERROR_ITEM_NOT_FOUND:
		break;
	default:
		printf("Command DELETE failed: 0x%x / %u\n", res, origin);
	}

	return res;
}


/**
 * Read from secure storage and verify size
 * Returns actual size read
 */
TEEC_Result read_and_verify_size(struct test_ctx *ctx, char *obj_id, 
                                  size_t expected_size)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(obj_id);
	char small_buffer[1];

	printf("  Verifying object size...\n");

	/* Query object size by requesting 1 byte */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE, TEEC_NONE);

	op.params[0].tmpref.buffer = obj_id;
	op.params[0].tmpref.size = id_len;

	op.params[1].tmpref.buffer = small_buffer;
	op.params[1].tmpref.size = 1;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_SECURE_STORAGE_CMD_READ_RAW,
				 &op, &origin);

	if (res == TEEC_ERROR_SHORT_BUFFER) {
		size_t actual_size = op.params[1].memref.size;
		printf("  ✓ Object size: %zu bytes (%.2f MB)\n", 
		       actual_size, actual_size / (1024.0 * 1024.0));
		
		if (actual_size == expected_size) {
			printf("  ✓ Size matches expected: %zu bytes\n", expected_size);
			return TEEC_SUCCESS;
		} else {
			printf("  ✗ Size mismatch! Expected: %zu, Got: %zu\n", 
			       expected_size, actual_size);
			return TEEC_ERROR_GENERIC;
		}
	} else if (res == TEEC_SUCCESS) {
		printf("  Object size: 1 byte or less\n");
		return TEEC_SUCCESS;
	} else {
		printf("  Error reading object: 0x%x / %u\n", res, origin);
		return res;
	}
}



int main(int argc, char *argv[])
{
	struct test_ctx ctx;
	char obj_id[] = "large_test_object";
	const char *test_file;
	struct stat st;
	TEEC_Result res;
	int use_generated_file = 0;

	printf("=======================================================\n");
	printf("  OP-TEE Secure Storage - Large File Test (Streaming)\n");
	printf("=======================================================\n\n");

	/* Check if file provided as argument */
	if (argc > 1) {
		test_file = argv[1];
		printf("Using provided file: %s\n", test_file);
		if (stat(test_file, &st) != 0) {
			printf("Error: File %s not found\n", test_file);
			return 1;
		}
	} else {
		/* Generate test file */
		test_file = "/tmp/secure_storage_test.bin";
		use_generated_file = 1;
		printf("No file provided, generating test file...\n");
		
		/* Start with 1MB, you can change this */
		if (generate_test_file(test_file, 1) != 0) {
			printf("Failed to generate test file\n");
			return 1;
		}
		
		if (stat(test_file, &st) != 0) {
			printf("Error: Cannot stat generated file\n");
			return 1;
		}
	}

	printf("\nTest file size: %zu bytes (%.2f MB)\n\n", 
	       st.st_size, st.st_size / (1024.0 * 1024.0));

	printf("Preparing TEE session...\n");
	prepare_tee_session(&ctx);
	printf("✓ Session established\n\n");

	/* Delete any existing object with same ID */
	printf("Cleaning up any existing object...\n");
	delete_secure_object(&ctx, obj_id);



	/*
	 * Test: Verify object exists and has correct size
	 */
	printf("\n=== TEST 2: Verify stored object ===\n");
	res = read_and_verify_size(&ctx, obj_id, st.st_size);
	if (res != TEEC_SUCCESS) {
		printf("✗ TEST 2 FAILED\n");
		goto cleanup;
	}
	printf("✓ TEST 2 PASSED\n");



cleanup:
	printf("\nCleaning up...\n");
	terminate_tee_session(&ctx);
	
	if (use_generated_file) {
		unlink(test_file);
		printf("✓ Temporary test file removed\n");
	}
	
	printf("✓ Session closed\n");
	
	return (res == TEEC_SUCCESS) ? 0 : 1;
}