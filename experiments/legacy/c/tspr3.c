#include "tspr3.h"
#include <stdlib.h>
#include <string.h>

static void sort_ternas(int32_t *output, size_t n) {
    size_t start;
    for (start = 0; start < n;) {
        size_t end = start + (n - start < 3 ? n - start : 3), i;
        for (i = start + 1; i < end; ++i) {
            int32_t value = output[i];
            size_t j = i;
            while (j > start && output[j - 1] > value) {
                output[j] = output[j - 1]; --j;
            }
            output[j] = value;
        }
        start = end;
    }
}
static int separated_ternas(const int32_t *input, size_t n) {
    size_t start = 0;
    int32_t previous_max = 0;
    while (start < n) {
        size_t end = start + (n - start < 3 ? n - start : 3), j;
        int32_t low = input[start], high = low;
        for (j = start + 1; j < end; ++j) {
            if (input[j] < low) low = input[j];
            if (input[j] > high) high = input[j];
        }
        if (start && previous_max > low) return 0;
        previous_max = high; start = end;
    }
    return 1;
}

/* Open addressing, power-of-two capacity, at most 50% occupancy.
 * count == 0 denotes an empty slot; every int32_t key is valid. */
static size_t frequency_hash(int32_t value) {
    uint32_t x = (uint32_t)value;
    x ^= x >> 16; x *= UINT32_C(0x7feb352d);
    x ^= x >> 15; x *= UINT32_C(0x846ca68b);
    x ^= x >> 16;
    return (size_t)x;
}
static size_t frequency_slot(const tspr3_frequency *table, size_t capacity, int32_t value) {
    size_t pos = frequency_hash(value) & (capacity - 1);
    while (table[pos].count && table[pos].value != value)
        pos = (pos + 1) & (capacity - 1);
    return pos;
}
static int frequency_compare(const void *a, const void *b) {
    const tspr3_frequency *x = a, *y = b;
    return (x->value > y->value) - (x->value < y->value);
}
static tspr3_status frequency_index_limited(const int32_t *input, size_t n,
    tspr3_frequency **index, size_t *distinct_count, size_t limit, int *exceeded) {
    size_t capacity = 16, used = 0, i;
    tspr3_frequency *table, *result;
    if (!index || !distinct_count) return TSPR3_INVALID;
    *index = NULL; *distinct_count = 0;
    *exceeded = 0;
    if (!n) return TSPR3_OK;
    if (!input || n > SIZE_MAX / sizeof(*input)) return TSPR3_INVALID;
    table = calloc(capacity, sizeof(*table));
    if (!table) return TSPR3_NO_MEMORY;
    for (i = 0; i < n; ++i) {
        size_t pos;
        if (limit == SIZE_MAX) pos = frequency_slot(table, capacity, input[i]);
        else {
            size_t probes = 0;
            pos = frequency_hash(input[i]) & (capacity-1);
            while (table[pos].count && table[pos].value != input[i]) {
                if (++probes == 64) { *exceeded=1;free(table);return TSPR3_OK; }
                pos=(pos+1)&(capacity-1);
            }
        }
        if (!table[pos].count) {
            if (used == limit) {
                *exceeded = 1; free(table); return TSPR3_OK;
            }
            if (used == capacity / 2) {
                size_t j, new_capacity;
                tspr3_frequency *grown;
                if (capacity > SIZE_MAX / 2 / sizeof(*table)) {
                    free(table); return TSPR3_NO_MEMORY;
                }
                new_capacity = capacity * 2;
                grown = calloc(new_capacity, sizeof(*grown));
                if (!grown) { free(table); return TSPR3_NO_MEMORY; }
                for (j = 0; j < capacity; ++j) if (table[j].count)
                    grown[frequency_slot(grown, new_capacity, table[j].value)] = table[j];
                free(table); table = grown; capacity = new_capacity;
                pos = frequency_slot(table, capacity, input[i]);
            }
            table[pos].value = input[i]; ++used;
        }
        ++table[pos].count; /* Each count is bounded by n. */
    }
    result = malloc(used * sizeof(*result));
    if (!result) { free(table); return TSPR3_NO_MEMORY; }
    used = 0;
    for (i = 0; i < capacity; ++i) if (table[i].count) result[used++] = table[i];
    free(table);
    qsort(result, used, sizeof(*result), frequency_compare);
    *index = result; *distinct_count = used;
    return TSPR3_OK;
}
tspr3_status tspr3_frequency_index(const int32_t *input, size_t n,
    tspr3_frequency **index, size_t *distinct_count) {
    int exceeded;
    return frequency_index_limited(input, n, index, distinct_count, SIZE_MAX, &exceeded);
}
static void expand_frequencies(const tspr3_frequency *index, size_t k, int32_t *output) {
    size_t i, j, pos = 0;
    for (i = 0; i < k; ++i)
        for (j = 0; j < index[i].count; ++j) output[pos++] = index[i].value;
}
tspr3_status tspr3_freq(const int32_t *input, size_t n, int32_t *output,
    size_t *distinct_count) {
    tspr3_frequency *index;
    size_t k;
    tspr3_status status;
    if (distinct_count) *distinct_count = 0;
    if (n && !output) return TSPR3_INVALID;
    status = tspr3_frequency_index(input, n, &index, &k);
    if (status != TSPR3_OK) return status;
    expand_frequencies(index, k, output);
    free(index);
    if (distinct_count) *distinct_count = k;
    return TSPR3_OK;
}

