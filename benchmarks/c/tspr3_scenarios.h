#ifndef TSPR3_SCENARIOS_H
#define TSPR3_SCENARIOS_H
/* Shared reproducible million-element input generators; requires N. */
static uint32_t state;
static uint32_t rnd(void) { state^=state<<13;state^=state>>17;state^=state<<5;return state; }
static int32_t wide(void) { return (int32_t)((int64_t)rnd()-INT64_C(2147483648)); }
static const char *names[]={"random_wide","random_bounded","ascending","descending","equal",
    "k8","k64","k512","k2048","k8192","k32768","k131072","sparse8","sparse2048",
    "separated","interleaved","organ_pipe","sawtooth","nearly_sorted_one","nearly_sorted_100",
    "runs16","extremes","duplicate_prefix","duplicate_suffix","skew99","outlier",
    "descending_groups","permutation","constant_low_bytes","signed_boundaries"
#ifdef LOG_EXPERIMENT
    ,"normal_wide","normal_quantized","normal_local","lognormal","bimodal","normal_clusters"
#endif
};
#ifdef LOG_EXPERIMENT
static double normal_value(void) {
    double u=((double)rnd()+1)/4294967297.0,v=((double)rnd()+1)/4294967297.0;
    return sqrt(-2*log(u))*cos(6.2831853071795864769*v);
}
#endif
static void generate(int32_t *a,int scenario,uint32_t seed) {
    size_t i; state=seed;
#ifdef LOG_EXPERIMENT
    if(scenario>=30) {
        double center=0;
        for(i=0;i<N;++i) {
            double value;
            if(scenario==30) value=normal_value()*1000000;
            else if(scenario==31) value=normal_value()*3;
            else if(scenario==32) {
                if(i%3==0) center=round(normal_value()*1000000);
                value=center+(double)(rnd()%5)-2;
            } else if(scenario==33) value=exp(3+2*normal_value());
            else if(scenario==34) value=(rnd()%2 ? 1000000 : -1000000)+normal_value()*1000;
            else {
                if(i%3==0) center=((double)(rnd()%64)-32)*100000;
                value=center+normal_value()*3;
            }
            if(value>INT32_MAX) value=INT32_MAX;
            if(value<INT32_MIN) value=INT32_MIN;
            a[i]=(int32_t)round(value);
        }
        return;
    }
#endif
    for(i=0;i<N;++i) {
        if(scenario==0) a[i]=wide();
        else if(scenario==1) a[i]=(int32_t)(rnd()%2000001)-1000000;
        else if(scenario==2 || scenario==18 || scenario==19 || scenario==27) a[i]=(int32_t)i;
        else if(scenario==3) a[i]=(int32_t)(N-i);
        else if(scenario==4) a[i]=-7;
        else if(scenario>=5 && scenario<=11) {
            const uint32_t k[]={8,64,512,2048,8192,32768,131072};a[i]=(int32_t)(rnd()%k[scenario-5]);
        } else if(scenario==12) a[i]=(int32_t)((rnd()%8)*UINT32_C(100000003))-400000000;
        else if(scenario==13) a[i]=(int32_t)((rnd()%2048)*UINT32_C(1000003))-1000000000;
        else if(scenario==14) a[i]=(int32_t)((i/3)*3+2-i%3);
        else if(scenario==15) a[i]=(int32_t)((i%3)*(N/3+1)+i/3);
        else if(scenario==16) a[i]=(int32_t)(i<N/2 ? i : N-i);
        else if(scenario==17) a[i]=(int32_t)(i%1000)-500;
        else if(scenario==20) a[i]=(int32_t)(((15-i/(N/16))*(N/16))+i%(N/16));
        else if(scenario==21) a[i]=i%2 ? INT32_MIN : INT32_MAX;
        else if(scenario==22) a[i]=i<N*9/10 ? (int32_t)(i%8) : wide();
        else if(scenario==23) a[i]=i>N/10 ? (int32_t)(i%8) : wide();
        else if(scenario==24) a[i]=rnd()%100 ? 0 : wide();
        else if(scenario==25) a[i]=(int32_t)(rnd()%64);
        else if(scenario==26) a[i]=(int32_t)((N-i)/1000);
        else if(scenario==28) a[i]=(int32_t)((int64_t)(rnd()%65536)*65536-INT64_C(2147483648));
        else a[i]=i%4==0 ? INT32_MIN : i%4==1 ? INT32_MAX : i%4==2 ? -1 : 0;
    }
    if(scenario==18 || scenario==19) {
        size_t swaps=scenario==18 ? 1 : 100;
        for(i=0;i<swaps;++i) { size_t x=rnd()%N,y=rnd()%N;int32_t t=a[x];a[x]=a[y];a[y]=t; }
    }
    if(scenario==25) a[N/2]=INT32_MIN;
    if(scenario==27) for(i=N-1;i>0;--i) { size_t j=rnd()%(i+1);int32_t t=a[i];a[i]=a[j];a[j]=t; }
}
#endif
