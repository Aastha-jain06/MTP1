#include <inttypes.h>
#include <secure_storage_ta.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#define CHUNK_SIZE (16 * 1024)
#define MAX_OBJECT_ID_LEN 256

struct write_session {
	TEE_ObjectHandle object;
	bool in_progress;
	size_t total_bytes_written;
	uint32_t chunk_count;
};

struct tee_memory_stats {
	uint32_t allocated_bytes;
	uint32_t peak_allocated;
	uint32_t allocation_count;
};

static struct tee_memory_stats g_mem_stats = {0};

static void track_allocation(size_t size)
{
	g_mem_stats.allocated_bytes += size;
	g_mem_stats.allocation_count++;
	
	if (g_mem_stats.allocated_bytes > g_mem_stats.peak_allocated) {
		g_mem_stats.peak_allocated = g_mem_stats.allocated_bytes;
	}
}

static void track_deallocation(size_t size)
{
	if (g_mem_stats.allocated_bytes >= size) {
		g_mem_stats.allocated_bytes -= size;
	}
}

static void* tracked_malloc(size_t size)
{
	void *ptr = TEE_Malloc(size, 0);
	if (ptr) {
		track_allocation(size);
	}
	return ptr;
}

static void tracked_free(void *ptr, size_t size)
{
	if (ptr) {
		TEE_Free(ptr);
		track_deallocation(size);
	}
}

/**
 * ⭐ Get TRUE secure storage information by enumerating objects
 * This uses TEE_StartPersistentObjectEnumerator to scan actual secure storage
 */
static TEE_Result get_storage_info(uint32_t param_types, TEE_Param params[4],
                                    const char *test_obj_id, size_t test_obj_id_len)
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_OUTPUT,  // count, storage_id
				TEE_PARAM_TYPE_VALUE_OUTPUT,  // total_size (low, high)
				TEE_PARAM_TYPE_VALUE_OUTPUT,  // test_obj_size (low, high)
				TEE_PARAM_TYPE_NONE);
	
	TEE_ObjectEnumHandle enumerator = TEE_HANDLE_NULL;
	TEE_ObjectInfo obj_info;
	TEE_Result res;
	char obj_id[MAX_OBJECT_ID_LEN];
	size_t obj_id_len;
	uint32_t object_count = 0;
	uint64_t total_size = 0;
	uint64_t test_object_size = 0;
	bool found_test_object = false;

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	IMSG("⭐ Enumerating secure storage objects...");

	/* ⭐ THIS IS THE KEY: Enumerate ACTUAL secure storage */
	res = TEE_AllocatePersistentObjectEnumerator(&enumerator);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate enumerator: 0x%08x", res);
		return res;
	}

	res = TEE_StartPersistentObjectEnumerator(enumerator, TEE_STORAGE_PRIVATE);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to start enumeration: 0x%08x", res);
		TEE_FreePersistentObjectEnumerator(enumerator);
		return res;
	}

	/* ⭐ Scan through ALL objects in secure storage */
	while (true) {
		obj_id_len = sizeof(obj_id);
		res = TEE_GetNextPersistentObject(enumerator, &obj_info, 
		                                   obj_id, &obj_id_len);
		
		if (res == TEE_ERROR_ITEM_NOT_FOUND) {
			/* End of enumeration - this is normal */
			break;
		}
		
		if (res != TEE_SUCCESS) {
			EMSG("Error during enumeration: 0x%08x", res);
			break;
		}

		/* Found an object */
		object_count++;
		total_size += obj_info.dataSize;

		IMSG("  Found object #%u: size=%u bytes", object_count, obj_info.dataSize);

		/* Check if this is our test object */
		if (test_obj_id && obj_id_len == test_obj_id_len &&
		    TEE_MemCompare(obj_id, test_obj_id, obj_id_len) == 0) {
			test_object_size = obj_info.dataSize;
			found_test_object = true;
			IMSG("  ⭐ This is our test object!");
		}
	}

	TEE_ResetPersistentObjectEnumerator(enumerator);
	TEE_FreePersistentObjectEnumerator(enumerator);

	/* Return results */
	params[0].value.a = object_count;
	params[0].value.b = TEE_STORAGE_PRIVATE;  // Storage ID
	
	/* Split 64-bit values into two 32-bit values */
	params[1].value.a = (uint32_t)(total_size & 0xFFFFFFFF);
	params[1].value.b = (uint32_t)(total_size >> 32);
	
	params[2].value.a = (uint32_t)(test_object_size & 0xFFFFFFFF);
	params[2].value.b = (uint32_t)(test_object_size >> 32);

	IMSG("⭐ Storage enumeration complete:");
	IMSG("  Total objects: %u", object_count);
	IMSG("  Total size: %lu bytes (%.2f MB)", 
	     total_size, total_size / (1024.0 * 1024.0));
	if (found_test_object) {
		IMSG("  Test object size: %lu bytes (%.2f MB)",
		     test_object_size, test_object_size / (1024.0 * 1024.0));
	}

	return TEE_SUCCESS;
}

