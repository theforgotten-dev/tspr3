#ifndef TSPR3_CPP_H
#define TSPR3_CPP_H
#include "tspr3.h"
#include <vector>
#include <stdexcept>
#include <new>
namespace tspr3 {
using options=tspr3_options;
using report=tspr3_report;
inline options default_options() { options result;tspr3_options_init(&result);return result; }
inline void check(tspr3_result result) {
    if(result==TSPR3_SUCCESS) return;
    if(result==TSPR3_ALLOCATION_FAILURE) throw std::bad_alloc();
    if(result==TSPR3_HEAP_LIMIT) throw std::length_error(tspr3_result_message(result));
    throw std::invalid_argument(tspr3_result_message(result));
}
struct sorted_result { std::vector<int32_t> values;report diagnostics{}; };
struct counted_result { std::vector<tspr3_frequency_i32> values;report diagnostics{}; };
inline sorted_result sort(const int32_t *input,std::size_t n,options config=default_options()) {
    if(n && !input) throw std::invalid_argument("null input");
    sorted_result result;result.values.resize(n);
    check(tspr3_sort_i32(input,n,result.values.data(),&config,&result.diagnostics));
    return result;
}
inline sorted_result sort(const std::vector<int32_t> &input,options config=default_options()) {
    return sort(input.data(),input.size(),config);
}
inline counted_result frequencies(const int32_t *input,std::size_t n,options config=default_options()) {
    if(n && !input) throw std::invalid_argument("null input");
    counted_result result;std::size_t used=0;result.values.resize(n);
    check(tspr3_counts_i32(input,n,result.values.data(),&used,&config,&result.diagnostics));
    result.values.resize(used);return result;
}
inline counted_result frequencies(const std::vector<int32_t> &input,options config=default_options()) {
    return frequencies(input.data(),input.size(),config);
}
}
#endif
