#include <tspr3/tspr3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1; } } while(0)
static int compare(const void *a,const void *b) {
    int32_t x=*(const int32_t *)a,y=*(const int32_t *)b;return (x>y)-(x<y);
}
int main(void) {
    int32_t input[10000],saved[10000],output[10000],expected[10000];
    tspr3_frequency_i32 counts[10000];size_t i,k,pos;
    tspr3_options options;tspr3_report report;int method;
    for(i=0;i<10000;++i) input[i]=i%7==0 ? INT32_MIN : i%11==0 ? INT32_MAX : (int32_t)(i%127)-63;
    memcpy(saved,input,sizeof(input));memcpy(expected,input,sizeof(input));qsort(expected,10000,sizeof(*expected),compare);
    tspr3_options_init(&options);
    CHECK(options.algorithm==TSPR3_ALGORITHM_AUTO && options.auxiliary_heap_limit>0);
    for(method=0;method<4;++method) {
        options.algorithm=(tspr3_algorithm)method;
        CHECK(tspr3_sort_i32(input,10000,output,&options,&report)==TSPR3_SUCCESS);
        CHECK(memcmp(output,expected,sizeof(output))==0 && memcmp(input,saved,sizeof(input))==0);
        CHECK(report.selected>=TSPR3_ALGORITHM_A && report.selected<=TSPR3_ALGORITHM_C);
        if(method) CHECK(report.selected==(tspr3_algorithm)method);
        if(method!=1) {
            CHECK(tspr3_counts_i32(input,10000,counts,&k,&options,&report)==TSPR3_SUCCESS);
            pos=0;
            for(i=0;i<k;++i) {
                size_t start=pos;
                while(pos<10000 && expected[pos]==counts[i].value) ++pos;
                CHECK(counts[i].count==pos-start && pos>start);
            }
            CHECK(pos==10000);
        }
    }
    CHECK(tspr3_sort_i32(NULL,0,NULL,NULL,NULL)==TSPR3_SUCCESS);
    CHECK(tspr3_counts_i32(NULL,0,NULL,&k,NULL,NULL)==TSPR3_SUCCESS && k==0);
    CHECK(tspr3_sort_i32(NULL,1,output,NULL,NULL)==TSPR3_INVALID_ARGUMENT);
    CHECK(tspr3_sort_i32(input,1,NULL,NULL,NULL)==TSPR3_INVALID_ARGUMENT);
    CHECK(tspr3_sort_i32(input,SIZE_MAX,output,NULL,NULL)==TSPR3_INVALID_ARGUMENT);
    options.algorithm=(tspr3_algorithm)99;
    CHECK(tspr3_sort_i32(input,1,output,&options,NULL)==TSPR3_INVALID_ARGUMENT);
    tspr3_options_init(&options);options.auxiliary_heap_limit=0;
    CHECK(tspr3_sort_i32(input,1,output,&options,NULL)==TSPR3_HEAP_LIMIT);
    CHECK(tspr3_counts_i32(input,1,counts,&k,&options,NULL)==TSPR3_HEAP_LIMIT);
    CHECK(tspr3_counts_i32(input,1,counts,NULL,NULL,NULL)==TSPR3_INVALID_ARGUMENT);
    CHECK(strcmp(tspr3_version(),TSPR3_VERSION)==0);
    puts("OK: public C ABI, all modes, counts, errors and immutable input.");return 0;
}
