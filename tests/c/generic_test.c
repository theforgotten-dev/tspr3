#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
static size_t allocations,fail_at=SIZE_MAX,live_allocations;
static void *test_malloc(size_t bytes) {
    void *p;
    if(allocations++==fail_at) return NULL;
    p=malloc(bytes);if(p) ++live_allocations;return p;
}
static void test_free(void *p) { if(p) { --live_allocations;free(p); } }
/* Compile the same implementation with deterministic allocator fault injection. */
#define malloc test_malloc
#define free test_free
#include "../../src/c/tspr3_generic_tri.c"
#undef malloc
#undef free
#define CHECK(condition) do { if(!(condition)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#condition);exit(1); } } while(0)
typedef struct { int key,id; double aligned; unsigned char payload[300]; } record;
typedef struct { const record *input; size_t n; } test_context;
static int compare_record(const void *a,const void *b,void *opaque) {
    const record *x=a,*y=b;test_context *context=opaque;
    CHECK(x>=context->input && x<context->input+context->n);
    CHECK(y>=context->input && y<context->input+context->n);
    return (x->key>y->key)-(x->key<y->key);
}
static int reference_compare(const void *a,const void *b) {
    const record *x=a,*y=b;
    int order=(x->key>y->key)-(x->key<y->key);
    return order?order:(x->id>y->id)-(x->id<y->id);
}
static uint64_t record_key(const void *a,void *context) {
    (void)context;return (uint32_t)((const record *)a)->key ^ UINT32_C(0x80000000);
}
static uint64_t colliding_key(const void *a,void *context) { (void)a;(void)context;return 0; }
static int compare_bytes(const void *a,const void *b,void *context) {
    (void)context;return (int)*(const unsigned char *)a-(int)*(const unsigned char *)b;
}
int main(void) {
    record input[151],output[151],expected[151],saved[151];
    size_t n,i,mode,failures=0;uint32_t seed=37;
    for(n=0;n<=151;++n) {
        test_context context={input,n};
        memset(input,0,sizeof(input));
        for(i=0;i<n;++i) { seed=seed*1664525u+1013904223u;input[i].key=(int)(seed%7)-3;input[i].id=(int)i;input[i].aligned=(double)i;memset(input[i].payload,(int)i,300); }
        memcpy(saved,input,sizeof(input));memcpy(expected,input,n*sizeof(record));
        qsort(expected,n,sizeof(record),reference_compare);
        for(mode=0;mode<3;++mode) {
            tspr3_tri_stats stats;
            CHECK(tspr3_generic_tri_sort(input,n,sizeof(record),(tspr3_tri_mode)mode,output,compare_record,record_key,&context,&stats)==TSPR3_OK);
            CHECK(!memcmp(output,expected,n*sizeof(record)));
            CHECK(!memcmp(input,saved,sizeof(input)));
            CHECK(stats.peak_aux_bytes<=tspr3_generic_tri_memory_bound(n,sizeof(record),(tspr3_tri_mode)mode));
            CHECK(!live_allocations);
        }
        CHECK(tspr3_generic_tri_sort(input,n,sizeof(record),TSPR3_TRI_BIN,output,compare_record,colliding_key,&context,NULL)==TSPR3_OK);
        CHECK(!memcmp(output,expected,n*sizeof(record)));
    }
    /* Force prefix-tree growth during allocator failure tests. */
    for(i=0;i<151;++i) input[i].key=(int)i;
    for(mode=0;mode<3;++mode) {
        unsigned char odd_input[12]={3,10,11,1,20,21,1,30,31,2,40,41},odd_output[12];
        const unsigned char odd_expected[12]={1,20,21,1,30,31,2,40,41,3,10,11};
        CHECK(tspr3_generic_tri_sort(odd_input,4,3,(tspr3_tri_mode)mode,odd_output,compare_bytes,colliding_key,NULL,NULL)==TSPR3_OK);
        CHECK(!memcmp(odd_output,odd_expected,12));
        for(i=0;i<32;++i) {
            tspr3_status status;test_context context={input,151};
            memset(output,0x5a,sizeof(output));allocations=0;fail_at=i;
            status=tspr3_generic_tri_sort(input,151,sizeof(record),(tspr3_tri_mode)mode,output,compare_record,record_key,&context,NULL);
            fail_at=SIZE_MAX;
            CHECK(!live_allocations);
            if(status==TSPR3_OK) break;
            CHECK(status==TSPR3_NO_MEMORY);++failures;
            { size_t j;for(j=0;j<sizeof(output);++j) CHECK(((unsigned char *)output)[j]==0x5a); }
        }
        CHECK(i<32);
    }
    CHECK(tspr3_generic_tri_memory_bound(1,SIZE_MAX,TSPR3_TRI_COUNTS)==SIZE_MAX);
    CHECK(tspr3_generic_tri_sort(input,2,SIZE_MAX,TSPR3_TRI_BASE,output,compare_record,NULL,NULL,NULL)==TSPR3_INVALID);
    CHECK(tspr3_generic_tri_memory_bound(1,0,TSPR3_TRI_BASE)==SIZE_MAX);
    CHECK(tspr3_generic_tri_memory_bound(SIZE_MAX,1,TSPR3_TRI_COUNTS)==SIZE_MAX);
    CHECK(tspr3_generic_tri_sort(NULL,0,1,TSPR3_TRI_BASE,NULL,NULL,NULL,NULL,NULL)==TSPR3_OK);
    CHECK(tspr3_generic_tri_sort(input,1,sizeof(record),TSPR3_TRI_BIN,output,compare_record,NULL,NULL,NULL)==TSPR3_INVALID);
    printf("OK: generic stable records, odd sizes, colliding keys; %zu allocation failures\n",failures);
    return 0;
}
