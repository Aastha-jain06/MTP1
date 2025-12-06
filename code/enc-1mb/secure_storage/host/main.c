#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>

/* OP-TEE TEE client API */
#include <tee_client_api.h>

/* TA API */
#include <secure_storage_ta.h>

#define CHUNK_SIZE (16 * 1024)  // 16KB - must match TA
#define AES_BLOCK_SIZE 16

/* TEE resources */
struct test_ctx {
	TEEC_Context ctx;
	TEEC_Session sess;
};

/* Timing and CPU utilization info */
struct perf_info {
	uint32_t encryption_time_ms;
	uint32_t decryption_time_ms;
	size_t file_size;
	double host_enc_time_sec;
	double host_dec_time_sec;
	double cpu_usage_enc;
	double cpu_usage_dec;
};

/* CPU usage calculation helpers */
struct cpu_snapshot {
	struct timeval wall_time;
	struct rusage usage;
};

void take_cpu_snapshot(struct cpu_snapshot *snap)
{
	gettimeofday(&snap->wall_time, NULL);
	getrusage(RUSAGE_SELF, &snap->usage);
}

double calculate_cpu_usage(struct cpu_snapshot *start, struct cpu_snapshot *end)
{
	double wall_sec = (end->wall_time.tv_sec - start->wall_time.tv_sec) +
	                  (end->wall_time.tv_usec - start->wall_time.tv_usec) / 1000000.0;
	
	double cpu_sec = (end->usage.ru_utime.tv_sec - start->usage.ru_utime.tv_sec) +
	                 (end->usage.ru_utime.tv_usec - start->usage.ru_utime.tv_usec) / 1000000.0 +
	                 (end->usage.ru_stime.tv_sec - start->usage.ru_stime.tv_sec) +
	                 (end->usage.ru_stime.tv_usec - start->usage.ru_stime.tv_usec) / 1000000.0;
	
	if (wall_sec > 0)
		return (cpu_sec / wall_sec) * 100.0;
	return 0.0;
}

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
		errx(1, "TEEC_OpenSession failed with code 0x%x origin 0x%x",
			res, origin);
}

void terminate_tee_session(struct test_ctx *ctx)
{
	TEEC_CloseSession(&ctx->sess);
	TEEC_FinalizeContext(&ctx->ctx);
}

/* Pad data to AES block size (PKCS#7 padding) */
size_t pad_data(uint8_t *data, size_t data_len, size_t buffer_size)
{
	size_t padding_len = AES_BLOCK_SIZE - (data_len % AES_BLOCK_SIZE);
	
	if (data_len + padding_len > buffer_size)
		return 0;
	
	for (size_t i = 0; i < padding_len; i++)
		data[data_len + i] = (uint8_t)padding_len;
	
	return data_len + padding_len;
}

/* Remove PKCS#7 padding */
size_t unpad_data(uint8_t *data, size_t data_len)
{
	if (data_len == 0)
		return 0;
	
	uint8_t padding_len = data[data_len - 1];
	
	if (padding_len == 0 || padding_len > AES_BLOCK_SIZE)
		return data_len;
	
	/* Verify padding */
	for (size_t i = data_len - padding_len; i < data_len; i++) {
		if (data[i] != padding_len)
			return data_len;
	}
	
	return data_len - padding_len;
}

