#include <jni.h>
#include <tspr3/tspr3.h>
#include <stdlib.h>
_Static_assert(sizeof(jint)==sizeof(int32_t),"JNI requires 32-bit integers");
static void fail(JNIEnv *env,const char *type,const char *message) {
    jclass cls=(*env)->FindClass(env,type);
    if(cls) (*env)->ThrowNew(env,cls,message);
}
JNIEXPORT jintArray JNICALL Java_org_tspr3_NativeTspr3_sortNative(JNIEnv *env,jclass cls,
    jintArray input,jint algorithm,jlong heapLimit) {
    jsize n,i;int32_t *in=NULL,*out=NULL;jint *transfer=NULL;jintArray result=NULL;tspr3_options options;tspr3_result status;
    (void)cls;
    if(!input || algorithm<0 || algorithm>3 || heapLimit<0) { fail(env,"java/lang/IllegalArgumentException","invalid native argument");return NULL; }
    n=(*env)->GetArrayLength(env,input);
    if(n) {
        if((size_t)n>SIZE_MAX/sizeof(*in)) { fail(env,"java/lang/OutOfMemoryError","array size overflow");return NULL; }
        in=malloc((size_t)n*sizeof(*in));out=malloc((size_t)n*sizeof(*out));transfer=malloc((size_t)n*sizeof(*transfer));
        if(!in || !out || !transfer) { fail(env,"java/lang/OutOfMemoryError","native buffers");goto done; }
        (*env)->GetIntArrayRegion(env,input,0,n,transfer);
        if((*env)->ExceptionCheck(env)) goto done;
        for(i=0;i<n;++i) in[i]=(int32_t)transfer[i];
    }
    tspr3_options_init(&options);options.algorithm=(tspr3_algorithm)algorithm;options.auxiliary_heap_limit=(size_t)heapLimit;
    status=tspr3_sort_i32(in,(size_t)n,out,&options,NULL);
    if(status!=TSPR3_SUCCESS) {
        fail(env,status==TSPR3_INVALID_ARGUMENT?"java/lang/IllegalArgumentException":"java/lang/OutOfMemoryError",tspr3_result_message(status));goto done;
    }
    result=(*env)->NewIntArray(env,n);
    if(result && n) {
        for(i=0;i<n;++i) transfer[i]=(jint)out[i];
        (*env)->SetIntArrayRegion(env,result,0,n,transfer);if((*env)->ExceptionCheck(env)) result=NULL;
    }
done:
    free(in);free(out);free(transfer);return result;
}
