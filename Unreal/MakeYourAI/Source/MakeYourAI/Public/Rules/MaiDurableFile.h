#pragma once
#include <string>
namespace mai {
// UTF-8 paths. Write-through staging + atomic replacement; previous save retained.
bool WriteDurableFile(const std::string& path,const std::string& bytes,std::string& error);
bool ReadBoundedFile(const std::string& path,std::string& bytes,std::string& error);
}
