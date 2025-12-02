#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

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

/* Storage statistics - FILESYSTEM level */
struct storage_stats {
	unsigned long total_bytes;
	unsigned long free_bytes;
	unsigned long available_bytes;
	double usage_percent;
};

/* ⭐ SECURE STORAGE statistics - TEE INTERNAL level */
struct secure_storage_info {
	uint32_t total_objects;        // Total objects in secure storage
	uint64_t total_size_bytes;     // Total size of all objects
	uint64_t this_object_size;     // Size of current test object
	uint32_t storage_id;           // TEE storage ID (TEE_STORAGE_PRIVATE)
};

struct ram_stats {
	unsigned long total_kb;
	unsigned long free_kb;
	unsigned long available_kb;
	unsigned long buffers_kb;
	unsigned long cached_kb;
	double usage_percent;
};

/**
 * ⭐ Get TRUE secure storage info from TEE
 * This queries the ACTUAL internal TEE secure storage, not the filesystem!
 */
TEEC_Result get_secure_storage_info(struct test_ctx *ctx, 
                                     struct secure_storage_info *info)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT,  // object count, storage_id
					 TEEC_VALUE_OUTPUT,  // total size (low, high)
					 TEEC_VALUE_OUTPUT,  // test object size (low, high)
					 TEEC_NONE);

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_SECURE_STORAGE_CMD_GET_STORAGE_INFO,
				 &op, &origin);

	if (res == TEEC_SUCCESS) {
		info->total_objects = op.params[0].value.a;
		info->storage_id = op.params[0].value.b;
		
		// Reconstruct 64-bit values from two 32-bit values
		info->total_size_bytes = ((uint64_t)op.params[1].value.b << 32) | 
		                         op.params[1].value.a;
		info->this_object_size = ((uint64_t)op.params[2].value.b << 32) | 
		                         op.params[2].value.a;
	} else {
		printf("Warning: Failed to get secure storage info: 0x%x\n", res);
	}

	return res;
}

/**
 * Print secure storage information
 */
void print_secure_storage_info(const char *label, struct secure_storage_info *info)
{
	const char *storage_name = "UNKNOWN";
	
	switch (info->storage_id) {
	case 0x00000001: storage_name = "TEE_STORAGE_PRIVATE"; break;
	case 0x80000000: storage_name = "TEE_STORAGE_REE_FS"; break;
	default: storage_name = "UNKNOWN"; break;
	}
	
	printf("\n%s:\n", label);
	printf("  Storage Type:    %s (0x%08x)\n", storage_name, info->storage_id);
	printf("  Total Objects:   %u\n", info->total_objects);
	printf("  Total Size:      %.2f MB (%lu bytes)\n", 
	       info->total_size_bytes / (1024.0 * 1024.0),
	       info->total_size_bytes);
	if (info->this_object_size > 0) {
		printf("  Test Object:     %.2f MB (%lu bytes)\n",
		       info->this_object_size / (1024.0 * 1024.0),
		       info->this_object_size);
	}
}

/**
 * Get filesystem storage statistics (for comparison)
 * ⚠️ This is FILESYSTEM level, not secure storage internal!
 */
int get_storage_stats(struct storage_stats *stats, const char *path)
{
	struct statvfs vfs;
	
	if (statvfs(path, &vfs) != 0) {
		printf("Warning: Cannot get storage stats for %s\n", path);
		return -1;
	}
	
	stats->total_bytes = vfs.f_blocks * vfs.f_frsize;
	stats->free_bytes = vfs.f_bfree * vfs.f_frsize;
	stats->available_bytes = vfs.f_bavail * vfs.f_frsize;
	stats->usage_percent = 100.0 * (1.0 - ((double)stats->free_bytes / stats->total_bytes));
	
	return 0;
}

/**
 * Print storage statistics
 */