/* Encrypt file in normal world */
TEEC_Result encrypt_file(struct test_ctx *ctx, const char *input_file,
                         const char *output_file, struct perf_info *perf)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	int in_fd, out_fd;
	uint8_t *plain_buf, *cipher_buf;
	ssize_t bytes_read;
	size_t total_encrypted = 0;
	int is_first = 1;
	struct stat st;
	struct cpu_snapshot cpu_start, cpu_end;
	struct timeval wall_start, wall_end;
	uint64_t original_size;
	
	if (stat(input_file, &st) != 0) {
		printf("Error: Cannot stat file %s\n", input_file);
		return TEEC_ERROR_ITEM_NOT_FOUND;
	}
	
	perf->file_size = st.st_size;
	original_size = st.st_size;
	
	printf("\n=== ENCRYPTION ===\n");
	printf("Input file: %s (%zu bytes = %.2f MB)\n", 
	       input_file, st.st_size, st.st_size / (1024.0 * 1024.0));
	
	/* Allocate buffers */
	plain_buf = malloc(CHUNK_SIZE + AES_BLOCK_SIZE);
	cipher_buf = malloc(CHUNK_SIZE + AES_BLOCK_SIZE);
	if (!plain_buf || !cipher_buf) {
		printf("Error: Cannot allocate buffers\n");
		free(plain_buf);
		free(cipher_buf);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}
	
	/* Open files */
	in_fd = open(input_file, O_RDONLY);
	if (in_fd < 0) {
		printf("Error: Cannot open input file\n");
		free(plain_buf);
		free(cipher_buf);
		return TEEC_ERROR_ITEM_NOT_FOUND;
	}
	
	out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (out_fd < 0) {
		printf("Error: Cannot create output file\n");
		close(in_fd);
		free(plain_buf);
		free(cipher_buf);
		return TEEC_ERROR_GENERIC;
	}
	
	/* Write original file size as header (8 bytes) */
	if (write(out_fd, &original_size, sizeof(original_size)) != sizeof(original_size)) {
		printf("Error: Cannot write header\n");
		close(in_fd);
		close(out_fd);
		free(plain_buf);
		free(cipher_buf);
		return TEEC_ERROR_GENERIC;
	}
	
	/* Start timing */
	gettimeofday(&wall_start, NULL);
	take_cpu_snapshot(&cpu_start);
	
	/* Process file in chunks */
	while ((bytes_read = read(in_fd, plain_buf, CHUNK_SIZE)) > 0) {
		size_t padded_size = bytes_read;
		
		/* Pad last chunk if NOT multiple of AES block size */
		if (bytes_read % AES_BLOCK_SIZE != 0) {
			padded_size = pad_data(plain_buf, bytes_read, 
			                       CHUNK_SIZE + AES_BLOCK_SIZE);
			if (padded_size == 0) {
				printf("Error: Padding failed\n");
				res = TEEC_ERROR_GENERIC;
				goto cleanup_enc;
			}
		}
		
		/* Encrypt chunk via TEE */
		memset(&op, 0, sizeof(op));
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						 TEEC_MEMREF_TEMP_OUTPUT,
						 TEEC_VALUE_INPUT,
						 TEEC_VALUE_OUTPUT);
		
		op.params[0].tmpref.buffer = plain_buf;
		op.params[0].tmpref.size = padded_size;
		op.params[1].tmpref.buffer = cipher_buf;
		op.params[1].tmpref.size = CHUNK_SIZE + AES_BLOCK_SIZE;
		op.params[2].value.a = is_first;
		
		res = TEEC_InvokeCommand(&ctx->sess,
					 TA_SECURE_STORAGE_CMD_ENCRYPT_CHUNK,
					 &op, &origin);
		if (res != TEEC_SUCCESS) {
			printf("Error: Encryption failed at offset %zu: 0x%x / %u\n",
			       total_encrypted, res, origin);
			goto cleanup_enc;
		}
		
		/* Write encrypted data */
		size_t encrypted_size = op.params[1].tmpref.size;
		if (write(out_fd, cipher_buf, encrypted_size) != encrypted_size) {
			printf("Error: Write failed\n");
			res = TEEC_ERROR_GENERIC;
			goto cleanup_enc;
		}
		
		total_encrypted += bytes_read;
		is_first = 0;
		
		/* Progress indicator */
		if (total_encrypted % (256 * 1024) == 0 || bytes_read < CHUNK_SIZE) {
			printf("  Progress: %zu/%zu bytes (%.1f%%)\n",
			       total_encrypted, st.st_size,
			       (total_encrypted * 100.0) / st.st_size);
		}
	}
	
	/* End timing */
	gettimeofday(&wall_end, NULL);
	take_cpu_snapshot(&cpu_end);
	
	perf->host_enc_time_sec = (wall_end.tv_sec - wall_start.tv_sec) +
	                          (wall_end.tv_usec - wall_start.tv_usec) / 1000000.0;
	perf->cpu_usage_enc = calculate_cpu_usage(&cpu_start, &cpu_end);
	
	printf("✓ Encryption complete: %zu bytes\n", total_encrypted);
	
	res = TEEC_SUCCESS;

