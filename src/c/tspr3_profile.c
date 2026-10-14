#include "tspr3_profile.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define SAMPLE 2048
static int compare(const void *a,const void *b) {
    int32_t x=*(const int32_t *)a,y=*(const int32_t *)b;return (x>y)-(x<y);
}
static double logarithmic(int32_t x,double mean,double scale) {
    double delta=(double)x-mean;
    return copysign(log2(1+fabs(delta)/scale),delta);
}
static unsigned int leading(uint32_t x) {
    unsigned int result=0;
    if(!x) return 32;
    while(!(x & UINT32_C(0x80000000))) { ++result;x<<=1; }
    return result;
}
tspr3_status tspr3_log_profile(const int32_t *input,size_t n,tspr3_profile *p) {
    int32_t values[SAMPLE*3],low[SAMPLE],high[SAMPLE];
    size_t m=n/3+(n%3!=0),s=m<SAMPLE ? m : SAMPLE,i,j,used=0,unique=0;
    double mean=0,m2=0,local_repeats=0,separated=0,prefix=0,root=0,width=0,cross=0,order=0;
    const double edges[]={-2,-1,-0.5,0,0.5,1,2};
    if(!p) return TSPR3_INVALID;
    memset(p,0,sizeof(*p));
    if(!n) return TSPR3_OK;
    if(!input || n>SIZE_MAX/sizeof(*input)) return TSPR3_INVALID;
    for(i=0;i<s;++i) {
        /* Avoid multiplication overflow: i*floor(m/s) + floor(i*(m%s)/s). */
        size_t block=i*(m/s)+(i*(m%s))/s,start=block*3,count=n-start;
        int32_t v[3];
        if(count>3) count=3;
        for(j=0;j<count;++j) {
            double delta=(double)input[start+j]-mean;
            v[j]=input[start+j];values[used++]=v[j];
            mean+=delta/(double)used;m2+=delta*((double)v[j]-mean);
        }
        qsort(v,count,sizeof(*v),compare);low[i]=v[0];high[i]=v[count-1];
        for(j=1;j<count;++j) if(v[j]==v[j-1]) ++local_repeats;
        {
            uint32_t difference=((uint32_t)low[i])^((uint32_t)high[i]);
            unsigned int bits=leading(difference);
            prefix+=bits;if(!bits) ++root;
        }
        if(i) {
            if(high[i-1]<=low[i] || high[i]<=low[i-1]) ++separated;
            if(low[i]>=low[i-1]) ++order;
        }
    }
    p->mean=mean;p->scale=used>1 ? sqrt(m2/(double)(used-1)) : 1;
    if(p->scale<1) p->scale=1;
    for(i=0;i<s;++i) {
        double a=logarithmic(low[i],mean,p->scale),b=logarithmic(high[i],mean,p->scale);
        width+=b-a;
        for(j=0;j<sizeof(edges)/sizeof(edges[0]);++j) if(a<edges[j] && b>=edges[j]) ++cross;
    }
    qsort(values,used,sizeof(*values),compare);
    for(i=0;i<used;++i) if(!i || values[i]!=values[i-1]) ++unique;
    p->sampled_triads=s;
    p->features[0]=local_repeats/(double)used;
    p->features[1]=1-(double)unique/(double)used;
    p->features[2]=s>1 ? separated/(double)(s-1) : 1;
    p->features[3]=prefix/(32*(double)s);
    p->features[4]=root/(double)s;
    p->features[5]=width/(double)s;
    p->features[6]=cross/(7*(double)s);
    p->features[7]=s>1 ? order/(double)(s-1) : 1;
    p->features[8]=log2(1+((double)values[used-1]-(double)values[0])/p->scale);
    return TSPR3_OK;
}
