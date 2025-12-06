#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* TA API: UUID and command IDs */
#include <secure_storage_ta.h>

#define CHUNK_SIZE (16 * 1024)  // 16KB - must match TA

/* Performance metrics structure */
struct performance_metrics {
	struct timeval start_time;
	struct timeval end_time;
	struct rusage start_usage;
	struct rusage end_usage;
	double elapsed_time_ms;
	double user_cpu_time_ms;
	double system_cpu_time_ms;
	double total_cpu_time_ms;
	double cpu_utilization_percent;
	size_t bytes_processed;
	double throughput_mbps;
};

/* TEE resources */
struct test_ctx {
	TEEC_Context ctx;
	TEEC_Session sess;
};

void start_performance_measurement(struct performance_metrics *perf)
{
	gettimeofday(&perf->start_time, NULL);
	getrusage(RUSAGE_SELF, &perf->start_usage);
}

void end_performance_measurement(struct performance_metrics *perf, size_t bytes)
{
	gettimeofday(&perf->end_time, NULL);
	getrusage(RUSAGE_SELF, &perf->end_usage);
	
	/* Calculate elapsed time in milliseconds */
	perf->elapsed_time_ms = (perf->end_time.tv_sec - perf->start_time.tv_sec) * 1000.0 +
	                        (perf->end_time.tv_usec - perf->start_time.tv_usec) / 1000.0;
	
	/* Calculate CPU time in milliseconds */
	perf->user_cpu_time_ms = 
		(perf->end_usage.ru_utime.tv_sec - perf->start_usage.ru_utime.tv_sec) * 1000.0 +
		(perf->end_usage.ru_utime.tv_usec - perf->start_usage.ru_utime.tv_usec) / 1000.0;
	
	perf->system_cpu_time_ms = 
		(perf->end_usage.ru_stime.tv_sec - perf->start_usage.ru_stime.tv_sec) * 1000.0 +
		(perf->end_usage.ru_stime.tv_usec - perf->start_usage.ru_stime.tv_usec) / 1000.0;
	
	perf->total_cpu_time_ms = perf->user_cpu_time_ms + perf->system_cpu_time_ms;
	
	/* Calculate CPU utilization percentage */
	if (perf->elapsed_time_ms > 0) {
		perf->cpu_utilization_percent = (perf->total_cpu_time_ms / perf->elapsed_time_ms) * 100.0;
	} else {
		perf->cpu_utilization_percent = 0.0;
	}
	
	/* Calculate throughput */
	perf->bytes_processed = bytes;
	if (perf->elapsed_time_ms > 0) {
		perf->throughput_mbps = (bytes / (1024.0 * 1024.0)) / (perf->elapsed_time_ms / 1000.0);
	} else {
		perf->throughput_mbps = 0.0;
	}
}

void print_performance_metrics(const char *operation, struct performance_metrics *perf)
{
	printf("\n╔════════════════════════════════════════════════════════╗\n");
	printf("║  Performance Metrics: %-30s  ║\n", operation);
	printf("╠════════════════════════════════════════════════════════╣\n");
	printf("║  Elapsed Time:        %10.2f ms                   ║\n", perf->elapsed_time_ms);
	printf("║  User CPU Time:       %10.2f ms                   ║\n", perf->user_cpu_time_ms);
	printf("║  System CPU Time:     %10.2f ms                   ║\n", perf->system_cpu_time_ms);
	printf("║  Total CPU Time:      %10.2f ms                   ║\n", perf->total_cpu_time_ms);
	printf("║  CPU Utilization:     %10.2f %%                   ║\n", perf->cpu_utilization_percent);
	printf("║  Data Processed:      %10.2f MB                  ║\n", perf->bytes_processed / (1024.0 * 1024.0));
	printf("║  Throughput:          %10.2f MB/s                ║\n", perf->throughput_mbps);
	printf("╚════════════════════════════════════════════════════════╝\n");
}

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
 */
TEEC_Result write_file_to_secure_storage_streaming(struct test_ctx *ctx, 
                                                    char *obj_id,
                                                    const char *filename,
                                                    struct performance_metrics *perf)
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

	/* Get file size */
	if (stat(filename, &st) != 0) {
		printf("Error: Cannot stat file %s\n", filename);
		return TEEC_ERROR_ITEM_NOT_FOUND;
	}

	printf("  Streaming file: %s (%zu bytes = %.2f MB)\n", 
	       filename, st.st_size, st.st_size / (1024.0 * 1024.0));

	/* Open source file */
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("Error: Cannot open file %s\n", filename);
		return TEEC_ERROR_ITEM_NOT_FOUND;
	}

	/* Start performance measurement */
	start_performance_measurement(perf);

	/* Stream file in chunks */
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

	/* Finalize write */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_SECURE_STORAGE_CMD_WRITE_RAW_FINAL,
				 &op, &origin);
	
	/* End performance measurement */
	end_performance_measurement(perf, total_written);
	
	if (res != TEEC_SUCCESS)
		printf("Error: Finalize failed: 0x%x / %u\n", res, origin);
	else
		printf("  ✓ Write finalized successfully\n");

	return res;
}

/**
 * Read from secure storage using the original read command
 * This reads in chunks internally in the TA
 */