void print_storage_stats(const char *label, struct storage_stats *stats)
{
	printf("\n%s:\n", label);
	printf("  Total Space:     %.2f MB\n", stats->total_bytes / (1024.0 * 1024.0));
	printf("  Used Space:      %.2f MB (%.1f%%)\n", 
	       (stats->total_bytes - stats->free_bytes) / (1024.0 * 1024.0),
	       stats->usage_percent);
	printf("  Free Space:      %.2f MB\n", stats->free_bytes / (1024.0 * 1024.0));
	printf("  Available Space: %.2f MB\n", stats->available_bytes / (1024.0 * 1024.0));
}

/**
 * Get RAM statistics from /proc/meminfo
 */
int get_ram_stats(struct ram_stats *stats)
{
	FILE *fp;
	char line[256];
	unsigned long mem_total = 0, mem_free = 0, mem_available = 0;
	unsigned long buffers = 0, cached = 0;
	
	fp = fopen("/proc/meminfo", "r");
	if (!fp) {
		printf("Warning: Cannot open /proc/meminfo\n");
		return -1;
	}
	
	while (fgets(line, sizeof(line), fp)) {
		if (sscanf(line, "MemTotal: %lu kB", &mem_total) == 1)
			continue;
		if (sscanf(line, "MemFree: %lu kB", &mem_free) == 1)
			continue;
		if (sscanf(line, "MemAvailable: %lu kB", &mem_available) == 1)
			continue;
		if (sscanf(line, "Buffers: %lu kB", &buffers) == 1)
			continue;
		if (sscanf(line, "Cached: %lu kB", &cached) == 1)
			continue;
	}
	
	fclose(fp);
	
	stats->total_kb = mem_total;
	stats->free_kb = mem_free;
	stats->available_kb = mem_available;
	stats->buffers_kb = buffers;
	stats->cached_kb = cached;
	stats->usage_percent = 100.0 * (1.0 - ((double)mem_available / mem_total));
	
	return 0;
}

/**
 * Print RAM statistics
 */
void print_ram_stats(const char *label, struct ram_stats *stats)
{
	printf("\n%s:\n", label);
	printf("  Total RAM:       %.2f MB\n", stats->total_kb / 1024.0);
	printf("  Used RAM:        %.2f MB (%.1f%%)\n", 
	       (stats->total_kb - stats->available_kb) / 1024.0,
	       stats->usage_percent);
	printf("  Free RAM:        %.2f MB\n", stats->free_kb / 1024.0);
	printf("  Available RAM:   %.2f MB\n", stats->available_kb / 1024.0);
	printf("  Buffers:         %.2f MB\n", stats->buffers_kb / 1024.0);
	printf("  Cached:          %.2f MB\n", stats->cached_kb / 1024.0);
}

/**
 * ⭐ Print secure storage delta (TRUE internal storage)
 */
void print_secure_storage_delta(struct secure_storage_info *before, 
                                 struct secure_storage_info *after)
{
	int64_t delta_objects = (int64_t)after->total_objects - (int64_t)before->total_objects;
	int64_t delta_bytes = (int64_t)after->total_size_bytes - (int64_t)before->total_size_bytes;
	
	printf("\n=== ⭐ SECURE STORAGE Changes (TEE Internal) ===\n");
	printf("  Objects Added:   %+ld (was: %u, now: %u)\n",
	       delta_objects, before->total_objects, after->total_objects);
	printf("  Storage Used:    %+.2f MB (%+ld bytes)\n",
	       delta_bytes / (1024.0 * 1024.0), delta_bytes);
	printf("  Before Total:    %.2f MB\n", before->total_size_bytes / (1024.0 * 1024.0));
	printf("  After Total:     %.2f MB\n", after->total_size_bytes / (1024.0 * 1024.0));
}

/**
 * Print filesystem storage delta (for comparison)
 */
