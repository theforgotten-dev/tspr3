#include "tspr3_log_selector.h"
#include "tspr3_log_weights.h"
#include <math.h>
#include <string.h>
tspr3_status tspr3_log_choose(const int32_t *input,size_t n,size_t limit,tspr3_log_choice *c) {
    tspr3_status status;int variant,best=-1;size_t i;
    if(!c) return TSPR3_INVALID;
    memset(c,0,sizeof(*c));
    status=tspr3_log_profile(input,n,&c->profile);
    if(status!=TSPR3_OK) return status;
    for(variant=0;variant<3;++variant) {
        double logtime=log_weights[variant][0];
        size_t memory=tspr3_tri_memory_bound(n,(tspr3_tri_mode)variant);
        for(i=0;i<TSPR3_FEATURES;++i)
            logtime+=log_weights[variant][i+1]*(c->profile.features[i]-log_center[i])/log_scale[i];
        c->predicted_ms[variant]=(double)n/1e6*exp(logtime);
        c->conservative_ms[variant]=c->predicted_ms[variant]*exp(log_margin[variant]);
        if(log_enabled[variant] && memory!=SIZE_MAX && memory<=limit &&
            (best<0 || c->conservative_ms[variant]<c->conservative_ms[best])) best=variant;
    }
    if(best<0) return TSPR3_NO_MEMORY;
    /* Prefer the base unless another route has a predicted >=10% advantage. */
    if(best && tspr3_tri_memory_bound(n,TSPR3_TRI_BASE)<=limit &&
       c->conservative_ms[best]>0.90*c->conservative_ms[TSPR3_TRI_BASE]) best=0;
    c->selected=(tspr3_tri_mode)best;
    return TSPR3_OK;
}
tspr3_status tspr3_log_sort(const int32_t *input,size_t n,int32_t *output,size_t limit,
    tspr3_log_choice *choice,tspr3_tri_stats *stats) {
    tspr3_log_choice local;tspr3_status status;
    if(stats) memset(stats,0,sizeof(*stats));
    if(n && !output) return TSPR3_INVALID;
    if(!choice) choice=&local;
    status=tspr3_log_choose(input,n,limit,choice);
    if(status!=TSPR3_OK) return status;
    return tspr3_tri_sort(input,n,choice->selected,output,stats);
}
