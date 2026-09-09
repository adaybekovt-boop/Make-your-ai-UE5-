@echo off
setlocal
cd /d "%~dp0\..\.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1
if not exist "Unreal\Tests\.out" mkdir "Unreal\Tests\.out"
cl /nologo /std:c++17 /EHsc /utf-8 /MD /O2 /I Unreal\MakeYourAI\Source\MakeYourAI\Public /I Unreal\ThirdParty\QuickJS\source /FoUnreal\Tests\.out\ /FeUnreal\Tests\.out\rules_host.exe Unreal\MakeYourAI\Source\MakeYourAI\Private\Rules\MaiRulesVM.cpp Unreal\Tests\rules_host.cpp Unreal\ThirdParty\QuickJS\build\Win64\Release\qjs.lib /link ws2_32.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /utf-8 /MD /I Unreal\MakeYourAI\Source\MakeYourAI\Public /FoUnreal\Tests\.out\ /FeUnreal\Tests\.out\durable-tests.exe Unreal\MakeYourAI\Source\MakeYourAI\Private\Rules\MaiDurableFile.cpp Unreal\Tests\native\durable_main.cpp
exit /b %errorlevel%
