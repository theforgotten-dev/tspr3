#include "tspr3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#ifdef _WIN32
#include <windows.h>
#endif

static double now_seconds(void) {
#ifdef _WIN32
    LARGE_INTEGER counter, frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
#endif
}

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static uint32_t state = 20261013;
static uint32_t random_value(void) { state ^= state << 13; state ^= state >> 17; state ^= state << 5; return state; }
static int compare(const void *a, const void *b) {
    int32_t x = *(const int32_t *)a, y = *(const int32_t *)b;
    return (x > y) - (x < y);
}
static void verify(const int32_t *a, size_t n) {
    int32_t out[501], expected[501], saved[501];
    tspr3_record records[167];
    tspr3_frequency *index;
    size_t k, expected_k = 0, position = 0;
    unsigned char seen[167] = {0};
    size_t i, m = n / 3 + (n % 3 != 0);
    CHECK(n <= 501);
    if (n) { memcpy(expected, a, n * sizeof(*a)); memcpy(saved, a, n * sizeof(*a)); }
    qsort(expected, n, sizeof(*expected), compare);
    CHECK(tspr3_full(a, n, out) == TSPR3_OK);
    CHECK(memcmp(out, expected, n * sizeof(*out)) == 0);
    CHECK(tspr3_radix(a, n, out) == TSPR3_OK);
    CHECK(memcmp(out, expected, n * sizeof(*out)) == 0);
    CHECK(tspr3_auto(a, n, out, NULL) == TSPR3_OK);
    CHECK(memcmp(out, expected, n * sizeof(*out)) == 0);
    CHECK(tspr3_freq(a, n, out, &k) == TSPR3_OK);
    CHECK(memcmp(out, expected, n * sizeof(*out)) == 0);
    for (i = 0; i < n; ++i) if (!i || expected[i] != expected[i-1]) ++expected_k;
    CHECK(k == expected_k);
    CHECK(tspr3_frequency_index(a, n, &index, &k) == TSPR3_OK);
    CHECK(k == expected_k);
    for (i = 0; i < k; ++i) {
        size_t end = position;
        CHECK(index[i].count > 0);
        while (end < n && expected[end] == index[i].value) ++end;
        CHECK(index[i].count == end - position);
        position = end;
    }
    CHECK(position == n);
    free(index);
    CHECK(tspr3_sum(a, n, records) == TSPR3_OK);
    for (i = 0; i < m; ++i) {
        size_t idx = records[i].original_index, j;
        int64_t sum = 0;
        CHECK(idx < m && !seen[idx]); seen[idx] = 1;
        for (j = idx * 3; j < n && j - idx * 3 < 3; ++j) sum += a[j];
        CHECK(sum == records[i].sum);
        if (i) CHECK(records[i-1].sum < records[i].sum ||
            (records[i-1].sum == records[i].sum && records[i-1].original_index < idx));
    }
    if (n) CHECK(memcmp(a, saved, n * sizeof(*a)) == 0);
}
static void tests(void) {
    const int32_t base[] = {9,2,7,12,4,1,8,5,3,6,11,10};
    const int32_t short_input[] = {5,1}, partial[] = {4,2,9,1};
    const int32_t negatives[] = {-5,2,1,0,-2,4}, duplicates[] = {3,3,3,3,3,3};
    const int32_t limits[] = {INT32_MAX,INT32_MAX,INT32_MAX,INT32_MIN,INT32_MIN,INT32_MIN};
    int32_t a[501]; size_t n, j; int repetition;
    verify(NULL,0); verify(base,12); verify(short_input,2); verify(partial,4);
    verify(negatives,6); verify(duplicates,6); verify(limits,6);
    CHECK(tspr3_full(NULL,1,a) == TSPR3_INVALID);
    CHECK(tspr3_sum(a,1,NULL) == TSPR3_INVALID);
    CHECK(tspr3_freq(NULL,1,a,NULL) == TSPR3_INVALID);
    CHECK(tspr3_freq(a,1,NULL,NULL) == TSPR3_INVALID);
    CHECK(tspr3_freq(NULL,0,NULL,NULL) == TSPR3_OK);
    CHECK(tspr3_radix(NULL,0,NULL) == TSPR3_OK);
    CHECK(tspr3_radix(NULL,1,a) == TSPR3_INVALID);
    CHECK(tspr3_radix(a,1,NULL) == TSPR3_INVALID);
    CHECK(tspr3_radix(a,SIZE_MAX,a) == TSPR3_INVALID);
    CHECK(tspr3_auto(NULL,0,NULL,NULL) == TSPR3_OK);
    CHECK(tspr3_auto(NULL,1,a,NULL) == TSPR3_INVALID);
    CHECK(tspr3_auto(a,1,NULL,NULL) == TSPR3_INVALID);
    CHECK(tspr3_auto(a,SIZE_MAX,a,NULL) == TSPR3_INVALID);
    CHECK(tspr3_freq(a,SIZE_MAX,a,NULL) == TSPR3_INVALID);
    CHECK(tspr3_frequency_index(a,1,NULL,&n) == TSPR3_INVALID);
    for (n = 0; n <= 500; ++n) for (repetition = 0; repetition < 10; ++repetition) {
        for (j = 0; j < n; ++j) a[j] = (int32_t)(random_value() % 2001) - 1000;
        verify(a,n);
    }
    /* Force repeated growth, many identical keys and widely spaced signed keys. */
    for (j = 0; j < 501; ++j) a[j] = (int32_t)j * 100003 - 25000000;
    verify(a,501);
    for (j = 0; j < 501; ++j) a[j] = INT32_MIN;
    verify(a,501);
    for (j = 0; j < 501; ++j) a[j] = j % 2 ? INT32_MAX : INT32_MIN;
    verify(a,501);
    {
        int32_t result[501]; tspr3_method method;
        size_t budget = tspr3_auto_frequency_budget(501);
        CHECK(budget >= 2 && budget <= 125);
        for (j=0;j<501;++j) a[j]=(int32_t)j;
        verify(a,501); CHECK(tspr3_auto(a,501,result,&method)==TSPR3_OK && method==TSPR3_METHOD_ASCENDING);
        for (j=0;j<501;++j) a[j]=(int32_t)(501-j);
        verify(a,501); CHECK(tspr3_auto(a,501,result,&method)==TSPR3_OK && method==TSPR3_METHOD_DESCENDING);
        for (j=0;j<501;++j) a[j]=(int32_t)(j%budget);
        verify(a,501); CHECK(tspr3_auto(a,501,result,&method)==TSPR3_OK && method==TSPR3_METHOD_FREQ);
        for (j=0;j<501;++j) a[j]=(int32_t)(j%(budget+1));
        verify(a,501); CHECK(tspr3_auto(a,501,result,&method)==TSPR3_OK && method==TSPR3_METHOD_FULL);
        /* Duplicates in the prefix do not hide the distinct suffix. */
        for (j=0;j<501;++j) a[j]=j<250 ? (int32_t)(j%2) : (int32_t)j;
        verify(a,501); CHECK(tspr3_auto(a,501,result,&method)==TSPR3_OK && method==TSPR3_METHOD_FULL);
    }
    for (j=0;j<501;++j) a[j]=(int32_t)((j/3)*3+2-j%3);
    verify(a,501);
    {
        int32_t result[501]; tspr3_method method;
        CHECK(tspr3_auto(a,501,result,&method)==TSPR3_OK && method==TSPR3_METHOD_SEPARATED_BLOCKS);
    }
    puts("OK: 16 deterministic + 5010 randomized cases, SUM/FULL/FREQ/RADIX/AUTO and index; invalid arguments and selection boundaries.");
}
static void large_paths(void) {
    const size_t n=8192;
    int32_t *a=malloc(n*sizeof(*a)), *expected=malloc(n*sizeof(*a)), *out=malloc(n*sizeof(*a));
    int scenario; size_t i;
    const tspr3_method paths[]={TSPR3_METHOD_DENSE,TSPR3_METHOD_FREQ,TSPR3_METHOD_RADIX,TSPR3_METHOD_RUNS,TSPR3_METHOD_DOMINANT,TSPR3_METHOD_RADIX,TSPR3_METHOD_RUNS};
    CHECK(a && expected && out);
    for(scenario=0;scenario<7;++scenario) {
        tspr3_method method;
        for(i=0;i<n;++i) {
            if(scenario==0) a[i]=(int32_t)(i%13)-6;
            else if(scenario==1) a[i]=i%2 ? INT32_MIN : INT32_MAX;
            else if(scenario==2) a[i]=(int32_t)((int64_t)random_value()-INT64_C(2147483648));
            else if(scenario==3) a[i]=(int32_t)((i+4096)%8192);
            else if(scenario==4) a[i]=i%100 ? 0 : INT32_MAX;
            else if(scenario==5) a[i]=i%16==8 ? 0 : (int32_t)((int64_t)random_value()-INT64_C(2147483648));
            else a[i]=(int32_t)(i<n/2 ? i : n-i);
        }
        memcpy(expected,a,n*sizeof(*a)); qsort(expected,n,sizeof(*a),compare);
        CHECK(tspr3_auto(a,n,out,&method)==TSPR3_OK);
        CHECK(method==paths[scenario]);
        CHECK(memcmp(expected,out,n*sizeof(*a))==0);
        CHECK(tspr3_radix(a,n,out)==TSPR3_OK);
        CHECK(memcmp(expected,out,n*sizeof(*a))==0);
    }
    free(a);free(expected);free(out);
    puts("OK: 7 large path cases, including misleading samples and descending natural runs.");
}
static double median(double *samples) {
    int i, j;
    for (i=1;i<5;++i) for(j=i;j>0 && samples[j]<samples[j-1];--j) {
        double t=samples[j]; samples[j]=samples[j-1]; samples[j-1]=t;
    }
    return samples[2];
}
static void benchmark(void) {
    /* First three sizes match calibration; remaining three are held out. */
    const size_t sizes[] = {1000,10000,100000,3000,30000,200000};
    const char *names[] = {"random","ascending","descending","duplicates","disjoint","interleaved","k64","k512","k2048","k8192","k32768"};
    size_t s, i; int d, r;
    puts("distribution,n,k,method,FULL_ms,FREQ_ms,AUTO_ms,qsort_ms (median of 5; allocation included, qsort input copy excluded)");
    for (s=0;s<sizeof(sizes)/sizeof(sizes[0]);++s) {
        size_t n=sizes[s];
        int32_t *a=malloc(n*sizeof(*a)), *out=malloc(n*sizeof(*out)), *ref=malloc(n*sizeof(*ref));
        CHECK(a && out && ref);
        for (d=0;d<11;++d) {
            double full[5], freq[5], automatic[5], baseline[5];
            tspr3_method method = TSPR3_METHOD_FULL;
            size_t k = 0;
            for(i=0;i<n;++i) {
                if(d==0) a[i]=(int32_t)(random_value()%2000001)-1000000;
                else if(d==1) a[i]=(int32_t)i;
                else if(d==2) a[i]=(int32_t)(n-i);
                else if(d==3) a[i]=(int32_t)(random_value()%8);
                else if(d==4) a[i]=(int32_t)((i/3)*3+2-i%3);
                else if(d==5) a[i]=(int32_t)((i%3)*(n/3+1)+i/3);
                else {
                    const uint32_t cardinalities[] = {64,512,2048,8192,32768};
                    a[i]=(int32_t)(random_value()%cardinalities[d-6]);
                }
            }
            for(r=0;r<5;++r) {
                double t;
                memcpy(ref,a,n*sizeof(*a));
                t=now_seconds(); qsort(ref,n,sizeof(*ref),compare);
                baseline[r]=1000.0*(now_seconds()-t);
                t=now_seconds(); CHECK(tspr3_full(a,n,out)==TSPR3_OK);
                full[r]=1000.0*(now_seconds()-t);
                CHECK(memcmp(ref,out,n*sizeof(*out))==0);
                t=now_seconds(); CHECK(tspr3_auto(a,n,out,&method)==TSPR3_OK);
                automatic[r]=1000.0*(now_seconds()-t);
                CHECK(memcmp(ref,out,n*sizeof(*out))==0);
                t=now_seconds(); CHECK(tspr3_freq(a,n,out,&k)==TSPR3_OK);
                freq[r]=1000.0*(now_seconds()-t);
                CHECK(memcmp(ref,out,n*sizeof(*out))==0);
            }
            printf("%s,%zu,%zu,%d,%.3f,%.3f,%.3f,%.3f\n",names[d],n,k,(int)method,median(full),median(freq),median(automatic),median(baseline));
        }
        free(a); free(out); free(ref);
    }
}
int main(int argc, char **argv) {
    tests();
    large_paths();
    if(argc>1 && strcmp(argv[1],"--bench")==0) benchmark();
    return 0;
}
