#ifndef TSPR3_GENERIC_TRI_H
#define TSPR3_GENERIC_TRI_H
#include "tspr3_tri.h"

typedef int (*tspr3_generic_compare_fn)(const void *a, const void *b, void *ctx);
typedef uint64_t (*tspr3_generic_key_fn)(const void *element, void *ctx);

/* Stable generic sort: preserves every complete record, including equal keys.
 * Sorts references to the original objects; compare/key callbacks see those
 * original addresses, never misaligned temporary byte copies. Input and output
 * must not overlap; caller provides n*elem_size output bytes.
 * COUNTS retains the merge strategy but never collapses records by key.
 * BIN requires an unsigned 64-bit order-preserving key: compare(a,b)<0 must
 * imply key(a)<=key(b); equal comparator keys must have equal numeric keys.
 * Collisions are allowed; incompatible numeric ordering is a contract violation.
 * stats.distinct_values is zero (distinct comparator keys are not counted).
 * n=0 permits NULL data/callbacks; elem_size must still be positive. */
tspr3_status tspr3_generic_tri_sort(const void *input, size_t n, size_t elem_size,
    tspr3_tri_mode mode, void *output,
    tspr3_generic_compare_fn compare, tspr3_generic_key_fn key_fn, void *ctx,
    tspr3_tri_stats *stats);

/* Requested auxiliary heap bound; SIZE_MAX means invalid size/mode or overflow.
 * Includes reference arrays and internal arenas, excludes caller input/output. */
size_t tspr3_generic_tri_memory_bound(size_t n, size_t elem_size, tspr3_tri_mode mode);
#endif
