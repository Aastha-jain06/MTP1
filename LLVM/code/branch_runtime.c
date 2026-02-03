#include "branch_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global counters
static volatile uint64_t cond_branch_count = 0;
static volatile uint64_t uncond_branch_count = 0;
static volatile uint64_t loop_header_count = 0;
static volatile uint64_t direct_call_count = 0;
static volatile uint64_t return_count = 0;

// Thread-safe increment (for single-threaded we can keep it simple)
void increment_cond_branch(void) {
    cond_branch_count++;
}

void increment_uncond_branch(void) {
    uncond_branch_count++;
}

void increment_loop_header(void) {
    loop_header_count++;
}

void increment_direct_call(void) {
    direct_call_count++;
}

void increment_return(void) {
    return_count++;
}

void init_branch_stats(void) {
    reset_branch_stats();
}

void reset_branch_stats(void) {
    cond_branch_count = 0;
    uncond_branch_count = 0;
    loop_header_count = 0;
    direct_call_count = 0;
    return_count = 0;
}

void print_branch_stats(void) {
    printf("\n");
    printf("================================\n");
    printf("   Branch Statistics Report     \n");
    printf("================================\n");
    printf("# Conditional Branches:   %llu\n", (unsigned long long)cond_branch_count);
    printf("# Unconditional Branches: %llu\n", (unsigned long long)uncond_branch_count);
    printf("# Loop Headers:           %llu\n", (unsigned long long)loop_header_count);
    printf("# Direct Calls:           %llu\n", (unsigned long long)direct_call_count);
    printf("# Returns/Exits:          %llu\n", (unsigned long long)return_count);
    printf("================================\n");
    printf("\n");
    fflush(stdout);
}

uint64_t get_cond_branch_count(void) {
    return cond_branch_count;
}

uint64_t get_uncond_branch_count(void) {
    return uncond_branch_count;
}

uint64_t get_loop_header_count(void) {
    return loop_header_count;
}

uint64_t get_direct_call_count(void) {
    return direct_call_count;
}

uint64_t get_return_count(void) {
    return return_count;
}