size_t tspr3_auto_frequency_budget(size_t n) {
    if (n < 256) return 0;
    /* Bound the rejected hash work independently of large input size. */
    return n / 16 < 4096 ? n / 16 : 4096;
}
static tspr3_status auto_small(const int32_t *input, size_t n, int32_t *output,
    tspr3_method *method) {
    size_t i, k, budget;
    int ascending = 1, descending = 1, exceeded;
    tspr3_frequency *index;
    tspr3_status status;
    if (method) *method = TSPR3_METHOD_FULL;
    if (!n) return TSPR3_OK;
    if (!input || !output || n > SIZE_MAX / sizeof(*output)) return TSPR3_INVALID;
    /* Stop checking order as soon as both monotonic directions are ruled out. */
    for (i = 1; i < n && (ascending || descending); ++i) {
        if (input[i-1] > input[i]) ascending = 0;
        if (input[i-1] < input[i]) descending = 0;
    }
    if (ascending) {
        memcpy(output, input, n * sizeof(*output));
        if (method) *method = TSPR3_METHOD_ASCENDING;
        return TSPR3_OK;
    }
    if (descending) {
        for (i = 0; i < n; ++i) output[i] = input[n-1-i];
        if (method) *method = TSPR3_METHOD_DESCENDING;
        return TSPR3_OK;
    }
    if (separated_ternas(input, n)) {
        memcpy(output, input, n * sizeof(*output));
        sort_ternas(output, n);
        if (method) *method = TSPR3_METHOD_SEPARATED_BLOCKS;
        return TSPR3_OK;
    }
    if (n < 256) return tspr3_full(input, n, output);
    /* Exact counting avoids biased sample estimates of the distinct keys. */
    budget = tspr3_auto_frequency_budget(n);
    if (!budget) return tspr3_full(input, n, output);
    status = frequency_index_limited(input, n, &index, &k, budget, &exceeded);
    if (status == TSPR3_NO_MEMORY || exceeded) return tspr3_full(input, n, output);
    if (status != TSPR3_OK) return status;
    expand_frequencies(index, k, output);
    free(index);
    if (method) *method = TSPR3_METHOD_FREQ;
    return TSPR3_OK;
}

