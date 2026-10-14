#include "tspr3_log_selector.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
int main(void) {
    int32_t a[7000];size_t i;int c;tspr3_profile p;tspr3_log_choice choice;
    CHECK(tspr3_log_profile(NULL,0,&p)==TSPR3_OK && p.sampled_triads==0);
    CHECK(tspr3_log_profile(NULL,1,&p)==TSPR3_INVALID);
    CHECK(tspr3_log_profile(a,SIZE_MAX,&p)==TSPR3_INVALID);
    CHECK(tspr3_log_profile(a,1,NULL)==TSPR3_INVALID);
    for(c=0;c<4;++c) {
        for(i=0;i<7000;++i) a[i]=c==0 ? -7 : c==1 ? (i%2 ? INT32_MIN : INT32_MAX) : c==2 ? (int32_t)i : (int32_t)(i%13);
        CHECK(tspr3_log_profile(a,7000,&p)==TSPR3_OK);
        CHECK(p.sampled_triads==2048 && p.scale>=1 && isfinite(p.scale));
        for(i=0;i<TSPR3_FEATURES;++i) CHECK(isfinite(p.features[i]) && p.features[i]>=0);
        for(i=0;i<5;++i) CHECK(p.features[i]<=1);
        CHECK(p.features[6]<=1 && p.features[7]<=1);
        if(c==0) CHECK(p.mean==-7 && p.features[1]>0.999 && p.features[5]==0);
        CHECK(tspr3_log_choose(a,7000,7000*sizeof(int32_t),&choice)==TSPR3_OK && choice.selected==TSPR3_TRI_BASE);
        CHECK(tspr3_log_choose(a,7000,1,&choice)==TSPR3_NO_MEMORY);
        CHECK(tspr3_tri_memory_bound(7000,TSPR3_TRI_COUNTS)<=tspr3_tri_memory_bound(7000,TSPR3_TRI_BIN));
    }
    CHECK(tspr3_tri_memory_bound(SIZE_MAX,TSPR3_TRI_BIN)==SIZE_MAX);
    CHECK(tspr3_tri_memory_bound(0,TSPR3_TRI_BIN)==0);
    puts("OK: logarithmic profile, finite features, sample cap, memory constraints and selector guards.");
    return 0;
}
