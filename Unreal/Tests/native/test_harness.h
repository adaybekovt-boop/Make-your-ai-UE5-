#pragma once
#include "Core/MaiDomain.h"
#include <iostream>
inline int assertions=0,failures=0,cases=0;
#define CHECK(x) do {++assertions;if(!(x)){++failures;std::cerr<<__FILE__<<":"<<__LINE__<<" CHECK failed: " #x "\n";}} while(false)
#define OK(x) do {const auto result=(x);++assertions;if(!result.ok){++failures;std::cerr<<__FILE__<<":"<<__LINE__<<" " #x ": "<<result.message<<"\n";}} while(false)
void RunCampaignTests();