cleanup_enc:
	close(in_fd);
	close(out_fd);
	free(plain_buf);
	free(cipher_buf);
	return res;
}

/* Decrypt file in normal world */
TEEC_Result decrypt_file(struct test_ctx *ctx, const char *input_file,
                         const char *output_file, struct perf_info *perf)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	int in_fd, out_fd;
	uint8_t *cipher_buf, *plain_buf;
	ssize_t bytes_read;
	size_t total_decrypted = 0;
	size_t total_written = 0;
	int is_first = 1;
	struct stat st;
	struct cpu_snapshot cpu_start, cpu_end;
	struct timeval wall_start, wall_end;
	uint64_t original_size;
	
	if (stat(input_file, &st) != 0) {
		printf("Error: Cannot stat file %s\n", input_file);
		return TEEC_ERROR_ITEM_NOT_FOUND;
	}
	
	printf("\n=== DECRYPTION ===\n");
	printf("Input file: %s (%zu bytes)\n", input_file, st.st_size);
	
	/* Allocate buffers */
	cipher_buf = malloc(CHUNK_SIZE + AES_BLOCK_SIZE);
	plain_buf = malloc(CHUNK_SIZE + AES_BLOCK_SIZE);
	if (!cipher_buf || !plain_buf) {
		printf("Error: Cannot allocate buffers\n");
		free(cipher_buf);
		free(plain_buf);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}
	
	/* Open files */
	in_fd = open(input_file, O_RDONLY);
	if (in_fd < 0) {
		printf("Error: Cannot open input file\n");
		free(cipher_buf);
		free(plain_buf);
		return TEEC_ERROR_ITEM_NOT_FOUND;
	}
	
	out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (out_fd < 0) {
		printf("Error: Cannot create output file\n");
		close(in_fd);
		free(cipher_buf);
		free(plain_buf);
		return TEEC_ERROR_GENERIC;
	}
	
	/* Read original file size from header */
	if (read(in_fd, &original_size, sizeof(original_size)) != sizeof(original_size)) {
		printf("Error: Cannot read header\n");
		close(in_fd);
		close(out_fd);
		free(cipher_buf);
		free(plain_buf);
		return TEEC_ERROR_GENERIC;
	}
	
	printf("Original file size: %zu bytes\n", (size_t)original_size);
	
	/* Start timing */
	gettimeofday(&wall_start, NULL);
	take_cpu_snapshot(&cpu_start);
	
	/* Process file in chunks */
	while ((bytes_read = read(in_fd, cipher_buf, CHUNK_SIZE)) > 0) {
		/* Decrypt chunk via TEE */
		memset(&op, 0, sizeof(op));
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						 TEEC_MEMREF_TEMP_OUTPUT,
						 TEEC_VALUE_INPUT,
						 TEEC_VALUE_OUTPUT);
		
		op.params[0].tmpref.buffer = cipher_buf;
		op.params[0].tmpref.size = bytes_read;
		op.params[1].tmpref.buffer = plain_buf;
		op.params[1].tmpref.size = CHUNK_SIZE + AES_BLOCK_SIZE;
		op.params[2].value.a = is_first;
		
		res = TEEC_InvokeCommand(&ctx->sess,
					 TA_SECURE_STORAGE_CMD_DECRYPT_CHUNK,
					 &op, &origin);
		if (res != TEEC_SUCCESS) {
			printf("Error: Decryption failed at offset %zu: 0x%x / %u\n",
			       total_decrypted, res, origin);
			goto cleanup_dec;
		}
		
		size_t decrypted_size = op.params[1].tmpref.size;
		
		/* Write only up to original file size */
		size_t to_write = decrypted_size;
		if (total_written + to_write > original_size) {
			to_write = original_size - total_written;
		}
		
		if (write(out_fd, plain_buf, to_write) != to_write) {
			printf("Error: Write failed\n");
			res = TEEC_ERROR_GENERIC;
			goto cleanup_dec;
		}
		
		total_decrypted += decrypted_size;
		total_written += to_write;
		is_first = 0;
		
		/* Stop if we've written all original data */
		if (total_written >= original_size) {
			break;
		}
		
		/* Progress indicator */
		if (total_written % (256 * 1024) < to_write || total_written >= original_size) {
			printf("  Progress: %zu/%zu bytes (%.1f%%)\n", 
			       total_written, (size_t)original_size,
			       (total_written * 100.0) / original_size);
		}
	}
	
	/* End timing */
	gettimeofday(&wall_end, NULL);
	take_cpu_snapshot(&cpu_end);
	
	perf->host_dec_time_sec = (wall_end.tv_sec - wall_start.tv_sec) +
	                          (wall_end.tv_usec - wall_start.tv_usec) / 1000000.0;
	perf->cpu_usage_dec = calculate_cpu_usage(&cpu_start, &cpu_end);
	
	printf("✓ Decryption complete: %zu bytes written\n", total_written);
	
	res = TEEC_SUCCESS;

