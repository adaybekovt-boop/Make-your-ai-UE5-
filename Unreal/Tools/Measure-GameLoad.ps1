#requires -Version 7.2
param([Parameter(Mandatory)][int]$GameProcessId,[Parameter(Mandatory)][string]$Output,[ValidateRange(10,60)][int]$Seconds=30)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $Output){throw 'Existing measurement is preserved'}
$Process=Get-Process -Id $GameProcessId
if($Process.ProcessName -notmatch 'MakeYourAI|UnrealEditor'){throw 'Target is not the game'}
$Samples=@();$Logical=[Environment]::ProcessorCount
$PreviousCpu=$Process.TotalProcessorTime.TotalSeconds;$PreviousTime=[Diagnostics.Stopwatch]::StartNew()
for($I=0;$I -lt $Seconds;$I++){
    Start-Sleep -Seconds 1
    $Process.Refresh();$Elapsed=$PreviousTime.Elapsed.TotalSeconds
    $Cpu=$Process.TotalProcessorTime.TotalSeconds
    $Gpu=& nvidia-smi --query-gpu=utilization.gpu,memory.used,memory.total,power.draw,temperature.gpu --format=csv,noheader,nounits
    $Samples+= [pscustomobject]@{second=$I+1;processCPUPercent=100*($Cpu-$PreviousCpu)/($Elapsed*$Logical);workingSetMiB=$Process.WorkingSet64/1MB;gpuAllDesktop=$Gpu}
    $PreviousCpu=$Cpu;$PreviousTime.Restart()
}
$Report=[ordered]@{scope='Game process CPU/RAM; GPU values cover the whole desktop, not this process alone. Not a frame-time benchmark.';process=$Process.ProcessName;logicalProcessors=$Logical;samples=$Samples}
$Report|ConvertTo-Json -Depth 5|Set-Content -LiteralPath $Output -Encoding utf8NoBOM
Write-Host "Load measurement: $Output"
