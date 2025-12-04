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
	case TA_SECURE_STORAGE_CMD_READ_RAW:
		return read_raw_object(param_types, params);

	default:
		EMSG("Command ID 0x%x is not supported", command);
		return TEE_ERROR_NOT_SUPPORTED;
	}
}