tspr3_status tspr3_radix(const int32_t *input, size_t n, int32_t *output) {
    size_t counts[4][256] = {{0}}, offsets[256], i;
    unsigned int pass;
    int32_t *scratch, *src, *dst;
    if (!n) return TSPR3_OK;
    if (!input || !output || n > SIZE_MAX / sizeof(*output)) return TSPR3_INVALID;
    scratch = malloc(n * sizeof(*scratch));
    if (!scratch) return TSPR3_NO_MEMORY;
    memcpy(output, input, n * sizeof(*output));
    /* Flip the sign bit: signed ascending order becomes unsigned order. */
    for (i = 0; i < n; ++i) {
        uint32_t key = (uint32_t)input[i] ^ UINT32_C(0x80000000);
        for (pass = 0; pass < 4; ++pass) ++counts[pass][(key >> (pass * 8)) & 255];
    }
    src = output; dst = scratch;
    for (pass = 0; pass < 4; ++pass) {
        size_t bucket, total = 0;
        int constant = 0;
        for (bucket = 0; bucket < 256; ++bucket) {
            offsets[bucket] = total; total += counts[pass][bucket];
            if (counts[pass][bucket] == n) constant = 1;
        }
        if (constant) continue;
        for (i = 0; i < n; ++i) {
            uint32_t key = (uint32_t)src[i] ^ UINT32_C(0x80000000);
            bucket = (key >> (pass * 8)) & 255;
            dst[offsets[bucket]++] = src[i];
        }
        { int32_t *tmp = src; src = dst; dst = tmp; }
    }
    if (src != output) memcpy(output, src, n * sizeof(*output));
    free(scratch);
    return TSPR3_OK;
}

/* Profile stores at most 64 ascending runs. More runs select another path. */
static tspr3_status sort_runs(const int32_t *input, size_t n, int32_t *output,
    size_t *boundaries, size_t runs, const unsigned char *reverse) {
    size_t next[65], i;
    int32_t *scratch = malloc(n * sizeof(*scratch)), *src, *dst;
    if (!scratch) return TSPR3_NO_MEMORY;
    memcpy(output, input, n * sizeof(*output));
    if (reverse) for (i=0;i<runs;++i) if (reverse[i]) {
        size_t l=boundaries[i],r=boundaries[i+1]-1;
        while(l<r) { int32_t t=output[l];output[l++]=output[r];output[r--]=t; }
    }
    src = output; dst = scratch;
    while (runs > 1) {
        size_t new_runs = 0;
        next[0] = 0;
        for (i = 0; i < runs; i += 2) {
            size_t start = boundaries[i], mid = boundaries[i+1];
            size_t end = i+1 < runs ? boundaries[i+2] : mid;
            size_t l = start, r = mid, pos = start;
            if (mid == end || src[mid-1] <= src[mid])
                memcpy(dst+start, src+start, (end-start)*sizeof(*dst));
            else if (src[end-1] < src[start]) {
                memcpy(dst+start, src+mid, (end-mid)*sizeof(*dst));
                memcpy(dst+start+end-mid, src+start, (mid-start)*sizeof(*dst));
            } else {
                while (l < mid && r < end) dst[pos++] = src[l] <= src[r] ? src[l++] : src[r++];
                while (l < mid) dst[pos++] = src[l++];
                while (r < end) dst[pos++] = src[r++];
            }
            next[++new_runs] = end;
        }
        memcpy(boundaries, next, (new_runs+1)*sizeof(*boundaries));
        runs = new_runs;
        { int32_t *tmp = src; src = dst; dst = tmp; }
    }
    if (src != output) memcpy(output, src, n*sizeof(*output));
    free(scratch); return TSPR3_OK;
}

static size_t natural_runs(const int32_t *input, size_t n, size_t *boundaries, unsigned char *reverse) {
    size_t start=0,runs=0;
    boundaries[0]=0;
    while(start<n) {
        size_t end=start+1;
        int descending=end<n && input[start]>input[end];
        if(runs==64) return 0;
        reverse[runs]=(unsigned char)descending;
        while(end<n && (descending ? input[end-1]>=input[end] : input[end-1]<=input[end])) ++end;
        boundaries[++runs]=end;start=end;
    }
    return runs;
}

/* Sampling is a hint only. An exact count confirms the 90% condition before
 * partitioning. Samples cannot make the output incorrect. */
