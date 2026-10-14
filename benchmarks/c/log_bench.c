#include "tspr3_tri.h"
#include "tspr3_profile.h"
#include <math.h>
#define LOG_EXPERIMENT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define N 2000000
#define REPEATS 5
#define VARIANTS 3
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
static double median(double values[REPEATS]) {
    int j,l;
    for(j=1;j<REPEATS;++j) for(l=j;l>0 && values[l]<values[l-1];--l) {
        double t=values[l];values[l]=values[l-1];values[l-1]=t;
    }
    return values[REPEATS/2];
}
int main(void) {
    int32_t *a=malloc(N*sizeof(*a)),*out=malloc(N*sizeof(*out)),*ref=malloc(N*sizeof(*ref));
    const char *variants[]={"TERNAS","TERNAS_FREQ","TERNAS_BIN"};
    FILE *raw=fopen("benchmarks/results/tri_2m_raw.csv","w"),*summary=fopen("benchmarks/results/tri_2m_summary.csv","w");
    size_t scenario,i;int repetition,slot,variant;
    CHECK(a && out && ref && raw && summary);
    fprintf(raw,"scenario,n,seed,k,variant,ms,comparisons,data_bytes_written,peak_aux_bytes,equal_merges,range_joins,prefix_tests,prefix_joins,prefix_nodes,crossing_root_blocks,fold,profile_ms,f0,f1,f2,f3,f4,f5,f6,f7,f8\n");
    fprintf(summary,"scenario,n,variant,median_ms,min_ms,max_ms,median_comparisons,median_data_bytes_written,median_peak_aux_bytes,median_prefix_nodes,median_crossing_root_blocks\n");
    for(scenario=0;scenario<sizeof(names)/sizeof(names[0]);++scenario) {
        double times[VARIANTS][REPEATS];tspr3_tri_stats stats[VARIANTS][REPEATS];
        for(repetition=0;repetition<REPEATS;++repetition) {
            uint32_t seed=UINT32_C(20261013)+(uint32_t)scenario*101+(uint32_t)repetition*7919;
            size_t k=0; tspr3_profile profile; double profile_ms,tprofile;
            generate(a,(int)scenario,seed);
            tprofile=now(); CHECK(tspr3_log_profile(a,N,&profile)==TSPR3_OK);profile_ms=(now()-tprofile)*1000;
            memcpy(ref,a,N*sizeof(*a));qsort(ref,N,sizeof(*ref),compare);
            for(i=0;i<N;++i) if(!i || ref[i]!=ref[i-1]) ++k;
            for(slot=0;slot<VARIANTS;++slot) {
                double t;tspr3_tri_stats *s;
                variant=(slot+repetition+(int)scenario)%VARIANTS;s=&stats[variant][repetition];
                t=now();CHECK(tspr3_tri_sort(a,N,(tspr3_tri_mode)variant,out,s)==TSPR3_OK);
                times[variant][repetition]=(now()-t)*1000;
                CHECK(memcmp(out,ref,N*sizeof(*out))==0);
                if(variant) CHECK(s->distinct_values==k);
                CHECK(s->triads==N/3+(N%3!=0));
                fprintf(raw,"%s,%d,%u,%zu,%s,%.6f,%llu,%llu,%zu,%llu,%llu,%llu,%llu,%zu,%zu",
                    names[scenario],N,seed,k,variants[variant],times[variant][repetition],
                    (unsigned long long)s->comparisons,(unsigned long long)s->data_bytes_written,s->peak_aux_bytes,
                    (unsigned long long)s->equal_merges,(unsigned long long)s->range_joins,
                    (unsigned long long)s->prefix_tests,(unsigned long long)s->prefix_joins,s->prefix_nodes,s->crossing_root_blocks);
                fprintf(raw,",%s,%.6f",repetition<3 ? "train" : "test",profile_ms);
                for(i=0;i<TSPR3_FEATURES;++i) fprintf(raw,",%.12g",profile.features[i]);
                fprintf(raw,"\n");
            }
        }
        for(variant=0;variant<VARIANTS;++variant) {
            double comp[REPEATS],bytes[REPEATS],memory[REPEATS],nodes[REPEATS],cross[REPEATS],middle;
            for(repetition=0;repetition<REPEATS;++repetition) {
                comp[repetition]=(double)stats[variant][repetition].comparisons;
                bytes[repetition]=(double)stats[variant][repetition].data_bytes_written;
                memory[repetition]=(double)stats[variant][repetition].peak_aux_bytes;
                nodes[repetition]=(double)stats[variant][repetition].prefix_nodes;
                cross[repetition]=(double)stats[variant][repetition].crossing_root_blocks;
            }
            middle=median(times[variant]);
            fprintf(summary,"%s,%d,%s,%.6f,%.6f,%.6f,%.0f,%.0f,%.0f,%.0f,%.0f\n",names[scenario],N,variants[variant],
                middle,times[variant][0],times[variant][REPEATS-1],median(comp),median(bytes),median(memory),median(nodes),median(cross));
        }
        printf("%-22s TERNAS=%.3f FREQ=%.3f BIN=%.3f ms\n",names[scenario],
            times[0][REPEATS/2],times[1][REPEATS/2],times[2][REPEATS/2]);
        fflush(stdout);fflush(raw);fflush(summary);
    }
    CHECK(fclose(raw)==0);CHECK(fclose(summary)==0);free(a);free(out);free(ref);
    puts("OK: 36 scenarios x 5 seeds x 2,000,000 elements; 540 ternas-sort checks.");
    return 0;
}
