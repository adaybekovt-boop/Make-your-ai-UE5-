#include "Rules/MaiDurableFile.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv){
    if(argc!=2)return 2;
    const auto root=std::filesystem::u8path(argv[1]);std::string error,bytes;
    auto require=[](bool okay,const char* what){if(!okay)throw std::runtime_error(what);};
    const auto path=(root/"company.mai.json").u8string();
    require(mai::WriteDurableFile(path,"first",error),"initial durable write");
    require(mai::ReadBoundedFile(path,bytes,error)&&bytes=="first","initial readback");
    require(mai::WriteDurableFile(path,"second",error),"atomic replacement");
    require(mai::ReadBoundedFile(path+".bak",bytes,error)&&bytes=="first","prior slot backup");
    require(mai::ReadBoundedFile(path,bytes,error)&&bytes=="second","replacement readback");
    require(!mai::WriteDurableFile(path,"",error),"empty write rejection");
    require(!mai::WriteDurableFile(path,std::string(8*1024*1024+1,'X'),error),"oversize write rejection");
    require(mai::ReadBoundedFile(path,bytes,error)&&bytes=="second","failed writes preserve previous slot");
    std::filesystem::create_directory(root/"directory-target");
    require(!mai::WriteDurableFile((root/"directory-target").u8string(),"bad",error),"directory target rejection");
    require(!mai::ReadBoundedFile((root/"missing").u8string(),bytes,error),"missing read rejection");
    std::ofstream huge(root/"huge",std::ios::binary);huge.seekp(8*1024*1024);huge.put('x');huge.close();
    require(!mai::ReadBoundedFile((root/"huge").u8string(),bytes,error),"oversize read rejection");
    std::cout<<"DURABLE_FILE_PASS: 11 assertions; actual filesystem writes/readback/backup/rejections\n";
}
