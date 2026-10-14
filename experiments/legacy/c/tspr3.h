#ifndef TSPR3_H
#define TSPR3_H
#include <stddef.h>
#include <stdint.h>

typedef struct { int64_t sum; size_t original_index; } tspr3_record;
typedef enum { TSPR3_OK, TSPR3_INVALID, TSPR3_NO_MEMORY } tspr3_status;

/* Caller owns output. NULL is accepted only when n == 0.
 * SUM needs ceil(n/3) records; FULL needs n int32_t values.
 * Input and output must not overlap. */
tspr3_status tspr3_sum(const int32_t *input, size_t n, tspr3_record *output);
tspr3_status tspr3_full(const int32_t *input, size_t n, int32_t *output);
typedef struct { int32_t value; size_t count; } tspr3_frequency;
/* Returns a sorted frequency index. Caller releases *index with free().
 * index and distinct_count are required; on error they are NULL and 0.
 * Empty input returns NULL and 0 successfully. */
tspr3_status tspr3_frequency_index(const int32_t *input, size_t n,
    tspr3_frequency **index, size_t *distinct_count);
/* Output has n elements and must not overlap input. distinct_count is optional. */
tspr3_status tspr3_freq(const int32_t *input, size_t n, int32_t *output,
    size_t *distinct_count);
typedef enum { TSPR3_METHOD_FULL, TSPR3_METHOD_FREQ,
    TSPR3_METHOD_ASCENDING, TSPR3_METHOD_DESCENDING,
    TSPR3_METHOD_SEPARATED_BLOCKS, TSPR3_METHOD_RADIX,
    TSPR3_METHOD_DENSE, TSPR3_METHOD_RUNS,
    TSPR3_METHOD_DOMINANT } tspr3_method;
/* Signed 32-bit radix sort: at most four stable byte passes, O(n). */
tspr3_status tspr3_radix(const int32_t *input, size_t n, int32_t *output);
/* Bounded exact-count policy; maximum distinct keys AUTO will count. */
size_t tspr3_auto_frequency_budget(size_t n);
/* Structural checks, bounded exact counting and radix fallback.
 * A sample only suggests the dominant-value path; exact counting confirms it.
 * Optional method reports the selected path. */
tspr3_status tspr3_auto(const int32_t *input, size_t n, int32_t *output,
    tspr3_method *method);
#endif
