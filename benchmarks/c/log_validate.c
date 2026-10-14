#include "tspr3_log_selector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define N 2000000
#define LOG_EXPERIMENT
#include "tspr3_scenarios.h"
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
static double now(void) {
#ifdef _WIN32
    LARGE_INTEGER t,f;QueryPerformanceFrequency(&f);QueryPerformanceCounter(&t);
    return (double)t.QuadPart/(double)f.QuadPart;
#else
    struct timespec t;timespec_get(&t,TIME_UTC);return (double)t.tv_sec+(double)t.tv_nsec/1e9;
#endif
}
static int compare(const void *a,const void *b) {
    int32_t x=*(const int32_t *)a,y=*(const int32_t *)b;return (x>y)-(x<y);
}
int main(void) {
    int32_t *a=malloc(N*sizeof(*a)),*out=malloc(N*sizeof(*out)),*ref=malloc(N*sizeof(*ref));
    FILE *csv=fopen("benchmarks/results/log_validation.csv","w");size_t scenario;int repetition;
    const char *variants[]={"TERNAS","TERNAS_FREQ","TERNAS_BIN"};
    tspr3_log_choice choice;
    CHECK(a && out && ref && csv);
    a[0]=0;
    CHECK(tspr3_log_sort(NULL,0,NULL,SIZE_MAX,&choice,NULL)==TSPR3_OK);
    CHECK(tspr3_log_sort(NULL,1,out,SIZE_MAX,&choice,NULL)==TSPR3_INVALID);
    CHECK(tspr3_log_sort(a,1,NULL,SIZE_MAX,&choice,NULL)==TSPR3_INVALID);
    CHECK(tspr3_log_choose(a,1,0,&choice)==TSPR3_NO_MEMORY);
    CHECK(tspr3_log_choose(NULL,0,SIZE_MAX,NULL)==TSPR3_INVALID);
    fprintf(csv,"scenario,n,seed,selected,selected_total_ms,predicted_A_ms,predicted_B_ms,predicted_C_ms,peak_aux_bytes\n");
    for(scenario=0;scenario<sizeof(names)/sizeof(names[0]);++scenario) {
        for(repetition=3;repetition<5;++repetition) {
            uint32_t seed=UINT32_C(20261013)+(uint32_t)scenario*101+(uint32_t)repetition*7919;
            double t,elapsed;tspr3_tri_stats stats;
            generate(a,(int)scenario,seed);
            memcpy(ref,a,N*sizeof(*a));qsort(ref,N,sizeof(*ref),compare);
            t=now();CHECK(tspr3_log_sort(a,N,out,256*1024*1024,&choice,&stats)==TSPR3_OK);elapsed=(now()-t)*1000;
            CHECK(memcmp(out,ref,N*sizeof(*out))==0);
            CHECK(stats.peak_aux_bytes<=tspr3_tri_memory_bound(N,choice.selected));
            fprintf(csv,"%s,%d,%u,%s,%.6f,%.6f,%.6f,%.6f,%zu\n",names[scenario],N,seed,
                variants[choice.selected],elapsed,choice.predicted_ms[0],choice.predicted_ms[1],choice.predicted_ms[2],stats.peak_aux_bytes);
            printf("%-22s seed=%u selected=%s total=%.3f ms\n",names[scenario],seed,variants[choice.selected],elapsed);
            fflush(csv);fflush(stdout);
        }
    }
    CHECK(fclose(csv)==0);free(a);free(out);free(ref);
    puts("OK: 72 held-out inputs x 2,000,000 elements; predictive selector outputs verified.");
    return 0;
}