TEEC_Result read_secure_object_full(struct test_ctx *ctx, char *obj_id, 
                                     size_t expected_size,
                                     struct performance_metrics *perf)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(obj_id);
	char *read_buffer = NULL;

	printf("  Reading object from secure storage...\n");

	/* Allocate buffer for expected size */
	read_buffer = malloc(expected_size);
	if (!read_buffer) {
		printf("Error: Cannot allocate read buffer of %zu bytes\n", expected_size);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	/* Start performance measurement */
	start_performance_measurement(perf);

	/* Setup read operation */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE, TEEC_NONE);

	op.params[0].tmpref.buffer = obj_id;
	op.params[0].tmpref.size = id_len;

	op.params[1].tmpref.buffer = read_buffer;
	op.params[1].tmpref.size = expected_size;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_SECURE_STORAGE_CMD_READ_RAW,
				 &op, &origin);

	/* End performance measurement */
	size_t bytes_read = op.params[1].memref.size;
	end_performance_measurement(perf, bytes_read);

	if (res == TEEC_SUCCESS) {
		printf("  ✓ Successfully read %zu bytes (%.2f MB)\n", 
		       bytes_read, bytes_read / (1024.0 * 1024.0));
	} else if (res == TEEC_ERROR_SHORT_BUFFER) {
		printf("  Error: Buffer too small. Need %zu bytes\n", bytes_read);
	} else {
		printf("  Error reading object: 0x%x / %u\n", res, origin);
	}

	free(read_buffer);
	return res;
}

/**
 * Read and verify size with performance measurement
 */
TEEC_Result read_and_verify_size(struct test_ctx *ctx, char *obj_id, 
                                  size_t expected_size,
                                  struct performance_metrics *perf)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(obj_id);
	char small_buffer[1];

	printf("  Verifying object size...\n");

	/* Start performance measurement */
	start_performance_measurement(perf);

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

	/* End performance measurement */
	end_performance_measurement(perf, 0);

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

int main(int argc, char *argv[])
{
	struct test_ctx ctx;
	char obj_id[] = "large_test_object";
	const char *test_file;
	struct stat st;
	TEEC_Result res;
	int use_generated_file = 0;
	
	/* Performance metrics */
	struct performance_metrics write_perf, read_perf, verify_perf;

	printf("=======================================================\n");
	printf("  OP-TEE Secure Storage - Performance Analysis\n");
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
	 * Test 1: Write file to secure storage with performance metrics
	 */
	printf("\n=== TEST 1: Write file to secure storage (streaming) ===\n");
	res = write_file_to_secure_storage_streaming(&ctx, obj_id, test_file, &write_perf);
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
	print_performance_metrics("WRITE Operation", &write_perf);

	/*
	 * Test 2: Verify object exists and has correct size
	 */
	printf("\n=== TEST 2: Verify stored object ===\n");
	res = read_and_verify_size(&ctx, obj_id, st.st_size, &verify_perf);
	if (res != TEEC_SUCCESS) {
		printf("✗ TEST 2 FAILED\n");
		goto cleanup;
	}
	printf("✓ TEST 2 PASSED\n");
	print_performance_metrics("SIZE VERIFICATION", &verify_perf);

	/*
	 * Test 3: Read entire object from secure storage
	 */
	printf("\n=== TEST 3: Read entire object from secure storage ===\n");
	res = read_secure_object_full(&ctx, obj_id, st.st_size, &read_perf);
	if (res != TEEC_SUCCESS) {
		printf("✗ TEST 3 FAILED\n");
		goto cleanup;
	}
	printf("✓ TEST 3 PASSED\n");
	print_performance_metrics("READ Operation", &read_perf);

	/*
	 * Test 4: Delete object
	 */
	printf("\n=== TEST 4: Delete stored object ===\n");
	res = delete_secure_object(&ctx, obj_id);
	if (res != TEEC_SUCCESS) {
		printf("✗ TEST 4 FAILED\n");
		goto cleanup;
	}
	printf("✓ Object deleted successfully\n");
	printf("✓ TEST 4 PASSED\n");

	/* Summary */
	printf("\n╔════════════════════════════════════════════════════════╗\n");
	printf("║              PERFORMANCE SUMMARY                       ║\n");
	printf("╠════════════════════════════════════════════════════════╣\n");
	printf("║  File Size:           %10.2f MB                  ║\n", st.st_size / (1024.0 * 1024.0));
	printf("╠════════════════════════════════════════════════════════╣\n");
	printf("║  Write Throughput:    %10.2f MB/s                ║\n", write_perf.throughput_mbps);
	printf("║  Write CPU Usage:     %10.2f %%                   ║\n", write_perf.cpu_utilization_percent);
	printf("║  Write Time:          %10.2f ms                   ║\n", write_perf.elapsed_time_ms);
	printf("╠════════════════════════════════════════════════════════╣\n");
	printf("║  Read Throughput:     %10.2f MB/s                ║\n", read_perf.throughput_mbps);
	printf("║  Read CPU Usage:      %10.2f %%                   ║\n", read_perf.cpu_utilization_percent);
	printf("║  Read Time:           %10.2f ms                   ║\n", read_perf.elapsed_time_ms);
	printf("╠════════════════════════════════════════════════════════╣\n");
	printf("║  Verify Time:         %10.2f ms                   ║\n", verify_perf.elapsed_time_ms);
	printf("╚════════════════════════════════════════════════════════╝\n");

	printf("\n=======================================================\n");
	printf("  ✓ ALL TESTS PASSED\n");
	printf("=======================================================\n");

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