void print_storage_delta(struct storage_stats *before, struct storage_stats *after)
{
	long delta_bytes = (long)before->free_bytes - (long)after->free_bytes;
	
	printf("\n=== Filesystem Changes (/data/tee) ===\n");
	printf("  Space Used:      %.2f MB\n", delta_bytes / (1024.0 * 1024.0));
	printf("  Usage Change:    %.1f%% → %.1f%% (Δ %.1f%%)\n",
	       before->usage_percent, after->usage_percent,
	       after->usage_percent - before->usage_percent);
}

void print_ram_delta(struct ram_stats *before, struct ram_stats *after)
{
	long delta_available = (long)before->available_kb - (long)after->available_kb;
	
	printf("\n=== RAM Changes ===\n");
	printf("  RAM Consumed:    %.2f MB\n", delta_available / 1024.0);
	printf("  Usage Change:    %.1f%% → %.1f%% (Δ %.1f%%)\n",
	       before->usage_percent, after->usage_percent,
	       after->usage_percent - before->usage_percent);
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

	if (stat(filename, &st) != 0) {
		printf("Error: Cannot stat file %s\n", filename);
		return TEEC_ERROR_ITEM_NOT_FOUND;
	}

	printf("  Streaming file: %s (%zu bytes = %.2f MB)\n", 
	       filename, st.st_size, st.st_size / (1024.0 * 1024.0));

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("Error: Cannot open file %s\n", filename);
		return TEEC_ERROR_ITEM_NOT_FOUND;
	}

	while ((bytes_read = read(fd, chunk_buffer, CHUNK_SIZE)) > 0) {
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
			close(fd);
			return res;
		}

		total_written += bytes_read;
		is_first = 0;

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

TEEC_Result read_and_verify_size(struct test_ctx *ctx, char *obj_id, 
                                  size_t expected_size)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(obj_id);
	char small_buffer[1];

	printf("  Verifying object size...\n");

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

