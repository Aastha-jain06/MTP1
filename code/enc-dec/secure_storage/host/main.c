#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

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

/* Timing structure */
struct timing_info {
	uint32_t encryption_time_ms;
	uint32_t decryption_time_ms;
	size_t file_size;
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
 * Stream file directly to secure storage WITHOUT loading entire file to memory
 * Now with encryption timing measurement
 */
TEEC_Result write_file_to_secure_storage_streaming(struct test_ctx *ctx, 
                                                    char *obj_id,
                                                    const char *filename,
                                                    struct timing_info *timing)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(obj_id);
	int fd;
	char chunk_buffer[CHUNK_SIZE];
	ssize_t bytes_read;
	size_t total_written = 0;
	int is_first = 1;
	struct stat st;
	struct timeval start_tv, end_tv;
	double host_time_sec;

	/* Get file size */
	if (stat(filename, &st) != 0) {
		printf("Error: Cannot stat file %s\n", filename);
		return TEEC_ERROR_ITEM_NOT_FOUND;
	}

	timing->file_size = st.st_size;

	printf("  Streaming file: %s (%zu bytes = %.2f MB)\n", 
	       filename, st.st_size, st.st_size / (1024.0 * 1024.0));

	/* Open source file */
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("Error: Cannot open file %s\n", filename);
		return TEEC_ERROR_ITEM_NOT_FOUND;
	}

	/* Start host-side timing */
	gettimeofday(&start_tv, NULL);

	/* Stream file in chunks - NO FULL FILE IN MEMORY! */
	while ((bytes_read = read(fd, chunk_buffer, CHUNK_SIZE)) > 0) {
		/* Send chunk to TEE */
		memset(&op, 0, sizeof(op));
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						 TEEC_MEMREF_TEMP_INPUT,
						 TEEC_VALUE_INPUT,
						 TEEC_NONE);

		op.params[0].tmpref.buffer = obj_id;
		op.params[0].tmpref.size = id_len;

		op.params[1].tmpref.buffer = chunk_buffer;
		op.params[1].tmpref.size = bytes_read;

		op.params[2].value.a = is_first;

		res = TEEC_InvokeCommand(&ctx->sess,
					 TA_SECURE_STORAGE_CMD_WRITE_RAW_CHUNK,
					 &op, &origin);
		if (res != TEEC_SUCCESS) {
			printf("Error: Write failed at offset %zu: 0x%x / %u\n",
			       total_written, res, origin);
			if (res == TEEC_ERROR_OUT_OF_MEMORY) {
				printf("\n*** STORAGE FULL ***\n");
				printf("Your /data/tee/ partition is too small.\n");
				printf("Current written: %zu bytes (%.2f MB)\n", 
				       total_written, total_written / (1024.0 * 1024.0));
				printf("Check: df -h /data/tee/\n\n");
			}
			close(fd);
			return res;
		}

		total_written += bytes_read;
		is_first = 0;

		/* Progress indicator every 1MB */
		if (total_written % (1024 * 1024) == 0) {
			printf("  Progress: %zu/%zu bytes (%.1f%%) - %.2f MB\n",
			       total_written, st.st_size,
			       (total_written * 100.0) / st.st_size,
			       total_written / (1024.0 * 1024.0));
		}
	}

	close(fd);

	if (bytes_read < 0) {
		printf("Error: Read failed from file\n");
		return TEEC_ERROR_GENERIC;
	}

	printf("  ✓ Total written: %zu bytes (%.2f MB)\n", 
	       total_written, total_written / (1024.0 * 1024.0));

	/* Finalize write and get TEE-side timing */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT,
					 TEEC_VALUE_OUTPUT,
					 TEEC_NONE,
					 TEEC_NONE);

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_SECURE_STORAGE_CMD_WRITE_RAW_FINAL,
				 &op, &origin);
	
	/* End host-side timing */
	gettimeofday(&end_tv, NULL);
	host_time_sec = (end_tv.tv_sec - start_tv.tv_sec) + 
	                (end_tv.tv_usec - start_tv.tv_usec) / 1000000.0;

	if (res != TEEC_SUCCESS) {
		printf("Error: Finalize failed: 0x%x / %u\n", res, origin);
	} else {
		timing->encryption_time_ms = op.params[0].value.a;
		
		printf("  ✓ Write finalized successfully\n");
		printf("\n  === ENCRYPTION TIMING ===\n");
		printf("  TEE Encryption time: %u ms (%.3f seconds)\n", 
		       timing->encryption_time_ms, 
		       timing->encryption_time_ms / 1000.0);
		printf("  Host total time: %.3f seconds\n", host_time_sec);
		printf("  Throughput: %.2f MB/s\n", 
		       (st.st_size / (1024.0 * 1024.0)) / (timing->encryption_time_ms / 1000.0));
	}

	return res;
}

