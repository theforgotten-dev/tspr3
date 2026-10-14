#include <tspr3/tspr3.hpp>
#include <iostream>
int main() {
    try {
        const std::vector<int32_t> input{9,8,9,15,12,13,8,8,9};
        const auto result=tspr3::sort(input);
        for(const auto value:result.values) std::cout<<value<<' ';
        std::cout<<'\n';
        const auto counts=tspr3::frequencies(input);
        for(const auto &item:counts.values) std::cout<<'('<<item.value<<','<<item.count<<") ";
        std::cout<<'\n';return 0;
    } catch(const std::exception &error) { std::cerr<<error.what()<<'\n';return 1; }
}