/**
 * Wrapper to get storage info without knowing test object ID
 */
static TEE_Result get_storage_info_wrapper(uint32_t param_types, TEE_Param params[4])
{
	/* Call with no test object filtering */
	return get_storage_info(param_types, params, NULL, 0);
}

static TEE_Result get_memory_stats(uint32_t param_types, TEE_Param params[4])
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_OUTPUT,
				TEE_PARAM_TYPE_VALUE_OUTPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	params[0].value.a = g_mem_stats.allocated_bytes;
	params[0].value.b = g_mem_stats.peak_allocated;
	params[1].value.a = g_mem_stats.allocation_count;

	IMSG("TEE Memory Stats - Current: %u bytes, Peak: %u bytes, Allocations: %u",
	     g_mem_stats.allocated_bytes, g_mem_stats.peak_allocated,
	     g_mem_stats.allocation_count);

	return TEE_SUCCESS;
}

static TEE_Result reset_memory_stats(void)
{
	g_mem_stats.allocated_bytes = 0;
	g_mem_stats.peak_allocated = 0;
	g_mem_stats.allocation_count = 0;
	IMSG("TEE Memory statistics reset");
	return TEE_SUCCESS;
}

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
	obj_id = tracked_malloc(obj_id_sz);
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
		tracked_free(obj_id, obj_id_sz);
		return res;
	}

	TEE_CloseAndDeletePersistentObject1(object);
	tracked_free(obj_id, obj_id_sz);

	IMSG("Object deleted successfully");
	return res;
}

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
	obj_id = tracked_malloc(obj_id_sz);
	if (!obj_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);

	data_sz = params[1].memref.size;
	
	if (data_sz > CHUNK_SIZE) {
		EMSG("Data size %zu exceeds chunk size. Use chunked write commands.", data_sz);
		tracked_free(obj_id, obj_id_sz);
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	
	data = tracked_malloc(data_sz);
	if (!data) {
		tracked_free(obj_id, obj_id_sz);
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
		tracked_free(obj_id, obj_id_sz);
		tracked_free(data, data_sz);
		return res;
	}

	res = TEE_WriteObjectData(object, data, data_sz);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_WriteObjectData failed 0x%08x", res);
		TEE_CloseAndDeletePersistentObject1(object);
	} else {
		TEE_CloseObject(object);
		IMSG("Object created: %zu bytes", data_sz);
	}
	tracked_free(obj_id, obj_id_sz);
	tracked_free(data, data_sz);
	return res;
}

static TEE_Result write_raw_chunk(uint32_t param_types, TEE_Param params[4],
				   struct write_session *sess)
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_VALUE_INPUT,
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

	obj_id = tracked_malloc(obj_id_sz);
	if (!obj_id)
		return TEE_ERROR_OUT_OF_MEMORY;
	TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);

	data = tracked_malloc(data_sz);
	if (!data) {
		tracked_free(obj_id, obj_id_sz);
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(data, params[1].memref.buffer, data_sz);

	if (is_first) {
		uint32_t obj_data_flag = TEE_DATA_FLAG_ACCESS_WRITE |
					 TEE_DATA_FLAG_ACCESS_WRITE_META |
					 TEE_DATA_FLAG_OVERWRITE;

		IMSG("Starting chunked write session");
		IMSG("TEE Memory before object creation - Current: %u bytes, Peak: %u bytes",
		     g_mem_stats.allocated_bytes, g_mem_stats.peak_allocated);

		res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
						obj_id, obj_id_sz,
						obj_data_flag,
						TEE_HANDLE_NULL,
						NULL, 0,
						&sess->object);
		if (res != TEE_SUCCESS) {
			EMSG("TEE_CreatePersistentObject failed 0x%08x", res);
			tracked_free(obj_id, obj_id_sz);
			tracked_free(data, data_sz);
			return res;
		}
		sess->in_progress = true;
		sess->total_bytes_written = 0;
		sess->chunk_count = 0;
	} else if (!sess->in_progress) {
		EMSG("No write session in progress");
		tracked_free(obj_id, obj_id_sz);
		tracked_free(data, data_sz);
		return TEE_ERROR_BAD_STATE;
	}

	res = TEE_WriteObjectData(sess->object, data, data_sz);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_WriteObjectData failed 0x%08x", res);
		TEE_CloseAndDeletePersistentObject1(sess->object);
		sess->in_progress = false;
	} else {
		sess->total_bytes_written += data_sz;
		sess->chunk_count++;

		if (sess->chunk_count % 10 == 0) {
			IMSG("Progress: %u chunks written, %zu total bytes, TEE mem: %u bytes",
			     sess->chunk_count, sess->total_bytes_written,
			     g_mem_stats.allocated_bytes);
		}
	}

	tracked_free(obj_id, obj_id_sz);
	tracked_free(data, data_sz);
	return res;
}

