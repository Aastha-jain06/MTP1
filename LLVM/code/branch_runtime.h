#ifndef BRANCH_RUNTIME_H
#define BRANCH_RUNTIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Counter increment functions (called by instrumented code)
void increment_cond_branch(void);
void increment_uncond_branch(void);
void increment_loop_header(void);
void increment_direct_call(void);
void increment_return(void);

// Statistics functions
void print_branch_stats(void);
void reset_branch_stats(void);
void init_branch_stats(void);

// Get individual counts
uint64_t get_cond_branch_count(void);
uint64_t get_uncond_branch_count(void);
uint64_t get_loop_header_count(void);
uint64_t get_direct_call_count(void);
uint64_t get_return_count(void);

#ifdef __cplusplus
}
#endif

#endif // BRANCH_RUNTIME_H