static tspr3_status dominant_sort(const int32_t *input, size_t n, int32_t *output, int *finished) {
    size_t j, votes=0, sample_count=0, count=0, step=n/512, m, split;
    int32_t candidate=0, *exceptions;
    tspr3_status status;
    *finished=0;
    for(j=0;j<512;++j) {
        int32_t value=input[j*step+step/2];
        if(!votes) { candidate=value;votes=1; }
        else if(value==candidate) ++votes;
        else --votes;
    }
    for(j=0;j<512;++j) if(input[j*step+step/2]==candidate) ++sample_count;
    if(sample_count<461) return TSPR3_OK;
    for(j=0;j<n;++j) if(input[j]==candidate) ++count;
    if(count<n-n/10) return TSPR3_OK;
    m=n-count;
    if(!m) { memcpy(output,input,n*sizeof(*output));*finished=1;return TSPR3_OK; }
    exceptions=malloc(m*sizeof(*exceptions));
    if(!exceptions) return TSPR3_NO_MEMORY;
    m=0;
    for(j=0;j<n;++j) if(input[j]!=candidate) exceptions[m++]=input[j];
    status=tspr3_radix(exceptions,m,output);
    free(exceptions);
    if(status!=TSPR3_OK) return status;
    split=0;
    while(split<m && output[split]<candidate) ++split;
    memmove(output+split+count,output+split,(m-split)*sizeof(*output));
    for(j=split;j<split+count;++j) output[j]=candidate;
    *finished=1;return TSPR3_OK;
}

tspr3_status tspr3_auto(const int32_t *input, size_t n, int32_t *output,
    tspr3_method *method) {
    size_t i, boundaries[65] = {0}, runs = 1, k;
    int descending = 1, exceeded;
    int32_t low, high;
    uint64_t span;
    tspr3_frequency *index;
    tspr3_status status;
    if (method) *method = TSPR3_METHOD_FULL;
    if (!n) return TSPR3_OK;
    if (!input || !output || n > SIZE_MAX / sizeof(*output)) return TSPR3_INVALID;
    if (n < 4096) return auto_small(input, n, output, method);
    low = high = input[0];
    /* One exact scan supplies range, monotonicity and natural-run boundaries. */
    for (i = 1; i < n && (runs <= 64 || descending); ++i) {
        if (input[i] < low) low = input[i];
        if (input[i] > high) high = input[i];
        if (descending && input[i-1] < input[i]) descending = 0;
        if (runs <= 64 && input[i-1] > input[i]) {
            if (runs < 64) boundaries[runs] = i;
            ++runs;
        }
    }
    /* Once run structure is ruled out, use a simple vectorizable range scan. */
    for (; i < n; ++i) {
        if (input[i] < low) low = input[i];
        if (input[i] > high) high = input[i];
    }
    if (runs == 1) {
        memcpy(output,input,n*sizeof(*output));
        if (method) *method = TSPR3_METHOD_ASCENDING;
        return TSPR3_OK;
    }
    if (descending) {
        for (i=0;i<n;++i) output[i]=input[n-1-i];
        if (method) *method = TSPR3_METHOD_DESCENDING;
        return TSPR3_OK;
    }
    if (separated_ternas(input,n)) {
        memcpy(output,input,n*sizeof(*output)); sort_ternas(output,n);
        if (method) *method = TSPR3_METHOD_SEPARATED_BLOCKS;
        return TSPR3_OK;
    }
    if (runs <= 64) {
        boundaries[runs] = n;
        status = sort_runs(input,n,output,boundaries,runs,NULL);
        if (status == TSPR3_OK) { if (method) *method=TSPR3_METHOD_RUNS; return status; }
    }
    {
        unsigned char reverse[64];
        size_t count=natural_runs(input,n,boundaries,reverse);
        if(count) {
            status=sort_runs(input,n,output,boundaries,count,reverse);
            if(status==TSPR3_OK) { if(method) *method=TSPR3_METHOD_RUNS;return status; }
        }
    }
    {
        int finished;
        status=dominant_sort(input,n,output,&finished);
        if(status==TSPR3_OK && finished) {
            if(method) *method=TSPR3_METHOD_DOMINANT;
            return TSPR3_OK;
        }
    }
    span = (uint64_t)((int64_t)high - (int64_t)low) + 1;
    /* 1 MiB counters maximum on x64; at most n/8 counters. */
    if (span <= 131072 && span <= n/8) {
        size_t *counts = calloc((size_t)span, sizeof(*counts));
        if (counts) {
            size_t pos = 0, bucket;
            for (i=0;i<n;++i) ++counts[(size_t)((int64_t)input[i]-low)];
            for (bucket=0;bucket<(size_t)span;++bucket) {
                size_t count=counts[bucket];
                int32_t value=(int32_t)((int64_t)low+(int64_t)bucket);
                while (count--) output[pos++]=value;
            }
            free(counts);
            if (method) *method=TSPR3_METHOD_DENSE;
            return TSPR3_OK;
        }
    }
    status=frequency_index_limited(input,n,&index,&k,tspr3_auto_frequency_budget(n),&exceeded);
    if (status==TSPR3_OK && !exceeded) {
        expand_frequencies(index,k,output); free(index);
        if (method) *method=TSPR3_METHOD_FREQ;
        return TSPR3_OK;
    }
    status=tspr3_radix(input,n,output);
    if (status==TSPR3_OK && method) *method=TSPR3_METHOD_RADIX;
    return status;
}

