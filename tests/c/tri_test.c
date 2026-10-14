#include "tspr3_tri.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
static size_t cases;
static uint32_t state=20261013;
static uint32_t rnd(void) { state^=state<<13;state^=state>>17;state^=state<<5;return state; }
static int compare(const void *a,const void *b) {
    int32_t x=*(const int32_t *)a,y=*(const int32_t *)b;return (x>y)-(x<y);
}
static void verify(const int32_t *a,size_t n) {
    int32_t out[1000],expected[1000],saved[1000];
    size_t i,k=0;int mode;
    CHECK(n<=1000);
    if(n) { memcpy(expected,a,n*sizeof(*a));memcpy(saved,a,n*sizeof(*a)); }
    qsort(expected,n,sizeof(*expected),compare);
    for(i=0;i<n;++i) if(!i || expected[i]!=expected[i-1]) ++k;
    for(mode=0;mode<3;++mode) {
        tspr3_tri_stats stats;
        CHECK(tspr3_tri_sort(a,n,(tspr3_tri_mode)mode,out,&stats)==TSPR3_OK);
        CHECK(memcmp(out,expected,n*sizeof(*out))==0);
        CHECK(stats.triads==n/3+(n%3!=0));
        if(mode) {
            tspr3_frequency *index;size_t distinct,pos=0;
            CHECK(stats.distinct_values==k);
            CHECK(tspr3_tri_index(a,n,(tspr3_tri_mode)mode,&index,&distinct,NULL)==TSPR3_OK);
            CHECK(distinct==k);
            for(i=0;i<distinct;++i) {
                size_t start=pos;
                while(pos<n && expected[pos]==index[i].value) ++pos;
                CHECK(index[i].count==pos-start && pos>start);
            }
            CHECK(pos==n);free(index);
        }
        if(n) CHECK(memcmp(a,saved,n*sizeof(*a))==0);
    }
    ++cases;
}
int main(void) {
    int32_t a[1000];size_t n,j,code,total;int r;
    const int32_t values[]={-2,0,1,3};
    const int32_t example[]={9,8,9,15,12,13,8,8,9};
    const int32_t extremes[]={INT32_MIN,INT32_MAX,0,-1,INT32_MIN,INT32_MAX,1};
    tspr3_frequency *index=NULL;size_t k=0;
    verify(example,9);verify(extremes,7);
    for(n=0,total=1;n<=8;++n,total*=4) for(code=0;code<total;++code) {
        size_t v=code;
        for(j=0;j<n;++j) { a[j]=values[v%4];v/=4; }
        verify(a,n);
    }
    for(n=0;n<=300;++n) for(r=0;r<10;++r) {
        for(j=0;j<n;++j) a[j]=(int32_t)((int64_t)rnd()-INT64_C(2147483648));
        verify(a,n);
    }
    for(r=0;r<6;++r) {
        for(j=0;j<1000;++j) {
            if(r==0) a[j]=(int32_t)j;
            else if(r==1) a[j]=(int32_t)(1000-j);
            else if(r==2) a[j]=(int32_t)(rnd()%8);
            else if(r==3) a[j]=(int32_t)(j%3 ? j*7 : 0);
            else if(r==4) a[j]=INT32_MIN;
            else a[j]=j%2 ? INT32_MIN : INT32_MAX;
        }
        verify(a,1000);
    }
    CHECK(tspr3_tri_sort(NULL,1,TSPR3_TRI_BIN,a,NULL)==TSPR3_INVALID);
    CHECK(tspr3_tri_sort(a,1,TSPR3_TRI_BIN,NULL,NULL)==TSPR3_INVALID);
    CHECK(tspr3_tri_sort(a,SIZE_MAX,TSPR3_TRI_BIN,a,NULL)==TSPR3_INVALID);
    CHECK(tspr3_tri_sort(NULL,0,(tspr3_tri_mode)99,NULL,NULL)==TSPR3_INVALID);
    CHECK(tspr3_tri_index(a,1,TSPR3_TRI_BASE,&index,&k,NULL)==TSPR3_INVALID);
    CHECK(tspr3_tri_index(a,1,TSPR3_TRI_BIN,NULL,&k,NULL)==TSPR3_INVALID);
    CHECK(tspr3_tri_index(a,1,TSPR3_TRI_BIN,&index,NULL,NULL)==TSPR3_INVALID);
    CHECK(tspr3_tri_sort(NULL,0,TSPR3_TRI_BIN,NULL,NULL)==TSPR3_OK);
    printf("OK: %zu input cases; BASE/COUNTS/BIN and compact counts; invalid arguments.\n",cases);
    return 0;
}
