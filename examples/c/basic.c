#include <tspr3/tspr3.h>
#include <stdio.h>
#include <inttypes.h>
int main(void) {
    const int32_t input[]={9,8,9,15,12,13,8,8,9};
    int32_t output[9];tspr3_report report;size_t i;
    tspr3_result status=tspr3_sort_i32(input,9,output,NULL,&report);
    if(status!=TSPR3_SUCCESS) { fprintf(stderr,"%s\n",tspr3_result_message(status));return 1; }
    printf("TSPR3 %s, selected=%d\n",tspr3_version(),(int)report.selected);
    for(i=0;i<9;++i) printf("%" PRId32 "%s",output[i],i==8 ? "\n" : " ");
    return 0;
}
