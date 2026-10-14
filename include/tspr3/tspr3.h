#ifndef TSPR3_PUBLIC_H
#define TSPR3_PUBLIC_H
#include <stddef.h>
#include <stdint.h>
#define TSPR3_VERSION "0.1.0"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    TSPR3_SUCCESS=0, TSPR3_INVALID_ARGUMENT, TSPR3_ALLOCATION_FAILURE, TSPR3_HEAP_LIMIT
} tspr3_result;
typedef enum {
    TSPR3_ALGORITHM_AUTO=0, TSPR3_ALGORITHM_A, TSPR3_ALGORITHM_B, TSPR3_ALGORITHM_C
} tspr3_algorithm;
typedef struct {
    tspr3_algorithm algorithm;
    size_t auxiliary_heap_limit;
} tspr3_options;
typedef struct { int32_t value; size_t count; } tspr3_frequency_i32;
typedef struct {
    tspr3_algorithm selected;
    double predicted_ms[3];
    double conservative_ms[3];
    uint64_t comparisons;
    uint64_t data_bytes_written;
    uint64_t equal_merges;
    uint64_t range_joins;
    uint64_t prefix_tests;
    uint64_t prefix_joins;
    size_t peak_aux_bytes;
    size_t triads;
    size_t distinct_values; /* Zero for A: that route does not count distinct keys. */
    size_t prefix_nodes;
    size_t crossing_root_blocks;
} tspr3_report;
const char *tspr3_version(void);
const char *tspr3_result_message(tspr3_result result);
/* Defaults: AUTO, 256 MiB requested auxiliary heap. NULL is a no-op. */
void tspr3_options_init(tspr3_options *options);
/* input and output must not overlap. Caller provides n output elements.
 * NULL data pointers are accepted when n==0. NULL options use defaults;
 * report is optional. Caller owns all buffers; input remains unchanged. */
tspr3_result tspr3_sort_i32(const int32_t *input,size_t n,int32_t *output,
    const tspr3_options *options,tspr3_report *report);
/* Caller provides n frequency records; only *distinct_count records are used.
 * B/C supported; AUTO selects B for compact output; A is invalid.
 * Output may not overlap input. No allocator crosses the library boundary. */
tspr3_result tspr3_counts_i32(const int32_t *input,size_t n,tspr3_frequency_i32 *output,
    size_t *distinct_count,const tspr3_options *options,tspr3_report *report);
#ifdef __cplusplus
}
#endif
#endif
