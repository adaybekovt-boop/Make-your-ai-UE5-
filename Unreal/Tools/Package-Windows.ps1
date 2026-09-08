#requires -Version 7.2
[CmdletBinding()]
param([Parameter(Mandatory)][string]$UE_ROOT,[Parameter(Mandatory)][string]$Output,[ValidateSet('Development','Shipping')][string]$Configuration='Shipping')
$ErrorActionPreference='Stop'
$Repo=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$Project=Join-Path $Repo 'Unreal/MakeYourAI/MakeYourAI.uproject'
$Descriptor=Get-Content (Join-Path $Repo 'Unreal/MakeYourAI/Content/Rules/city-content.json') -Raw|ConvertFrom-Json
if($Descriptor.cityMap -notmatch '^/Game/Generated/CityV4/R_[a-f0-9]+/L_City_DAY$'){throw 'Invalid generated city descriptor'}
if(Test-Path -LiteralPath $Output){throw 'Choose a new output directory; existing builds are never overwritten'}
$Automation=Join-Path $UE_ROOT 'Engine/Build/BatchFiles/RunUAT.bat'
if(!(Test-Path -LiteralPath $Automation)){throw 'Unreal AutomationTool missing'}
& $Automation BuildCookRun "-project=$Project" -noP4 -platform=Win64 "-clientconfig=$Configuration" -build -cook "-map=$($Descriptor.cityMap)+/Engine/Maps/Entry" -stage -pak -archive "-archivedirectory=$Output" -prereqs -utf8output -unattended
if($LASTEXITCODE -ne 0){throw "Packaging failed: $LASTEXITCODE"}
$Executable=Get-ChildItem -LiteralPath $Output -Filter MakeYourAI.exe -Recurse | Select-Object -First 1
if(!$Executable){throw 'Packaging returned without a launch executable'}
Write-Host "Packaged executable: $($Executable.FullName)"
