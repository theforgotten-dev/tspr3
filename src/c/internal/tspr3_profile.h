#ifndef TSPR3_PROFILE_H
#define TSPR3_PROFILE_H
#include "tspr3_tri.h"
#define TSPR3_FEATURES 9
typedef struct {
    double features[TSPR3_FEATURES];
    double mean, scale;
    size_t sampled_triads;
} tspr3_profile;
/* Bounded stratified sample. Features never replace exact sorting. */
tspr3_status tspr3_log_profile(const int32_t *input,size_t n,tspr3_profile *profile);
#endif