int generate_test_file(const char *filename, size_t size_mb)
{
	int fd;
	size_t total_written = 0;
	size_t chunk_size = 1024 * 1024;
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
	const char *tee_storage_path = "/data/tee";
	struct stat st;
	TEEC_Result res;
	int use_generated_file = 0;
	
	/* ⭐ TRUE Secure Storage stats (TEE internal) */
	struct secure_storage_info sec_storage_before, sec_storage_after, sec_storage_final;
	
	/* Filesystem stats (for comparison) */
	struct storage_stats storage_before, storage_after;
	struct ram_stats ram_before, ram_after;

	printf("=======================================================\n");
	printf("  OP-TEE Secure Storage Test with TRUE Internal Monitoring\n");
	printf("  ⭐ Monitoring ACTUAL TEE secure storage, not just filesystem\n");
	printf("=======================================================\n\n");

	if (argc > 1) {
		test_file = argv[1];
		printf("Using provided file: %s\n", test_file);
		if (stat(test_file, &st) != 0) {
			printf("Error: File %s not found\n", test_file);
			return 1;
		}
	} else {
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

	printf("\nTest file size: %zu bytes (%.2f MB)\n\n", 
	       st.st_size, st.st_size / (1024.0 * 1024.0));

	printf("Preparing TEE session...\n");
	prepare_tee_session(&ctx);
	printf("✓ Session established\n\n");

	printf("Cleaning up any existing object...\n");
	delete_secure_object(&ctx, obj_id);

	/* ====== CAPTURE INITIAL STATE ====== */
	printf("\n=======================================================\n");
	printf("  📊 INITIAL SYSTEM STATE (BEFORE WRITE)\n");
	printf("=======================================================\n");
	
	/* ⭐ Get TRUE secure storage info */
	if (get_secure_storage_info(&ctx, &sec_storage_before) == TEEC_SUCCESS) {
		print_secure_storage_info("⭐ SECURE STORAGE (TEE Internal)", &sec_storage_before);
	}
	
	/* Get filesystem info for comparison */
	if (get_storage_stats(&storage_before, tee_storage_path) == 0) {
		print_storage_stats("Filesystem (/data/tee) - For Comparison", &storage_before);
	}
	
	if (get_ram_stats(&ram_before) == 0) {
		print_ram_stats("System RAM", &ram_before);
	}

	/* ====== WRITE TEST ====== */
	printf("\n=======================================================\n");
	printf("  💾 TEST 1: Write file to secure storage (streaming)\n");
	printf("=======================================================\n");
	res = write_file_to_secure_storage_streaming(&ctx, obj_id, test_file);
	if (res != TEEC_SUCCESS) {
		printf("\n✗ FAILED to write file to secure storage\n");
		goto cleanup;
	}
	printf("✓ TEST 1 PASSED\n");

	/* ====== CAPTURE STATE AFTER WRITE ====== */
	printf("\n=======================================================\n");
	printf("  📊 SYSTEM STATE AFTER WRITE\n");
	printf("=======================================================\n");
	
	/* ⭐ Get TRUE secure storage info after write */
	if (get_secure_storage_info(&ctx, &sec_storage_after) == TEEC_SUCCESS) {
		print_secure_storage_info("⭐ SECURE STORAGE (TEE Internal)", &sec_storage_after);
	}
	
	if (get_storage_stats(&storage_after, tee_storage_path) == 0) {
		print_storage_stats("Filesystem (/data/tee) - For Comparison", &storage_after);
	}
	
	if (get_ram_stats(&ram_after) == 0) {
		print_ram_stats("System RAM", &ram_after);
	}

	/* ====== SHOW DELTAS ====== */
	printf("\n=======================================================\n");
	printf("  📈 RESOURCE CONSUMPTION ANALYSIS\n");
	printf("=======================================================\n");
	
	/* ⭐ Show TRUE secure storage delta */
	print_secure_storage_delta(&sec_storage_before, &sec_storage_after);
	
	/* Show filesystem delta for comparison */
	print_storage_delta(&storage_before, &storage_after);
	print_ram_delta(&ram_before, &ram_after);

	/* ====== VERIFY TEST ====== */
	printf("\n=======================================================\n");
	printf("  🔍 TEST 2: Verify stored object\n");
	printf("=======================================================\n");
	res = read_and_verify_size(&ctx, obj_id, st.st_size);
	if (res != TEEC_SUCCESS) {
		printf("✗ TEST 2 FAILED\n");
		goto cleanup;
	}
	printf("✓ TEST 2 PASSED\n");

	/* ====== DELETE TEST ====== */
	printf("\n=======================================================\n");
	printf("  🗑️  TEST 3: Delete stored object\n");
	printf("=======================================================\n");
	res = delete_secure_object(&ctx, obj_id);
	if (res != TEEC_SUCCESS) {
		printf("✗ TEST 3 FAILED\n");
		goto cleanup;
	}
	printf("✓ Object deleted successfully\n");
	printf("✓ TEST 3 PASSED\n");

	/* ====== FINAL STATE (AFTER CLEANUP) ====== */
	printf("\n=======================================================\n");
	printf("  📊 FINAL SYSTEM STATE (AFTER CLEANUP)\n");
	printf("=======================================================\n");
	
	/* ⭐ Get final secure storage state */
	if (get_secure_storage_info(&ctx, &sec_storage_final) == TEEC_SUCCESS) {
		print_secure_storage_info("⭐ SECURE STORAGE (TEE Internal)", &sec_storage_final);
		
		printf("\n=== ⭐ Storage Recovery (TEE Internal) ===\n");
		int64_t recovered = (int64_t)sec_storage_after.total_size_bytes - 
		                    (int64_t)sec_storage_final.total_size_bytes;
		printf("  Space Recovered: %.2f MB (%ld bytes)\n", 
		       recovered / (1024.0 * 1024.0), recovered);
		printf("  Objects Removed: %d\n", 
		       sec_storage_after.total_objects - sec_storage_final.total_objects);
	}

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