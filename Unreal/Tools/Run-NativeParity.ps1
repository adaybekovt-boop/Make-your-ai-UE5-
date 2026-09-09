#requires -Version 7.2
[CmdletBinding()]
param(
    [ValidateSet('all','audit','pin-engine','generate','build','automation','content','visual')][string]$Stage='all',
    [string]$UE_ROOT=$env:UE_ROOT,
    [string]$Blender=$env:BLENDER_EXE,
    [string]$CityManifest,
    [switch]$EnableNanite,
    [ValidateSet('all','1280x720','1600x900','1920x1080')][string]$VisualResolution='all',
    [switch]$CameraSweep,
    [int]$TimeoutSeconds=3600
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$Repo=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$ProjectDir=Join-Path $Repo 'Unreal/MakeYourAI'
$Project=Join-Path $ProjectDir 'MakeYourAI.uproject'
$RunId=(Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssZ')+'-'+[guid]::NewGuid().ToString('N').Substring(0,8)
$Output=Join-Path $ProjectDir "Saved/Verification/$RunId"
[void](New-Item -ItemType Directory -Path $Output)
$Report=[ordered]@{ run=$RunId; stage=$Stage; steps=@(); engine=$null; ueBuild='NOT_RUN'; automation='NOT_RUN'; content='NOT_RUN'; gpuCapture='NOT_RUN'; humanPlaytest=$false; fullMigrationVerified=$false }
$Before=@{}
function Hash([string]$File){(Get-FileHash -LiteralPath $File -Algorithm SHA256).Hash.ToLowerInvariant()}
function Write-Json([string]$File,$Value){$Value|ConvertTo-Json -Depth 40|Set-Content -LiteralPath $File -Encoding utf8NoBOM}
function Tool([string]$Name){$Result=Get-Command $Name -ErrorAction SilentlyContinue;if(!$Result){throw "Required tool not installed: $Name"};$Result.Source}
function Run([string]$Name,[string]$Exe,[string[]]$Arguments,[string]$Working=$Repo){
    $Stdout=Join-Path $Output ($Name+'.stdout.log');$Stderr=Join-Path $Output ($Name+'.stderr.log')
    $Info=[Diagnostics.ProcessStartInfo]::new();$Info.FileName=$Exe;$Info.WorkingDirectory=$Working
    $Info.UseShellExecute=$false;$Info.RedirectStandardOutput=$true;$Info.RedirectStandardError=$true
    foreach($Argument in $Arguments){[void]$Info.ArgumentList.Add($Argument)}
    $Process=[Diagnostics.Process]::new();$Process.StartInfo=$Info
    $Started=(Get-Date).ToUniversalTime().ToString('o');Write-Host "[$Name] $Exe"
    if(!$Process.Start()){throw "Could not start $Name"}
    $ReadOut=$Process.StandardOutput.ReadToEndAsync();$ReadErr=$Process.StandardError.ReadToEndAsync()
    $TimedOut=!$Process.WaitForExit($TimeoutSeconds*1000)
    if($TimedOut){$Process.Kill($true);$Process.WaitForExit()}
    [IO.File]::WriteAllText($Stdout,$ReadOut.GetAwaiter().GetResult());[IO.File]::WriteAllText($Stderr,$ReadErr.GetAwaiter().GetResult())
    $Code=$Process.ExitCode
    $Report.steps+=@{name=$Name;started=$Started;executable=$Exe;arguments=$Arguments;exitCode=$Code;timedOut=$TimedOut;stdout=$Stdout;stderr=$Stderr}
    Write-Json (Join-Path $Output 'result.json') $Report
    if($TimedOut -or $Code -ne 0){throw "$Name failed (exit $Code, timeout $TimedOut). Inspect $Stdout and $Stderr"}
    return $Stdout
}
function Discover-Engine {
    $Candidates=[Collections.Generic.List[string]]::new()
    if($UE_ROOT){$Candidates.Add($UE_ROOT)}else{
        $Launcher=Join-Path $env:ProgramData 'Epic/UnrealEngineLauncher/LauncherInstalled.dat'
        if(Test-Path $Launcher){foreach($Item in (Get-Content $Launcher -Raw|ConvertFrom-Json).InstallationList){if($Item.AppName -like 'UE_*'){$Candidates.Add($Item.InstallLocation)}}}
        $Registry='HKCU:\Software\Epic Games\Unreal Engine\Builds'
        if(Test-Path $Registry){foreach($Property in (Get-ItemProperty $Registry).PSObject.Properties){if($Property.Name -notlike 'PS*' -and $Property.Value -is [string]){$Candidates.Add($Property.Value)}}}
    }
    $Valid=@(foreach($Candidate in ($Candidates|Select-Object -Unique)){
        $VersionFile=Join-Path $Candidate 'Engine/Build/Build.version'
        if(Test-Path $VersionFile){$V=Get-Content $VersionFile -Raw|ConvertFrom-Json;if($V.MajorVersion -eq 5){[pscustomobject]@{root=$Candidate;version=([version]"$($V.MajorVersion).$($V.MinorVersion).$($V.PatchVersion)");hash=(Hash $VersionFile);data=$V}}}
    })
    if(!$Valid.Count){throw 'No UE5 installation discovered. Set UE_ROOT to an installed engine root.'}
    if($Valid.Count -gt 1){throw 'Several UE installations found. Set UE_ROOT explicitly; no engine migration will be guessed.'}
    return $Valid[0]
}
function Require-Pin {
    if(!(Test-Path $Pin)){throw 'Engine is not pinned. Run -Stage pin-engine first.'}
    $Old=Get-Content $Pin -Raw|ConvertFrom-Json
    if($Old.buildVersionSHA256 -ne $Engine.hash){throw 'Installed UE differs from the saved pin. Review an explicit migration; no pin or uproject overwritten.'}
}
function Verify-Automation([string]$Dir){
    $Index=Join-Path $Dir 'index.json';if(!(Test-Path $Index)){throw 'Automation produced no index.json; process exit 0 is insufficient.'}
    $IndexData=Get-Content $Index -Raw|ConvertFrom-Json
    $Expected=@(Get-ChildItem (Join-Path $ProjectDir 'Source/MakeYourAI/Private/Tests') -Filter '*.cpp'|ForEach-Object{[regex]::Matches((Get-Content $_ -Raw),'"(MakeYourAI\.[^"]+)"')}|ForEach-Object{$_.Groups[1].Value}|Sort-Object -Unique)
    if(!$Expected.Count){throw 'No expected Automation tests found in source'}
    foreach($Name in $Expected){$Found=@($IndexData.tests|Where-Object {$_.fullTestPath -eq $Name});if($Found.Count -ne 1 -or $Found[0].state -ne 'Success'){throw "Missing/failed automation test: $Name"}}
    if($IndexData.failed -gt 0){throw 'Automation has failed tests'}
    return $Expected.Count
}
$Exit=0
try {
    if(!$IsWindows){throw 'This runner is for Windows and PowerShell 7.2+.'}
    if($TimeoutSeconds -lt 30 -or $TimeoutSeconds -gt 86400){throw 'TimeoutSeconds must be 30..86400.'}
    $Git=Tool 'git';$Node=Tool 'node';$Cmake=Tool 'cmake'
    foreach($File in @($Project,(Join-Path $ProjectDir 'Config/DefaultEngine.ini'),(Join-Path $Repo 'Unreal/CityV4/city-v4.blend'),(Join-Path $Repo 'Unreal/CityV4/city-v4.fbx'))){if(Test-Path $File){$Before[$File]=Hash $File}}
    $null=Run 'audit-git' $Git @('status','--porcelain=v1')
    $null=Run 'audit-head' $Git @('rev-parse','HEAD')
    $VsWhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if(!(Test-Path $VsWhere)){throw 'Visual Studio Installer/vswhere not found. C++ build tools are required.'}
    $CompilerLog=Run 'audit-compiler' $VsWhere @('-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-find','VC/Tools/MSVC/**/bin/Hostx64/x64/cl.exe')
    if(!(Get-Content $CompilerLog -Raw).Trim()){throw 'MSVC x64 C++ compiler not found.'}
    $GPU=@(Get-CimInstance Win32_VideoController|Select-Object Name,DriverVersion,AdapterRAM)
    Write-Json (Join-Path $Output 'host.json') @{gpu=$GPU;gpuRenderVerified=$false;pwsh=$PSVersionTable.PSVersion.ToString();sourceBefore=$Before}
    $Engine=Discover-Engine;$Report.engine=@{root=$Engine.root;version=$Engine.version.ToString();buildVersion=$Engine.data;buildVersionSHA256=$Engine.hash}
    $Editor=Join-Path $Engine.root 'Engine/Binaries/Win64/UnrealEditor.exe'
    $Commandlet=Join-Path $Engine.root 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
    $UBT=Join-Path $Engine.root 'Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll'
    foreach($File in @($Editor,$Commandlet,$UBT)){if(!(Test-Path $File)){throw "Incomplete installed engine: $File"}}
    $Dotnet=@(Get-ChildItem (Join-Path $Engine.root 'Engine/Binaries/ThirdParty/DotNet') -Filter dotnet.exe -Recurse|Where-Object{$_.FullName -match 'win-x64'})
    if($Dotnet.Count -ne 1){throw 'Expected one engine-bundled Windows x64 dotnet; inspect installation.'}
    $Pin=Join-Path $ProjectDir 'Saved/NativeParity/engine.lock.json'
    $Stages=if($Stage -eq 'all'){@('audit','pin-engine','generate','build','automation','content','visual')}else{@($Stage)}
    foreach($Current in $Stages){switch($Current){
        'audit' {Write-Json (Join-Path $Output 'engine-audit.json') $Report.engine}
        'pin-engine' {
            if(Test-Path $Pin){Require-Pin}else{[void](New-Item -ItemType Directory -Force -Path (Split-Path $Pin));Write-Json $Pin @{buildVersionSHA256=$Engine.hash;version=$Engine.version.ToString()}}
            # Explicit engine tools are used. Never rewrite the author's EngineAssociation.
        }
        'generate' {
            Require-Pin
            $Deps=Join-Path $Repo 'Unreal/ThirdParty/QuickJS';$Source=Join-Path $Deps 'source'
            $Commit='1ab8676f4b6d6d669baeb5f21790fb9734636a20'
            if(!(Test-Path $Source)){
                $null=Run 'quickjs-clone' $Git @('clone','--no-checkout','--filter=blob:none','https://github.com/quickjs-ng/quickjs.git',$Source)
                $null=Run 'quickjs-fetch' $Git @('-C',$Source,'fetch','--depth=1','origin',$Commit)
                $null=Run 'quickjs-checkout' $Git @('-C',$Source,'checkout','--detach',$Commit)
            }
            $HeadLog=Run 'quickjs-head' $Git @('-C',$Source,'rev-parse','HEAD')
            $DirtyLog=Run 'quickjs-worktree' $Git @('-C',$Source,'status','--porcelain')
            $HeadText=[IO.File]::ReadAllText($HeadLog)
            $DirtyText=[IO.File]::ReadAllText($DirtyLog)
            if($HeadText.Trim() -ne $Commit -or ![string]::IsNullOrWhiteSpace($DirtyText)){throw 'QuickJS source differs from pin; existing files retained.'}
            $BuildDir=Join-Path $Deps 'build/Win64'
            $null=Run 'quickjs-configure' $Cmake @('-S',$Source,'-B',$BuildDir,'-A','x64','-DBUILD_SHARED_LIBS=OFF','-DCMAKE_POSITION_INDEPENDENT_CODE=ON')
            $null=Run 'quickjs-build' $Cmake @('--build',$BuildDir,'--config','Release','--target','qjs')
            $NpmCmd=Tool 'npm.cmd';$NpmCli=Join-Path (Split-Path $NpmCmd) 'node_modules/npm/bin/npm-cli.js'
            if(!(Test-Path $NpmCli)){throw 'Cannot locate npm CLI beside npm.cmd; use a standard Node installation.'}
            $null=Run 'npm-ci' $Node @($NpmCli,'ci','--ignore-scripts')
            $null=Run 'rules-typecheck' $Node @('node_modules/typescript/bin/tsc','-p','Unreal/Rules/tsconfig.json')
            $null=Run 'rules-bundle' $Node @('Unreal/Rules/build.mjs')
            $null=Run 'generate-project' $Dotnet[0].FullName @($UBT,'-ProjectFiles',"-Project=$Project",'-Game','-Engine')
        }
        'build' {
            Require-Pin
            $null=Run 'ue-editor-build' $Dotnet[0].FullName @($UBT,'MakeYourAIEditor','Win64','Development',"-Project=$Project",'-WaitMutex','-NoHotReloadFromIDE')
            $Report.ueBuild='PASS'
        }
        'automation' {
            Require-Pin;$Automation=Join-Path $Output 'automation'
            $null=Run 'ue-automation' $Commandlet @($Project,'-unattended','-nop4','-NullRHI','-nosplash','-nosound','-ExecCmds=Automation RunTests MakeYourAI.','-TestExit=Automation Test Queue Empty',"-ReportExportPath=$Automation",'-stdout','-FullStdOutLogOutput')
            $Count=Verify-Automation $Automation;$Report.automation="PASS ($Count explicitly matched tests; no GPU)"
        }
        'content' {
            Require-Pin
            if(!$CityManifest){
                if(!$Blender){$Cmd=Get-Command blender -ErrorAction SilentlyContinue;if($Cmd){$Blender=$Cmd.Source}}
                if(!$Blender -or !(Test-Path $Blender)){throw 'Set BLENDER_EXE or pass -CityManifest from a completed v2 export.'}
                $Source=Join-Path $Repo 'Unreal/CityV4/city-v4.blend';$SourceHash=Hash $Source
                $Script=Join-Path $PSScriptRoot 'export_cityv4_instances.py';$ExportHash=Hash $Script
                $Derived=Join-Path $Repo ('Unreal/Derived/CityV4/'+$SourceHash.Substring(0,12)+'-'+$ExportHash.Substring(0,12))
                $null=Run 'city-export' $Blender @('--background','--factory-startup','--disable-autoexec','--python',$Script,'--','--source',$Source,'--out',$Derived)
                $CityManifest=Join-Path $Derived 'manifest.json'
            }
            if(!(Test-Path $CityManifest)){throw 'Derived CityV4 manifest does not exist.'}
            $env:MAI_CITY_MANIFEST=(Resolve-Path $CityManifest).Path
            $env:MAI_CITY_NANITE=if($EnableNanite){'1'}else{'0'}
            $env:MAI_CONTENT_REPORT=Join-Path $Output 'city-content.json'
            $Script=Join-Path $PSScriptRoot 'build_city_content.py'
            $null=Run 'ue-city-content' $Commandlet @($Project,'-unattended','-nop4','-d3d12','-sm6','-run=pythonscript',"-script=$Script",'-stdout','-FullStdOutLogOutput')
            if(!(Test-Path $env:MAI_CONTENT_REPORT) -or !(Get-Content $env:MAI_CONTENT_REPORT -Raw|ConvertFrom-Json).imported){throw 'Content build did not produce a successful receipt.'}
            $Report.content='PASS (real assets imported; visual match not asserted)'
        }
        'visual' {
            Require-Pin
            if(!$GPU.Count){throw 'No display adapter detected; GPU capture is not verified.'}
            foreach($Size in @(@(1280,720),@(1600,900),@(1920,1080))){
                if($VisualResolution -ne 'all' -and "$($Size[0])x$($Size[1])" -ne $VisualResolution){continue}
                $Report.visualResolution=$VisualResolution
                $Session=$RunId+'-'+$Size[0]+'x'+$Size[1]
                $Args=@($Project,'-game','-windowed',"-ResX=$($Size[0])","-ResY=$($Size[1])",'-ForceRes','-d3d12','-sm6','-nosplash',"-MaiCaptureSession=$Session",'-stdout','-FullStdOutLogOutput')
                if($CameraSweep){$Args+='-MaiCameraSweep'}
                foreach($Mode in @('capture','resume')){
                    $RunArgs=$Args;if($Mode -eq 'resume'){$RunArgs+=@('-MaiCaptureResume')}
                    $null=Run "visual-$Session-$Mode" $Editor $RunArgs
                    $Path=Join-Path $ProjectDir "Saved/Verification/$Session/$Mode/result.json"
                    if(!(Test-Path $Path)){throw 'No runtime capture report. A zero exit code is insufficient.'}
                    $Capture=Get-Content $Path -Raw|ConvertFrom-Json
                    if(!$Capture.completed -or !$Capture.captures.Count){throw "Capture failed: $($Capture.error)"}
                    if($Mode -eq 'resume' -and !$Capture.processRestartLoadVerified){throw 'Actual process restart did not verify saved company.'}
                    foreach($Image in $Capture.captures){
                        $Png=[IO.File]::ReadAllBytes($Image)
                        if($Png.Length -lt 24){throw 'Incomplete screenshot'}
                        $Width=([long]$Png[16]*16777216)+([long]$Png[17]*65536)+([long]$Png[18]*256)+$Png[19]
                        $Height=([long]$Png[20]*16777216)+([long]$Png[21]*65536)+([long]$Png[22]*256)+$Png[23]
                        if($Width -ne $Size[0] -or $Height -ne $Size[1]){throw 'Actual capture does not match requested resolution'}
                    }
                }
                Copy-Item -LiteralPath (Join-Path $ProjectDir "Saved/Verification/$Session") -Destination $Output -Recurse
            }
            $Report.gpuCapture='CAPTURED; images require visual review. Full gameplay and pointer playthrough are NOT verified.'
        }
    }}
}catch{$Exit=2;$Report.error=$_.Exception.Message;Write-Host $_.ScriptStackTrace;Write-Error -Message $_ -ErrorAction Continue}
finally{
    foreach($File in $Before.Keys){if(!(Test-Path $File) -or (Hash $File) -ne $Before[$File]){$Report.error="Authored source changed: $File";$Exit=2}}
    $Report.exitCode=$Exit;Write-Json (Join-Path $Output 'result.json') $Report
    Write-Host "Evidence: $Output"
}
exit $Exit