static TEE_Result write_raw_final(struct write_session *sess)
{
	if (!sess->in_progress) {
		EMSG("No write session in progress");
		return TEE_ERROR_BAD_STATE;
	}

	TEE_CloseObject(sess->object);
	sess->in_progress = false;
	
	IMSG("Write session completed successfully");
	IMSG("  Total chunks: %u", sess->chunk_count);
	IMSG("  Total bytes written: %zu", sess->total_bytes_written);
	IMSG("  TEE Memory - Current: %u bytes, Peak: %u bytes",
	     g_mem_stats.allocated_bytes, g_mem_stats.peak_allocated);
	
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
	obj_id = tracked_malloc(obj_id_sz);
	if (!obj_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);

	data_sz = params[1].memref.size;

	chunk_buffer = tracked_malloc(CHUNK_SIZE);
	if (!chunk_buffer) {
		tracked_free(obj_id, obj_id_sz);
		return TEE_ERROR_OUT_OF_MEMORY;
	}

	IMSG("TEE Memory before read - Current: %u bytes, Peak: %u bytes",
	     g_mem_stats.allocated_bytes, g_mem_stats.peak_allocated);

	res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, obj_id_sz,
					TEE_DATA_FLAG_ACCESS_READ |
					TEE_DATA_FLAG_SHARE_READ,
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to open persistent object, res=0x%08x", res);
		tracked_free(obj_id, obj_id_sz);
		tracked_free(chunk_buffer, CHUNK_SIZE);
		return res;
	}

	res = TEE_GetObjectInfo1(object, &object_info);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to get object info, res=0x%08x", res);
		goto exit;
	}

	IMSG("Reading object: %u bytes", object_info.dataSize);

	if (object_info.dataSize > data_sz) {
		params[1].memref.size = object_info.dataSize;
		res = TEE_ERROR_SHORT_BUFFER;
		goto exit;
	}

	TEE_GetSystemTime(&start_time);

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

	IMSG("Read completed: %u bytes in %u ms", object_info.dataSize, elapsed_ms);
	IMSG("TEE Memory after read - Current: %u bytes, Peak: %u bytes",
	     g_mem_stats.allocated_bytes, g_mem_stats.peak_allocated);
	
	params[1].memref.size = total_read;

exit:
	TEE_CloseObject(object);
	tracked_free(obj_id, obj_id_sz);
	tracked_free(chunk_buffer, CHUNK_SIZE);
	return res;
}

TEE_Result TA_CreateEntryPoint(void)
{
	IMSG("TA Create Entry Point - Initializing TEE memory tracking");
	reset_memory_stats();
	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
	IMSG("TA Destroy Entry Point - Final memory stats:");
	IMSG("  Allocated: %u bytes", g_mem_stats.allocated_bytes);
	IMSG("  Peak: %u bytes", g_mem_stats.peak_allocated);
	IMSG("  Total allocations: %u", g_mem_stats.allocation_count);
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t __unused param_types,
				    TEE_Param __unused params[4],
				    void **session)
{
	struct write_session *sess;

	IMSG("Opening new session");

	sess = tracked_malloc(sizeof(*sess));
	if (!sess)
		return TEE_ERROR_OUT_OF_MEMORY;

	sess->in_progress = false;
	sess->total_bytes_written = 0;
	sess->chunk_count = 0;
	*session = sess;
	
	IMSG("Session opened - TEE Memory: %u bytes allocated",
	     g_mem_stats.allocated_bytes);
	
	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void *session)
{
	struct write_session *sess = session;

	IMSG("Closing session");

	if (sess) {
		if (sess->in_progress) {
			IMSG("Warning: Closing session with write in progress");
			TEE_CloseObject(sess->object);
		}
		tracked_free(sess, sizeof(*sess));
	}
	
	IMSG("Session closed - TEE Memory: %u bytes allocated",
	     g_mem_stats.allocated_bytes);
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
	case TA_SECURE_STORAGE_CMD_GET_MEM_STATS:
		return get_memory_stats(param_types, params);
	case TA_SECURE_STORAGE_CMD_RESET_MEM_STATS:
		return reset_memory_stats();
	case TA_SECURE_STORAGE_CMD_GET_STORAGE_INFO:
		return get_storage_info_wrapper(param_types, params);
	default:
		EMSG("Command ID 0x%x is not supported", command);
		return TEE_ERROR_NOT_SUPPORTED;
	}
}
