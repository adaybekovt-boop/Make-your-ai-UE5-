@echo off
setlocal
cd /d "%~dp0\..\.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1
if not exist "Unreal\Tests\.out" mkdir "Unreal\Tests\.out"
cl /nologo /std:c++17 /EHsc /utf-8 /W3 /I Unreal\MakeYourAI\Source\MakeYourAI\Public /FoUnreal\Tests\.out\ /FeUnreal\Tests\.out\core-tests.exe Unreal\MakeYourAI\Source\MakeYourAI\Private\Core\MaiCatalog.cpp Unreal\MakeYourAI\Source\MakeYourAI\Private\Core\MaiSimulation.cpp Unreal\MakeYourAI\Source\MakeYourAI\Private\Core\MaiCodec.cpp Unreal\MakeYourAI\Source\MakeYourAI\Private\Campaign\MaiCampaign.cpp Unreal\MakeYourAI\Source\MakeYourAI\Private\Campaign\MaiCampaignCodec.cpp Unreal\MakeYourAI\Source\MakeYourAI\Private\Campaign\MaiCampaignRules.cpp Unreal\MakeYourAI\Source\MakeYourAI\Private\Campaign\MaiCampaignTick.cpp Unreal\MakeYourAI\Source\MakeYourAI\Private\Campaign\MaiCampaignValidation.cpp Unreal\Tests\native\main.cpp Unreal\Tests\native\campaign_tests.cpp
if errorlevel 1 exit /b 1
Unreal\Tests\.out\core-tests.exe
if errorlevel 1 exit /b 1
Unreal\Tests\.out\core-tests.exe --fixtures > Unreal\Tests\.out\fixtures.json
exit /b %errorlevel%