cleanup_dec:
	close(in_fd);
	close(out_fd);
	free(cipher_buf);
	free(plain_buf);
	return res;
}

/* Get final timing from TEE */
TEEC_Result get_timing_info(struct test_ctx *ctx, struct perf_info *perf)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT,
					 TEEC_VALUE_OUTPUT,
					 TEEC_VALUE_OUTPUT,
					 TEEC_NONE);
	
	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_SECURE_STORAGE_CMD_FINALIZE,
				 &op, &origin);
	if (res != TEEC_SUCCESS) {
		printf("Error: Finalize failed: 0x%x / %u\n", res, origin);
		return res;
	}
	
	perf->encryption_time_ms = op.params[0].value.a;
	perf->decryption_time_ms = op.params[1].value.a;
	
	return TEEC_SUCCESS;
}

/* Generate test file */
int generate_test_file(const char *filename, size_t size_mb)
{
	int fd;
	size_t total_written = 0;
	size_t chunk_size = 256 * 1024; // 256KB chunks
	uint8_t *buffer;
	size_t target_size = size_mb * 1024 * 1024;
	
	printf("Generating test file: %s (%.2f MB)...\n", 
	       filename, size_mb);
	
	buffer = malloc(chunk_size);
	if (!buffer) {
		printf("Error: Cannot allocate buffer\n");
		return -1;
	}
	
	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		printf("Error: Cannot create file\n");
		free(buffer);
		return -1;
	}
	
	/* Fill with pseudo-random pattern */
	for (size_t i = 0; i < chunk_size; i++)
		buffer[i] = (uint8_t)(i ^ (i >> 8));
	
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