/**
 * Read entire file from secure storage and measure decryption time
 */
TEEC_Result read_entire_file_from_secure_storage(struct test_ctx *ctx, 
                                                   char *obj_id,
                                                   struct timing_info *timing)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(obj_id);
	char *read_buffer = NULL;
	size_t buffer_size;
	struct timeval start_tv, end_tv;
	double host_time_sec;

	printf("  Reading entire file from secure storage...\n");

	/* First, query the size */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_VALUE_OUTPUT,
					 TEEC_NONE);

	char small_buffer[1];
	op.params[0].tmpref.buffer = obj_id;
	op.params[0].tmpref.size = id_len;
	op.params[1].tmpref.buffer = small_buffer;
	op.params[1].tmpref.size = 1;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_SECURE_STORAGE_CMD_READ_RAW,
				 &op, &origin);

	if (res == TEEC_ERROR_SHORT_BUFFER) {
		buffer_size = op.params[1].memref.size;
		printf("  Object size: %zu bytes (%.2f MB)\n", 
		       buffer_size, buffer_size / (1024.0 * 1024.0));
	} else {
		printf("  Error querying object size: 0x%x / %u\n", res, origin);
		return res;
	}

	/* Allocate buffer for entire file */
	read_buffer = malloc(buffer_size);
	if (!read_buffer) {
		printf("  Error: Cannot allocate %zu bytes for reading\n", buffer_size);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	/* Start host-side timing */
	gettimeofday(&start_tv, NULL);

	/* Read entire file */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_VALUE_OUTPUT,
					 TEEC_NONE);

	op.params[0].tmpref.buffer = obj_id;
	op.params[0].tmpref.size = id_len;
	op.params[1].tmpref.buffer = read_buffer;
	op.params[1].tmpref.size = buffer_size;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_SECURE_STORAGE_CMD_READ_RAW,
				 &op, &origin);

	/* End host-side timing */
	gettimeofday(&end_tv, NULL);
	host_time_sec = (end_tv.tv_sec - start_tv.tv_sec) + 
	                (end_tv.tv_usec - start_tv.tv_usec) / 1000000.0;

	if (res != TEEC_SUCCESS) {
		printf("  Error: Read failed: 0x%x / %u\n", res, origin);
		free(read_buffer);
		return res;
	}

	timing->decryption_time_ms = op.params[2].value.a;
	size_t bytes_read = op.params[1].memref.size;

	printf("  ✓ Read complete: %zu bytes\n", bytes_read);
	printf("\n  === DECRYPTION TIMING ===\n");
	printf("  TEE Decryption time: %u ms (%.3f seconds)\n", 
	       timing->decryption_time_ms,
	       timing->decryption_time_ms / 1000.0);
	printf("  Host total time: %.3f seconds\n", host_time_sec);
	printf("  Throughput: %.2f MB/s\n", 
	       (bytes_read / (1024.0 * 1024.0)) / (timing->decryption_time_ms / 1000.0));

	free(read_buffer);
	return res;
}

/**
 * Generate test file with random data
 */
int generate_test_file(const char *filename, size_t size_mb)
{
	int fd;
	size_t total_written = 0;
	size_t chunk_size = 1024 * 1024; // 1MB chunks
	char *buffer;
	size_t target_size = size_mb * 1024 * 1024;

	printf("Generating test file: %s (%zu MB)...\n", filename, size_mb);

	buffer = malloc(chunk_size);
	if (!buffer) {
		printf("Error: Cannot allocate buffer for test file\n");
		return -1;
	}

	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		printf("Error: Cannot create test file %s\n", filename);
		free(buffer);
		return -1;
	}

	/* Fill with pattern */
	memset(buffer, 0xAB, chunk_size);

	while (total_written < target_size) {
		size_t to_write = (target_size - total_written > chunk_size) ?
				   chunk_size : (target_size - total_written);
		
		if (write(fd, buffer, to_write) != to_write) {
			printf("Error: Write failed\n");
			close(fd);
			free(buffer);
			return -1;
		}
		total_written += to_write;
	}

	close(fd);
	free(buffer);
	printf("✓ Test file created: %zu bytes\n", total_written);
	return 0;
}

