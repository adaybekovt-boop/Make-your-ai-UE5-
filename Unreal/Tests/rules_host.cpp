#include "Rules/MaiRulesVM.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
int main(int argc,char** argv) {
    if(argc!=2) { std::cerr<<"rules_host trusted-bundle.js\n";return 2; }
    std::ifstream file(argv[1],std::ios::binary);
    if(!file) {std::cerr<<"Bundle not found\n";return 2;}
    const std::string bundle((std::istreambuf_iterator<char>(file)),{});
    mai::RulesVM vm;std::string error,response;
    if(!vm.Open(bundle,0x12345678,error)) {std::cerr<<error<<'\n';return 3;}
    for(std::string line;std::getline(std::cin,line);) {
        if(!vm.Call(line,response,error)) {std::cerr<<error<<'\n';return 4;}
        std::cout<<response<<'\n'<<std::flush;
    }
    return 0;
}
