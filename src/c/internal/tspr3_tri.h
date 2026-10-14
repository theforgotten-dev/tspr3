#ifndef TSPR3_TRI_H
#define TSPR3_TRI_H
#include "tspr3.h"

typedef enum { TSPR3_TRI_BASE, TSPR3_TRI_COUNTS, TSPR3_TRI_BIN } tspr3_tri_mode;
typedef struct {
    uint64_t comparisons;       /* Logical comparisons between numeric keys. */
    uint64_t data_bytes_written;/* Writes to value/count buffers, not metadata. */
    uint64_t equal_merges;
    uint64_t range_joins;
    uint64_t prefix_tests;
    uint64_t prefix_joins;
    size_t peak_aux_bytes;      /* Peak requested heap, including compact arenas;
                                * excludes input and caller's expanded output. */
    size_t triads;
    size_t distinct_values;     /* COUNTS/BIN only; BASE does not compute counts. */
    size_t prefix_nodes;
    size_t crossing_root_blocks;
} tspr3_tri_stats;

/* All modes form consecutive ternas. BIN indexes whole blocks by their
 * common prefix; crossing blocks remain at an ancestor. No radix/hash route.
 * Caller reserves n output values, disjoint from input. stats may be NULL. */
tspr3_status tspr3_tri_sort(const int32_t *input, size_t n,
    tspr3_tri_mode mode, int32_t *output, tspr3_tri_stats *stats);

/* COUNTS/BIN return sorted (value,count). Caller free()s *index.
 * Underlying arena capacity is n records; *distinct_count is its used length.
 * Required output arguments are reset to NULL/0 on error. */
tspr3_status tspr3_tri_index(const int32_t *input, size_t n,
    tspr3_tri_mode mode, tspr3_frequency **index, size_t *distinct_count,
    tspr3_tri_stats *stats);
/* Conservative bound on requested heap; SIZE_MAX signals overflow/invalid mode. */
size_t tspr3_tri_memory_bound(size_t n,tspr3_tri_mode mode);
#endif