static int record_compare(const void *a, const void *b) {
    const tspr3_record *x = a, *y = b;
    if (x->sum != y->sum) return (x->sum > y->sum) - (x->sum < y->sum);
    return (x->original_index > y->original_index) - (x->original_index < y->original_index);
}

tspr3_status tspr3_sum(const int32_t *input, size_t n, tspr3_record *output) {
    size_t i, j, m = n / 3 + (n % 3 != 0);
    if (n == 0) return TSPR3_OK;
    if (!input || !output || n > SIZE_MAX / sizeof(*input) ||
        m > SIZE_MAX / sizeof(*output)) return TSPR3_INVALID;
    for (i = 0; i < m; ++i) {
        size_t start = i * 3, count = n - start;
        if (count > 3) count = 3;
        output[i].sum = 0;
        output[i].original_index = i;
        /* Three int32_t values always fit in int64_t. */
        for (j = 0; j < count; ++j) output[i].sum += input[start + j];
    }
    qsort(output, m, sizeof(*output), record_compare);
    return TSPR3_OK;
}

tspr3_status tspr3_full(const int32_t *input, size_t n, int32_t *output) {
    size_t start, width;
    int32_t *scratch, *src, *dst;
    if (n == 0) return TSPR3_OK;
    if (!input || !output || n > SIZE_MAX / sizeof(*output)) return TSPR3_INVALID;
    scratch = malloc(n * sizeof(*scratch));
    if (!scratch) return TSPR3_NO_MEMORY;
    memcpy(output, input, n * sizeof(*output));
    /* Stable insertion sort on blocks of at most three. */
    sort_ternas(output, n);
    src = output; dst = scratch;
    for (width = 3; width < n;) {
        for (start = 0; start < n;) {
            size_t mid = start + (n - start < width ? n - start : width);
            size_t end = mid + (n - mid < width ? n - mid : width);
            if (mid == end || src[mid - 1] <= src[mid]) {
                memcpy(dst + start, src + start, (end - start) * sizeof(*dst));
            } else if (src[end - 1] < src[start]) {
                /* Strict inequality preserves the order of equal keys. */
                memcpy(dst + start, src + mid, (end - mid) * sizeof(*dst));
                memcpy(dst + start + end - mid, src + start, (mid - start) * sizeof(*dst));
            } else {
                size_t l = start, r = mid, k = start;
                while (l < mid && r < end)
                    dst[k++] = src[l] <= src[r] ? src[l++] : src[r++];
                while (l < mid) dst[k++] = src[l++];
                while (r < end) dst[k++] = src[r++];
            }
            start = end;
        }
        { int32_t *tmp = src; src = dst; dst = tmp; }
        if (width > n / 2) break;
        width *= 2;
    }
    if (src != output) memcpy(output, src, n * sizeof(*output));
    free(scratch);
    return TSPR3_OK;
}