void print_performance_summary(struct perf_info *perf)
{
	printf("\n=======================================================\n");
	printf("  PERFORMANCE SUMMARY\n");
	printf("=======================================================\n");
	printf("  File size: %.2f MB (%zu bytes)\n", 
	       perf->file_size / (1024.0 * 1024.0), perf->file_size);
	
	printf("\n  Encryption:\n");
	printf("    TEE time: %u ms (%.3f seconds)\n", 
	       perf->encryption_time_ms, perf->encryption_time_ms / 1000.0);
	printf("    Host total time: %.3f seconds\n", perf->host_enc_time_sec);
	printf("    Throughput: %.2f MB/s\n", 
	       (perf->file_size / (1024.0 * 1024.0)) / perf->host_enc_time_sec);
	printf("    CPU usage: %.1f%%\n", perf->cpu_usage_enc);
	
	printf("\n  Decryption:\n");
	printf("    TEE time: %u ms (%.3f seconds)\n", 
	       perf->decryption_time_ms, perf->decryption_time_ms / 1000.0);
	printf("    Host total time: %.3f seconds\n", perf->host_dec_time_sec);
	printf("    Throughput: %.2f MB/s\n", 
	       (perf->file_size / (1024.0 * 1024.0)) / perf->host_dec_time_sec);
	printf("    CPU usage: %.1f%%\n", perf->cpu_usage_dec);
	
	if (perf->decryption_time_ms > 0 && perf->encryption_time_ms > 0) {
		double ratio = (double)perf->encryption_time_ms / perf->decryption_time_ms;
		printf("\n  Encryption/Decryption ratio: %.2fx ", ratio);
		if (ratio > 1.0)
			printf("(encryption %.2fx slower)\n", ratio);
		else
			printf("(decryption %.2fx slower)\n", 1.0/ratio);
	}
	printf("=======================================================\n");
}

int main(int argc, char *argv[])
{
	struct test_ctx ctx;
	const char *input_file;
	const char *encrypted_file = "/tmp/encrypted.bin";
	const char *decrypted_file = "/tmp/decrypted.bin";
	struct perf_info perf = {0};
	TEEC_Result res;
	int use_generated = 0;
	
	printf("=======================================================\n");
	printf("  OP-TEE File Encryption/Decryption Test\n");
	printf("  Keys stored in Secure World\n");
	printf("  Files processed in Normal World (16KB chunks)\n");
	printf("=======================================================\n\n");
	
	/* Check for input file */
	if (argc > 1) {
		input_file = argv[1];
		printf("Using provided file: %s\n", input_file);
	} else {
		input_file = "/tmp/test_input.bin";
		use_generated = 1;
		printf("Generating 1MB test file...\n");
		if (generate_test_file(input_file, 1) != 0) {
			printf("Failed to generate test file\n");
			return 1;
		}
	}
	
	printf("\nPreparing TEE session...\n");
	prepare_tee_session(&ctx);
	printf("✓ Session established\n");
	
	/* Test 1: Encrypt file */
	printf("\n=== TEST 1: Encrypt file ===\n");
	res = encrypt_file(&ctx, input_file, encrypted_file, &perf);
	if (res != TEEC_SUCCESS) {
		printf("✗ TEST 1 FAILED\n");
		goto cleanup;
	}
	printf("✓ TEST 1 PASSED\n");
	
	/* Test 2: Decrypt file */
	printf("\n=== TEST 2: Decrypt file ===\n");
	res = decrypt_file(&ctx, encrypted_file, decrypted_file, &perf);
	if (res != TEEC_SUCCESS) {
		printf("✗ TEST 2 FAILED\n");
		goto cleanup;
	}
	printf("✓ TEST 2 PASSED\n");
	
	/* Get TEE timing */
	get_timing_info(&ctx, &perf);
	
	/* Test 3: Verify integrity */
	printf("\n=== TEST 3: Verify integrity ===\n");
	char cmd[512];
	snprintf(cmd, sizeof(cmd), "cmp -s %s %s", input_file, decrypted_file);
	if (system(cmd) == 0) {
		printf("✓ Files match - integrity verified\n");
		printf("✓ TEST 3 PASSED\n");
	} else {
		printf("✗ Files differ - integrity check failed\n");
		printf("✗ TEST 3 FAILED\n");
	}
	
	/* Print performance summary */
	print_performance_summary(&perf);
	
	printf("\n✓ ALL TESTS PASSED\n");

cleanup:
	printf("\nCleaning up...\n");
	terminate_tee_session(&ctx);
	
	if (use_generated) {
		unlink(input_file);
		printf("✓ Temporary files removed\n");
	}
	unlink(encrypted_file);
	unlink(decrypted_file);
	
	printf("✓ Session closed\n");
	
	return (res == TEEC_SUCCESS) ? 0 : 1;
}