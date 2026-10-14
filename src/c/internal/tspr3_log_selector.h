#ifndef TSPR3_LOG_SELECTOR_H
#define TSPR3_LOG_SELECTOR_H
#include "tspr3_profile.h"
typedef struct {
    tspr3_tri_mode selected;
    tspr3_profile profile;
    double predicted_ms[3], conservative_ms[3];
} tspr3_log_choice;
/* heap_limit bounds requested auxiliary heap, not RSS or profile stack.
 * Model is calibrated for the documented 2M distributions. */
tspr3_status tspr3_log_choose(const int32_t *input,size_t n,size_t heap_limit,tspr3_log_choice *choice);
tspr3_status tspr3_log_sort(const int32_t *input,size_t n,int32_t *output,size_t heap_limit,
    tspr3_log_choice *choice,tspr3_tri_stats *stats);
#endif
