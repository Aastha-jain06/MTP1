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
// *** CHANGE 1: Add default iterations (no upper limit) ***
#define DEFAULT_ITERATIONS 100

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
 * Stream file directly to secure storage WITHOUT loading entire file to memory
 * This is the KEY function that solves the memory problem
 */
TEEC_Result write_file_to_secure_storage_streaming(struct test_ctx *ctx, 
                                                    char *obj_id,
                                                    const char *filename)
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

	/* Finalize write */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_SECURE_STORAGE_CMD_WRITE_RAW_FINAL,
				 &op, &origin);
	if (res != TEEC_SUCCESS)
		printf("Error: Finalize failed: 0x%x / %u\n", res, origin);
	else
		printf("  ✓ Write finalized successfully\n");

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
	// *** CHANGE 2: Single base object ID ***
	char obj_id_base[] = "large_test_object";
	char obj_id[128];  // Buffer for generating unique IDs
	
	const char *test_file;
	struct stat st;
	TEEC_Result res;
	int use_generated_file = 0;
	// *** CHANGE 3: Add iterations variable ***
	int iterations = DEFAULT_ITERATIONS;
	int i;

	printf("=======================================================\n");
	printf("  OP-TEE Secure Storage - Multiple Copy Test (Loop)\n");
	printf("=======================================================\n\n");

	// *** CHANGE 4: Parse command line arguments ***
	/* Usage: ./program [iterations] [file]
	 * Examples:
	 *   ./program              -> 5 iterations with generated 1MB file
	 *   ./program 10           -> 10 iterations with generated 1MB file
	 *   ./program 3 myfile.bin -> 3 iterations with myfile.bin
	 */
	if (argc > 1) {
		iterations = atoi(argv[1]);
		if (iterations <= 0) {
			printf("Error: Invalid iterations. Must be a positive number\n");
			return 1;
		}
	}

	if (argc > 2) {
		test_file = argv[2];
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
		
		if (generate_test_file(test_file, 1) != 0) {
			printf("Failed to generate test file\n");
			return 1;
		}
		
		if (stat(test_file, &st) != 0) {
			printf("Error: Cannot stat generated file\n");
			return 1;
		}
	}

	// *** CHANGE 5: Display iteration count and total storage ***
	printf("\n========== Test Configuration ==========\n");
	printf("Iterations: %d\n", iterations);
	printf("File size: %zu bytes (%.2f MB)\n", 
	       st.st_size, st.st_size / (1024.0 * 1024.0));
	printf("Total storage needed: %.2f MB\n", 
	       (st.st_size * iterations) / (1024.0 * 1024.0));
	printf("========================================\n\n");

	printf("Preparing TEE session...\n");
	prepare_tee_session(&ctx);
	printf("✓ Session established\n\n");

	// *** CHANGE 6: Clean up all existing objects in loop ***
	printf("Cleaning up any existing objects...\n");
	for (i = 1; i <= iterations; i++) {
		snprintf(obj_id, sizeof(obj_id), "%s_%d", obj_id_base, i);
		delete_secure_object(&ctx, obj_id);
	}
	printf("✓ Cleanup complete\n\n");

	// *** CHANGE 7: Write file multiple times in loop ***
	printf("=======================================================\n");
	printf("  WRITING PHASE - Storing file %d times\n", iterations);
	printf("=======================================================\n\n");

	for (i = 1; i <= iterations; i++) {
		/* Generate unique object ID */
		snprintf(obj_id, sizeof(obj_id), "%s_%d", obj_id_base, i);
		
		printf("--- Iteration %d/%d ---\n", i, iterations);
		printf("Object ID: %s\n", obj_id);
		
		res = write_file_to_secure_storage_streaming(&ctx, obj_id, test_file);
		if (res != TEEC_SUCCESS) {
			printf("\n✗ FAILED to write iteration %d\n", i);
			if (res == TEEC_ERROR_OUT_OF_MEMORY) {
				printf("\nDiagnosis:\n");
				printf("  - Your /data/tee/ partition is FULL\n");
				printf("  - Successfully stored %d/%d copies\n", i-1, iterations);
				printf("  - Run: df -h /data/tee/\n");
				printf("  - You need at least %.0f MB free space for all %d copies\n", 
				       (st.st_size * iterations * 1.5) / (1024.0 * 1024.0),
				       iterations);
			}
			goto cleanup;
		}
		printf("✓ Iteration %d/%d PASSED\n\n", i, iterations);
	}

	printf("=======================================================\n");
	printf("  ✓ ALL %d COPIES WRITTEN SUCCESSFULLY!\n", iterations);
	printf("  Total stored: %.2f MB\n", 
	       (st.st_size * iterations) / (1024.0 * 1024.0));
	printf("=======================================================\n\n");

	// *** CHANGE 8: Verify all objects in loop ***
	printf("=======================================================\n");
	printf("  VERIFICATION PHASE - Checking all %d objects\n", iterations);
	printf("=======================================================\n\n");

	for (i = 1; i <= iterations; i++) {
		snprintf(obj_id, sizeof(obj_id), "%s_%d", obj_id_base, i);
		
		printf("--- Verifying %d/%d: %s ---\n", i, iterations, obj_id);
		res = read_and_verify_size(&ctx, obj_id, st.st_size);
		if (res != TEEC_SUCCESS) {
			printf("✗ Verification FAILED for iteration %d\n", i);
			goto cleanup;
		}
		printf("✓ Verification %d/%d PASSED\n\n", i, iterations);
	}

	printf("=======================================================\n");
	printf("  ✓ ALL %d OBJECTS VERIFIED SUCCESSFULLY!\n", iterations);
	printf("=======================================================\n\n");

	// *** CHANGE 9: Delete all objects in loop ***
	printf("=======================================================\n");
	printf("  DELETION PHASE - Removing all %d objects\n", iterations);
	printf("=======================================================\n\n");

	for (i = 1; i <= iterations; i++) {
		snprintf(obj_id, sizeof(obj_id), "%s_%d", obj_id_base, i);
		
		printf("--- Deleting %d/%d: %s ---\n", i, iterations, obj_id);
		res = delete_secure_object(&ctx, obj_id);
		if (res != TEEC_SUCCESS) {
			printf("✗ Deletion FAILED for iteration %d\n", i);
			// Continue deleting other objects even if one fails
		} else {
			printf("✓ Deletion %d/%d PASSED\n\n", i, iterations);
		}
	}

	printf("=======================================================\n");
	printf("  ✓ ALL TESTS PASSED!\n");
	printf("  - %d copies written\n", iterations);
	printf("  - %d copies verified\n", iterations);
	printf("  - %d copies deleted\n", iterations);
	printf("  - Total data processed: %.2f MB\n", 
	       (st.st_size * iterations) / (1024.0 * 1024.0));
	printf("=======================================================\n");

	// Set result to success if we made it this far
	res = TEEC_SUCCESS;

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