/* Million-element scenarios, independent reference, rotated timing order.
 * CSV files contain every timing; no network, parallelism or dependencies. */
#include "tspr3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
#define N 1000000
#define REPEATS 5
#define METHODS 5
#include "tspr3_scenarios.h"
static double now(void) {
#ifdef _WIN32
    LARGE_INTEGER t,f; QueryPerformanceFrequency(&f);QueryPerformanceCounter(&t);
    return (double)t.QuadPart/(double)f.QuadPart;
#else
    struct timespec t;timespec_get(&t,TIME_UTC);return (double)t.tv_sec+(double)t.tv_nsec/1e9;
#endif
}
static int compare(const void *a,const void *b) {
    int32_t x=*(const int32_t *)a,y=*(const int32_t *)b;return (x>y)-(x<y);
}
static void verify_sum(const int32_t *a) {
    const size_t m=N/3+(N%3!=0);
    tspr3_record *records=malloc(m*sizeof(*records));
    unsigned char *seen=calloc(m,1);size_t i;
    CHECK(records && seen);CHECK(tspr3_sum(a,N,records)==TSPR3_OK);
    for(i=0;i<m;++i) {
        size_t idx=records[i].original_index,j;int64_t sum=0;
        CHECK(idx<m && !seen[idx]);seen[idx]=1;
        for(j=idx*3;j<N && j-idx*3<3;++j) sum+=a[j];
        CHECK(sum==records[i].sum);
        if(i) CHECK(records[i-1].sum<records[i].sum ||
            (records[i-1].sum==records[i].sum && records[i-1].original_index<idx));
    }
    free(records);free(seen);
}
int main(void) {
    int32_t *a=malloc(N*sizeof(*a)),*out=malloc(N*sizeof(*out)),*ref=malloc(N*sizeof(*ref));
    const char *algorithms[]={"FULL","FREQ","RADIX","AUTO","qsort"};
    FILE *raw=fopen("benchmarks/results/million_raw.csv","w"),*summary=fopen("benchmarks/results/million_summary.csv","w");
    size_t scenario,i;int repetition,slot,algorithm;
    CHECK(a && out && ref && raw && summary);
    fprintf(raw,"scenario,n,seed,k,algorithm,method,ms\n");
    fprintf(summary,"scenario,n,algorithm,median_ms,min_ms,max_ms\n");
    for(scenario=0;scenario<sizeof(names)/sizeof(names[0]);++scenario) {
        double times[METHODS][REPEATS];
        for(repetition=0;repetition<REPEATS;++repetition) {
            uint32_t seed=UINT32_C(20261013)+(uint32_t)scenario*101+(uint32_t)repetition*7919;
            size_t expected_k=0;
            generate(a,(int)scenario,seed);
            memcpy(ref,a,N*sizeof(*a));qsort(ref,N,sizeof(*ref),compare);
            for(i=0;i<N;++i) if(!i || ref[i]!=ref[i-1]) ++expected_k;
            if(!repetition) verify_sum(a);
            for(slot=0;slot<METHODS;++slot) {
                double t,elapsed;size_t k=0;
                tspr3_method method=TSPR3_METHOD_FULL;
                algorithm=(slot+repetition+(int)scenario)%METHODS;
                t=now();
                if(algorithm==0) CHECK(tspr3_full(a,N,out)==TSPR3_OK);
                else if(algorithm==1) CHECK(tspr3_freq(a,N,out,&k)==TSPR3_OK);
                else if(algorithm==2) CHECK(tspr3_radix(a,N,out)==TSPR3_OK);
                else if(algorithm==3) CHECK(tspr3_auto(a,N,out,&method)==TSPR3_OK);
                else { memcpy(out,a,N*sizeof(*a));qsort(out,N,sizeof(*out),compare); }
                elapsed=(now()-t)*1000;times[algorithm][repetition]=elapsed;
                CHECK(memcmp(out,ref,N*sizeof(*out))==0);
                if(algorithm==1) CHECK(k==expected_k);
                fprintf(raw,"%s,%d,%u,%zu,%s,%d,%.6f\n",names[scenario],N,seed,expected_k,
                    algorithms[algorithm],algorithm==3 ? (int)method : -1,elapsed);
            }
        }
        for(algorithm=0;algorithm<METHODS;++algorithm) {
            int j,l;
            for(j=1;j<REPEATS;++j) for(l=j;l>0 && times[algorithm][l]<times[algorithm][l-1];--l) {
                double t=times[algorithm][l];times[algorithm][l]=times[algorithm][l-1];times[algorithm][l-1]=t;
            }
            fprintf(summary,"%s,%d,%s,%.6f,%.6f,%.6f\n",names[scenario],N,algorithms[algorithm],
                times[algorithm][REPEATS/2],times[algorithm][0],times[algorithm][REPEATS-1]);
        }
        printf("%-22s AUTO=%.3f FULL=%.3f FREQ=%.3f RADIX=%.3f qsort=%.3f ms\n",names[scenario],
            times[3][REPEATS/2],times[0][REPEATS/2],times[1][REPEATS/2],times[2][REPEATS/2],times[4][REPEATS/2]);
        fflush(stdout);fflush(raw);fflush(summary);
    }
    CHECK(fclose(raw)==0);CHECK(fclose(summary)==0);free(a);free(out);free(ref);
    puts("OK: 30 scenarios x 5 seeds x 1,000,000 elements; 750 full-sort checks + 30 SUM checks.");
    return 0;
}
