#include <inttypes.h>
#include <secure_storage_ta.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#define CHUNK_SIZE (16 * 1024)  // 16KB chunks for shared memory safety

/* Session context to maintain state across calls */
struct write_session {
	TEE_ObjectHandle object;
	bool in_progress;
};

static TEE_Result delete_object(uint32_t param_types, TEE_Param params[4])
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);
	TEE_ObjectHandle object;
	TEE_Result res;
	char *obj_id;
	size_t obj_id_sz;

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	obj_id_sz = params[0].memref.size;
	obj_id = TEE_Malloc(obj_id_sz, 0);
	if (!obj_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);

	res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, obj_id_sz,
					TEE_DATA_FLAG_ACCESS_READ |
					TEE_DATA_FLAG_ACCESS_WRITE_META,
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to open persistent object, res=0x%08x", res);
		TEE_Free(obj_id);
		return res;
	}

	TEE_CloseAndDeletePersistentObject1(object);
	TEE_Free(obj_id);

	return res;
}

/* Original write function - kept for compatibility with small files */
static TEE_Result create_raw_object(uint32_t param_types, TEE_Param params[4])
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);
	TEE_ObjectHandle object;
	TEE_Result res;
	char *obj_id;
	size_t obj_id_sz;
	char *data;
	size_t data_sz;
	uint32_t obj_data_flag;

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	obj_id_sz = params[0].memref.size;
	obj_id = TEE_Malloc(obj_id_sz, 0);
	if (!obj_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);

	data_sz = params[1].memref.size;
	
	/* Check if data size is too large for single allocation */
	if (data_sz > CHUNK_SIZE) {
		EMSG("Data size %zu exceeds chunk size. Use chunked write commands.", data_sz);
		TEE_Free(obj_id);
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	
	data = TEE_Malloc(data_sz, 0);
	if (!data) {
		TEE_Free(obj_id);
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(data, params[1].memref.buffer, data_sz);

	obj_data_flag = TEE_DATA_FLAG_ACCESS_READ |
			TEE_DATA_FLAG_ACCESS_WRITE |
			TEE_DATA_FLAG_ACCESS_WRITE_META |
			TEE_DATA_FLAG_OVERWRITE;

	res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, obj_id_sz,
					obj_data_flag,
					TEE_HANDLE_NULL,
					NULL, 0,
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_CreatePersistentObject failed 0x%08x", res);
		TEE_Free(obj_id);
		TEE_Free(data);
		return res;
	}

	res = TEE_WriteObjectData(object, data, data_sz);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_WriteObjectData failed 0x%08x", res);
		TEE_CloseAndDeletePersistentObject1(object);
	} else {
		TEE_CloseObject(object);
	}
	TEE_Free(obj_id);
	TEE_Free(data);
	return res;
}

/* Start writing object in chunks - creates/truncates the object */
static TEE_Result write_raw_chunk(uint32_t param_types, TEE_Param params[4],
				   struct write_session *sess)
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,  /* obj_id */
				TEE_PARAM_TYPE_MEMREF_INPUT,  /* data chunk */
				TEE_PARAM_TYPE_VALUE_INPUT,   /* is_first */
				TEE_PARAM_TYPE_NONE);
	TEE_Result res;
	char *obj_id;
	size_t obj_id_sz;
	char *data;
	size_t data_sz;
	uint32_t is_first;

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	obj_id_sz = params[0].memref.size;
	data_sz = params[1].memref.size;
	is_first = params[2].value.a;

	if (data_sz > CHUNK_SIZE) {
		EMSG("Chunk size %zu exceeds maximum %d", data_sz, CHUNK_SIZE);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	obj_id = TEE_Malloc(obj_id_sz, 0);
	if (!obj_id)
		return TEE_ERROR_OUT_OF_MEMORY;
	TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);

	data = TEE_Malloc(data_sz, 0);
	if (!data) {
		TEE_Free(obj_id);
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(data, params[1].memref.buffer, data_sz);

	/* If first chunk, create/truncate object */
	if (is_first) {
		uint32_t obj_data_flag = TEE_DATA_FLAG_ACCESS_WRITE |
					 TEE_DATA_FLAG_ACCESS_WRITE_META |
					 TEE_DATA_FLAG_OVERWRITE;

		res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
						obj_id, obj_id_sz,
						obj_data_flag,
						TEE_HANDLE_NULL,
						NULL, 0,
						&sess->object);
		if (res != TEE_SUCCESS) {
			EMSG("TEE_CreatePersistentObject failed 0x%08x", res);
			TEE_Free(obj_id);
			TEE_Free(data);
			return res;
		}
		sess->in_progress = true;
	} else if (!sess->in_progress) {
		EMSG("No write session in progress");
		TEE_Free(obj_id);
		TEE_Free(data);
		return TEE_ERROR_BAD_STATE;
	}

	/* Write the chunk */
	res = TEE_WriteObjectData(sess->object, data, data_sz);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_WriteObjectData failed 0x%08x", res);
		TEE_CloseAndDeletePersistentObject1(sess->object);
		sess->in_progress = false;
	}

	TEE_Free(obj_id);
	TEE_Free(data);
	return res;
}

