#include "Rules/MaiDurableFile.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv){
    if(argc<2||argc>3)return 2;
    const auto root=std::filesystem::u8path(argv[1]);std::string error,bytes;
    int assertions=0;
    auto require=[&](bool okay,const char* what){++assertions;if(!okay)throw std::runtime_error(what);};
    const auto path=(root/"company.mai.json").u8string();
    const mai::SaveValidator valid=[](const std::string& s,std::string& why){
        const bool okay=s=="{\"revision\":1}"||s=="{\"revision\":2}"||s=="{\"revision\":3}";
        if(!okay)why="Invalid test save schema";
        return okay;
    };
    const std::string first="{\"revision\":1}",second="{\"revision\":2}",third="{\"revision\":3}";
    bool recovered=false;
    if(argc==3){
        const std::string mode=argv[2];
        if(mode=="export"){
            require(mai::WriteDurableFile(path,first,error,valid),"process export first");
            require(mai::WriteDurableFile(path,second,error,valid),"process export second");
        }else if(mode=="resume"){
            require(mai::ReadValidatedSave(path,valid,bytes,recovered,error)&&bytes==second&&!recovered,"fresh process reads primary");
            std::ofstream(path,std::ios::binary|std::ios::trunc)<<"{truncated";
        }else if(mode=="recover"){
            require(mai::ReadValidatedSave(path,valid,bytes,recovered,error)&&bytes==first&&recovered,"fresh process recovers backup");
        }else return 2;
        std::cout<<"DURABLE_PROCESS_PASS "<<mode<<" assertions="<<assertions<<"\n";return 0;
    }
    require(mai::WriteDurableFile(path,"first",error),"legacy durable write");
    require(mai::ReadBoundedFile(path,bytes,error)&&bytes=="first","legacy readback");
    require(mai::WriteDurableFile(path,"second",error),"legacy atomic replacement");
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
    require(!mai::ReadValidatedSave(path,{},bytes,recovered,error),"validation cannot be skipped during recovery");
    require(mai::WriteDurableFile(path,first,error,valid),"validated first write over corrupt legacy primary");
    require(mai::WriteDurableFile(path,second,error,valid),"validated second write");
    require(mai::ReadValidatedSave(path,valid,bytes,recovered,error)&&bytes==second&&!recovered,"primary preferred");
    require(!mai::WriteDurableFile(path,"{}",error,valid),"malformed candidate rejected before replacing");
    require(mai::ReadBoundedFile(path,bytes,error)&&bytes==second,"semantic rejection retains primary");
    require(mai::ReadBoundedFile(path+".bak",bytes,error)&&bytes==first,"semantic rejection retains backup");
    std::ofstream(path,std::ios::binary|std::ios::trunc)<<"{truncated";
    require(mai::ReadValidatedSave(path,valid,bytes,recovered,error)&&bytes==first&&recovered,"corrupt primary fallback");
    require(mai::ReadBoundedFile(path,bytes,error)&&bytes=="{truncated","recovery does not silently overwrite evidence");
    require(mai::WriteDurableFile(path,third,error,valid),"save following recovery");
    require(mai::ReadBoundedFile(path+".bak",bytes,error)&&bytes==first,"corrupt primary never rotates over healthy backup");
    std::filesystem::remove(path);
    require(mai::ReadValidatedSave(path,valid,bytes,recovered,error)&&bytes==first&&recovered,"missing primary fallback");
    std::ofstream(path+".bak",std::ios::binary|std::ios::trunc)<<"wrong schema";
    require(!mai::ReadValidatedSave(path,valid,bytes,recovered,error)&&bytes.empty()&&!recovered,"both unavailable or invalid fail closed");
    for(const auto& entry:std::filesystem::directory_iterator(root))
        require(entry.path().filename().string().find(".pending-")==std::string::npos,"no abandoned staging files");
    std::cout<<"DURABLE_FILE_PASS: "<<assertions<<" assertions; real filesystem, validation, recovery and backup preservation\n";
}
