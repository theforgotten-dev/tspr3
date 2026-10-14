#include <tspr3/tspr3.hpp>
#include <algorithm>
#include <iostream>
int main() {
    const std::vector<int32_t> input{9,8,9,15,12,13,8,8,9};
    auto expected=input;std::sort(expected.begin(),expected.end());
    for(int method=0;method<4;++method) {
        auto options=tspr3::default_options();options.algorithm=static_cast<tspr3_algorithm>(method);
        if(tspr3::sort(input,options).values!=expected) return 1;
    }
    const auto counts=tspr3::frequencies(input);
    if(counts.values.size()!=5 || counts.values[0].value!=8 || counts.values[0].count!=3) return 2;
    if(!tspr3::sort(std::vector<int32_t>{}).values.empty()) return 3;
    bool rejected=false;
    try { tspr3::sort(nullptr,1); } catch(const std::invalid_argument &) { rejected=true; }
    if(!rejected) return 4;
    rejected=false;
    auto options=tspr3::default_options();options.auxiliary_heap_limit=0;
    try { tspr3::sort(input,options); } catch(const std::length_error &) { rejected=true; }
    if(!rejected) return 5;
    std::cout<<"OK: C++17 wrappers link to C library and handle errors.\n";return 0;
}
