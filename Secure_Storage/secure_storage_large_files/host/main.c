#include <err.h>
#include <stdio.h>
#include <string.h>

#include <tee_client_api.h>
#include <secure_storage_ta.h>

#define CHUNK_SIZE (16 * 1024)  // 16KB - must match TA

struct test_ctx {
	TEEC_Context ctx;
	TEEC_Session sess;
};

void prepare_tee_session(struct test_ctx *ctx)
{
	TEEC_UUID uuid = TA_SECURE_STORAGE_UUID;
	uint32_t origin;
	TEEC_Result res;

	res = TEEC_InitializeContext(NULL, &ctx->ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	res = TEEC_OpenSession(&ctx->ctx, &ctx->sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, origin);
}

void terminate_tee_session(struct test_ctx *ctx)
{
	TEEC_CloseSession(&ctx->sess);
	TEEC_FinalizeContext(&ctx->ctx);
}

TEEC_Result read_secure_object(struct test_ctx *ctx, char *id,
			char *data, size_t data_len)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(id);

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE, TEEC_NONE);

	op.params[0].tmpref.buffer = id;
	op.params[0].tmpref.size = id_len;

	op.params[1].tmpref.buffer = data;
	op.params[1].tmpref.size = data_len;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_SECURE_STORAGE_CMD_READ_RAW,
				 &op, &origin);
	switch (res) {
	case TEEC_SUCCESS:
	case TEEC_ERROR_SHORT_BUFFER:
	case TEEC_ERROR_ITEM_NOT_FOUND:
		break;
	default:
		printf("Command READ_RAW failed: 0x%x / %u\n", res, origin);
	}

	return res;
}

/* Original write function - for small files only */
TEEC_Result write_secure_object(struct test_ctx *ctx, char *id,
			char *data, size_t data_len)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(id);

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE, TEEC_NONE);

	op.params[0].tmpref.buffer = id;
	op.params[0].tmpref.size = id_len;

	op.params[1].tmpref.buffer = data;
	op.params[1].tmpref.size = data_len;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_SECURE_STORAGE_CMD_WRITE_RAW,
				 &op, &origin);
	if (res != TEEC_SUCCESS)
		printf("Command WRITE_RAW failed: 0x%x / %u\n", res, origin);

	return res;
}

/* New chunked write function for large files */
TEEC_Result write_secure_object_chunked(struct test_ctx *ctx, char *id,
					char *data, size_t data_len)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(id);
	size_t offset = 0;
	size_t chunk_size;
	int is_first = 1;

	printf("  Writing %zu bytes in chunks of %d bytes\n", data_len, CHUNK_SIZE);

	while (offset < data_len) {
		chunk_size = (data_len - offset > CHUNK_SIZE) ? 
		              CHUNK_SIZE : (data_len - offset);

		memset(&op, 0, sizeof(op));
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						 TEEC_MEMREF_TEMP_INPUT,
						 TEEC_VALUE_INPUT,
						 TEEC_NONE);

		op.params[0].tmpref.buffer = id;
		op.params[0].tmpref.size = id_len;

		op.params[1].tmpref.buffer = data + offset;
		op.params[1].tmpref.size = chunk_size;

		op.params[2].value.a = is_first;

		res = TEEC_InvokeCommand(&ctx->sess,
					 TA_SECURE_STORAGE_CMD_WRITE_RAW_CHUNK,
					 &op, &origin);
		if (res != TEEC_SUCCESS) {
			printf("Command WRITE_RAW_CHUNK failed at offset %zu: 0x%x / %u\n",
			       offset, res, origin);
			return res;
		}

		offset += chunk_size;
		is_first = 0;
		
		if (offset % (CHUNK_SIZE * 10) == 0 || offset == data_len)
			printf("  Progress: %zu/%zu bytes (%.1f%%)\n", 
			       offset, data_len, (offset * 100.0) / data_len);
	}

	/* Finalize the write */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_SECURE_STORAGE_CMD_WRITE_RAW_FINAL,
				 &op, &origin);
	if (res != TEEC_SUCCESS)
		printf("Command WRITE_RAW_FINAL failed: 0x%x / %u\n", res, origin);

	return res;
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

#define TEST_OBJECT_SIZE	(1024 * 1024)  // 1MB for testing

int main(void)
{
	struct test_ctx ctx;
	char obj1_id[] = "object#1";
	char obj2_id[] = "object#2";
	char *obj1_data;
	char *read_data;
	TEEC_Result res;

	printf("Prepare session with the TA\n");
	prepare_tee_session(&ctx);

	/* Allocate large buffers */
	obj1_data = malloc(TEST_OBJECT_SIZE);
	read_data = malloc(TEST_OBJECT_SIZE);
	if (!obj1_data || !read_data)
		errx(1, "Failed to allocate test buffers");

	/*
	 * Create large object, read it, delete it.
	 */
	printf("\n=== Test on large object \"%s\" (%d KB) ===\n", 
	       obj1_id, TEST_OBJECT_SIZE / 1024);

	printf("- Create and load object in the TA secure storage\n");
	memset(obj1_data, 0xA1, TEST_OBJECT_SIZE);

	/* Use chunked write for large files */
	res = write_secure_object_chunked(&ctx, obj1_id,
					  obj1_data, TEST_OBJECT_SIZE);
	if (res != TEEC_SUCCESS)
		errx(1, "Failed to create an object in the secure storage");

	printf("- Read back the object\n");
	res = read_secure_object(&ctx, obj1_id,
				 read_data, TEST_OBJECT_SIZE);
	if (res != TEEC_SUCCESS)
		errx(1, "Failed to read an object from the secure storage");
	
	if (memcmp(obj1_data, read_data, TEST_OBJECT_SIZE))
		errx(1, "Unexpected content found in secure storage");

	printf("- Verification successful! Data matches.\n");

	printf("- Delete the object\n");
	res = delete_secure_object(&ctx, obj1_id);
	if (res != TEEC_SUCCESS)
		errx(1, "Failed to delete the object: 0x%x", res);

	/*
	 * Non volatile storage test
	 */
	printf("\n=== Test on object \"%s\" ===\n", obj2_id);

	res = read_secure_object(&ctx, obj2_id,
				  read_data, TEST_OBJECT_SIZE);
	if (res != TEEC_SUCCESS && res != TEEC_ERROR_ITEM_NOT_FOUND)
		errx(1, "Unexpected status when reading an object : 0x%x", res);

	if (res == TEEC_ERROR_ITEM_NOT_FOUND) {
		char data[] = "This is data stored in the secure storage.\n";

		printf("- Object not found in TA secure storage, create it.\n");

		/* Small file - can use original write */
		res = write_secure_object(&ctx, obj2_id,
					  data, sizeof(data));
		if (res != TEEC_SUCCESS)
			errx(1, "Failed to create/load an object");

	} else if (res == TEEC_SUCCESS) {
		printf("- Object found in TA secure storage, delete it.\n");

		res = delete_secure_object(&ctx, obj2_id);
		if (res != TEEC_SUCCESS)
			errx(1, "Failed to delete an object");
	}

	printf("\n=== We're done, close and release TEE resources ===\n");
	
	free(obj1_data);
	free(read_data);
	terminate_tee_session(&ctx);
	
	printf("SUCCESS: All tests passed!\n");
	return 0;
}