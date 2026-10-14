#include <tspr3/tspr3.h>
#include "tspr3_log_selector.h"
#include <stdlib.h>
#include <string.h>

const char *tspr3_version(void) { return TSPR3_VERSION; }
const char *tspr3_result_message(tspr3_result result) {
    switch(result) {
    case TSPR3_SUCCESS:return "success";
    case TSPR3_INVALID_ARGUMENT:return "invalid argument";
    case TSPR3_ALLOCATION_FAILURE:return "allocation failure";
    case TSPR3_HEAP_LIMIT:return "auxiliary heap limit exceeded";
    default:return "unknown result";
    }
}
void tspr3_options_init(tspr3_options *o) {
    if(o) { o->algorithm=TSPR3_ALGORITHM_AUTO;o->auxiliary_heap_limit=(size_t)256*1024*1024; }
}
static int valid(const tspr3_options *o) {
    return o->algorithm>=TSPR3_ALGORITHM_AUTO && o->algorithm<=TSPR3_ALGORITHM_C;
}
static tspr3_result translate(tspr3_status status) {
    if(status==TSPR3_OK) return TSPR3_SUCCESS;
    return status==TSPR3_INVALID ? TSPR3_INVALID_ARGUMENT : TSPR3_ALLOCATION_FAILURE;
}
static void describe(tspr3_report *r,tspr3_tri_mode mode,const tspr3_tri_stats *s) {
    if(!r) return;
    r->selected=(tspr3_algorithm)((int)mode+1);
    r->comparisons=s->comparisons;r->data_bytes_written=s->data_bytes_written;
    r->equal_merges=s->equal_merges;r->range_joins=s->range_joins;
    r->prefix_tests=s->prefix_tests;r->prefix_joins=s->prefix_joins;
    r->peak_aux_bytes=s->peak_aux_bytes;r->triads=s->triads;
    r->distinct_values=s->distinct_values;r->prefix_nodes=s->prefix_nodes;
    r->crossing_root_blocks=s->crossing_root_blocks;
}
tspr3_result tspr3_sort_i32(const int32_t *input,size_t n,int32_t *output,
    const tspr3_options *options,tspr3_report *report) {
    tspr3_options defaults;tspr3_tri_mode mode;tspr3_tri_stats stats;
    size_t bound;tspr3_status status;
    if(report) memset(report,0,sizeof(*report));
    tspr3_options_init(&defaults);if(!options) options=&defaults;
    if(!valid(options) || (n && (!input || !output)) || n>SIZE_MAX/sizeof(tspr3_frequency))
        return TSPR3_INVALID_ARGUMENT;
    if(options->algorithm==TSPR3_ALGORITHM_AUTO) {
        tspr3_log_choice choice;
        status=tspr3_log_choose(input,n,options->auxiliary_heap_limit,&choice);
        if(status!=TSPR3_OK) return status==TSPR3_NO_MEMORY ? TSPR3_HEAP_LIMIT : translate(status);
        mode=choice.selected;
        if(report) {
            memcpy(report->predicted_ms,choice.predicted_ms,sizeof(report->predicted_ms));
            memcpy(report->conservative_ms,choice.conservative_ms,sizeof(report->conservative_ms));
        }
    } else mode=(tspr3_tri_mode)((int)options->algorithm-1);
    bound=tspr3_tri_memory_bound(n,mode);
    if(bound==SIZE_MAX || bound>options->auxiliary_heap_limit) return TSPR3_HEAP_LIMIT;
    status=tspr3_tri_sort(input,n,mode,output,&stats);
    describe(report,mode,&stats);return translate(status);
}
tspr3_result tspr3_counts_i32(const int32_t *input,size_t n,tspr3_frequency_i32 *output,
    size_t *distinct_count,const tspr3_options *options,tspr3_report *report) {
    tspr3_options defaults;tspr3_tri_mode mode;tspr3_tri_stats stats;
    tspr3_frequency *index;size_t k,i,bound;tspr3_status status;
    if(report) memset(report,0,sizeof(*report));
    if(!distinct_count) return TSPR3_INVALID_ARGUMENT;
    *distinct_count=0;tspr3_options_init(&defaults);if(!options) options=&defaults;
    if(!valid(options) || options->algorithm==TSPR3_ALGORITHM_A ||
       (n && (!input || !output)) || n>SIZE_MAX/sizeof(tspr3_frequency)) return TSPR3_INVALID_ARGUMENT;
    mode=options->algorithm==TSPR3_ALGORITHM_C ? TSPR3_TRI_BIN : TSPR3_TRI_COUNTS;
    bound=tspr3_tri_memory_bound(n,mode);
    if(bound==SIZE_MAX || bound>options->auxiliary_heap_limit) return TSPR3_HEAP_LIMIT;
    status=tspr3_tri_index(input,n,mode,&index,&k,&stats);
    if(status==TSPR3_OK) {
        for(i=0;i<k;++i) { output[i].value=index[i].value;output[i].count=index[i].count; }
        stats.data_bytes_written+=(uint64_t)k*sizeof(*output);
        *distinct_count=k;free(index);
    }
    describe(report,mode,&stats);return translate(status);
}