/* Finalize writing - closes the object */
static TEE_Result write_raw_final(struct write_session *sess)
{
	if (!sess->in_progress) {
		EMSG("No write session in progress");
		return TEE_ERROR_BAD_STATE;
	}

	TEE_CloseObject(sess->object);
	sess->in_progress = false;
	IMSG("Write session completed successfully");
	return TEE_SUCCESS;
}

static TEE_Result read_raw_object(uint32_t param_types, TEE_Param params[4])
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_OUTPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);
	TEE_ObjectHandle object;
	TEE_ObjectInfo object_info;
	TEE_Result res;
	uint32_t read_bytes;
	char *obj_id;
	size_t obj_id_sz;
	char *chunk_buffer;
	size_t data_sz;
	size_t total_read = 0;
	size_t chunk_size;
	TEE_Time start_time, end_time;
	uint32_t elapsed_ms;

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	obj_id_sz = params[0].memref.size;
	obj_id = TEE_Malloc(obj_id_sz, 0);
	if (!obj_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);

	data_sz = params[1].memref.size;

	chunk_buffer = TEE_Malloc(CHUNK_SIZE, 0);
	if (!chunk_buffer) {
		TEE_Free(obj_id);
		return TEE_ERROR_OUT_OF_MEMORY;
	}

	res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, obj_id_sz,
					TEE_DATA_FLAG_ACCESS_READ |
					TEE_DATA_FLAG_SHARE_READ,
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to open persistent object, res=0x%08x", res);
		TEE_Free(obj_id);
		TEE_Free(chunk_buffer);
		return res;
	}

	res = TEE_GetObjectInfo1(object, &object_info);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to get object info, res=0x%08x", res);
		goto exit;
	}

	if (object_info.dataSize > data_sz) {
		params[1].memref.size = object_info.dataSize;
		res = TEE_ERROR_SHORT_BUFFER;
		goto exit;
	}

	TEE_GetSystemTime(&start_time);

	/* Read data in chunks */
	while (total_read < object_info.dataSize) {
		chunk_size = (object_info.dataSize - total_read > CHUNK_SIZE) ? 
		              CHUNK_SIZE : (object_info.dataSize - total_read);

		res = TEE_ReadObjectData(object, chunk_buffer, chunk_size, &read_bytes);
		if (res != TEE_SUCCESS) {
			EMSG("TEE_ReadObjectData failed 0x%08x at offset %zu", 
			     res, total_read);
			goto exit;
		}

		if (read_bytes != chunk_size) {
			EMSG("Read size mismatch: expected %zu, got %u", 
			     chunk_size, read_bytes);
			res = TEE_ERROR_GENERIC;
			goto exit;
		}

		TEE_MemMove((char *)params[1].memref.buffer + total_read, 
		            chunk_buffer, read_bytes);

		total_read += read_bytes;
	}

	TEE_GetSystemTime(&end_time);
	elapsed_ms = (end_time.seconds - start_time.seconds) * 1000 +
	             (end_time.millis - start_time.millis);

	IMSG("Time taken to read %u bytes: %u ms", object_info.dataSize, elapsed_ms);
	params[1].memref.size = total_read;

exit:
	TEE_CloseObject(object);
	TEE_Free(obj_id);
	TEE_Free(chunk_buffer);
	return res;
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
	struct write_session *sess;

	sess = TEE_Malloc(sizeof(*sess), 0);
	if (!sess)
		return TEE_ERROR_OUT_OF_MEMORY;

	sess->in_progress = false;
	*session = sess;
	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void *session)
{
	struct write_session *sess = session;

	if (sess) {
		if (sess->in_progress)
			TEE_CloseObject(sess->object);
		TEE_Free(sess);
	}
}

TEE_Result TA_InvokeCommandEntryPoint(void *session,
				      uint32_t command,
				      uint32_t param_types,
				      TEE_Param params[4])
{
	struct write_session *sess = session;

	switch (command) {
	case TA_SECURE_STORAGE_CMD_WRITE_RAW:
		return create_raw_object(param_types, params);
	case TA_SECURE_STORAGE_CMD_WRITE_RAW_CHUNK:
		return write_raw_chunk(param_types, params, sess);
	case TA_SECURE_STORAGE_CMD_WRITE_RAW_FINAL:
		return write_raw_final(sess);
	case TA_SECURE_STORAGE_CMD_READ_RAW:
		return read_raw_object(param_types, params);
	case TA_SECURE_STORAGE_CMD_DELETE:
		return delete_object(param_types, params);
	default:
		EMSG("Command ID 0x%x is not supported", command);
		return TEE_ERROR_NOT_SUPPORTED;
	}
}