void print_performance_summary(struct timing_info *timing)
{
	printf("\n=======================================================\n");
	printf("  PERFORMANCE SUMMARY\n");
	printf("=======================================================\n");
	printf("  File size: %.2f MB (%zu bytes)\n", 
	       timing->file_size / (1024.0 * 1024.0), timing->file_size);
	printf("\n  Encryption:\n");
	printf("    Time: %u ms (%.3f seconds)\n", 
	       timing->encryption_time_ms, timing->encryption_time_ms / 1000.0);
	printf("    Throughput: %.2f MB/s\n", 
	       (timing->file_size / (1024.0 * 1024.0)) / (timing->encryption_time_ms / 1000.0));
	printf("\n  Decryption:\n");
	printf("    Time: %u ms (%.3f seconds)\n", 
	       timing->decryption_time_ms, timing->decryption_time_ms / 1000.0);
	printf("    Throughput: %.2f MB/s\n", 
	       (timing->file_size / (1024.0 * 1024.0)) / (timing->decryption_time_ms / 1000.0));
	
	/* Calculate speed ratio */
	if (timing->decryption_time_ms > 0 && timing->encryption_time_ms > 0) {
		double ratio = (double)timing->encryption_time_ms / timing->decryption_time_ms;
		printf("\n  Encryption/Decryption ratio: %.2fx ", ratio);
		if (ratio > 1.0)
			printf("(encryption is %.2fx slower)\n", ratio);
		else
			printf("(decryption is %.2fx slower)\n", 1.0/ratio);
	}
	printf("=======================================================\n");
}

int main(int argc, char *argv[])
{
	struct test_ctx ctx;
	char obj_id[] = "large_test_object";
	const char *test_file;
	struct stat st;
	TEEC_Result res;
	int use_generated_file = 0;
	struct timing_info timing = {0};

	printf("=======================================================\n");
	printf("  OP-TEE Secure Storage - Encryption/Decryption Test\n");
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
		
		/* You can change the size here (in MB) */
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
	 * Test 1: Write file to secure storage (encryption)
	 */
	printf("\n=== TEST 1: Write file to secure storage (encryption) ===\n");
	res = write_file_to_secure_storage_streaming(&ctx, obj_id, test_file, &timing);
	if (res != TEEC_SUCCESS) {
		printf("\n✗ FAILED to write file to secure storage\n");
		if (res == TEEC_ERROR_OUT_OF_MEMORY) {
			printf("\nDiagnosis:\n");
			printf("  - Your /data/tee/ partition is FULL\n");
			printf("  - Run: df -h /data/tee/\n");
			printf("  - You need at least %.0f MB free space\n", 
			       (st.st_size * 1.5) / (1024.0 * 1024.0));
		}
		goto cleanup;
	}
	printf("✓ TEST 1 PASSED\n");

	/*
	 * Test 2: Read entire file from secure storage (decryption)
	 */
	printf("\n=== TEST 2: Read file from secure storage (decryption) ===\n");
	res = read_entire_file_from_secure_storage(&ctx, obj_id, &timing);
	if (res != TEEC_SUCCESS) {
		printf("✗ TEST 2 FAILED\n");
		goto cleanup;
	}
	printf("✓ TEST 2 PASSED\n");

	/*
	 * Test 3: Delete object
	 */
	printf("\n=== TEST 3: Delete stored object ===\n");
	res = delete_secure_object(&ctx, obj_id);
	if (res != TEEC_SUCCESS) {
		printf("✗ TEST 3 FAILED\n");
		goto cleanup;
	}
	printf("✓ Object deleted successfully\n");
	printf("✓ TEST 3 PASSED\n");

	/* Print performance summary */
	print_performance_summary(&timing);

	printf("\n  ✓ ALL TESTS PASSED\n");

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