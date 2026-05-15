[CmdletBinding()]
param(
    [switch]$Configure,
    [switch]$Build,
    [switch]$Test,
    [switch]$All,
    [Alias("h")]
    [switch]$Help
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$script:ConfigurePreset = "windows"
$script:BuildPreset = "windows-release"
$script:TestPreset = "windows-tests"

function Show-Usage {
    @"
Usage:
  .\test.ps1
  .\test.ps1 -All
  .\test.ps1 -Configure
  .\test.ps1 -Build
  .\test.ps1 -Test
  .\test.ps1 -Configure -Build
  .\test.ps1 -Build -Test
  .\test.ps1 -Help

Windows platform-neutral fallback:
  - Configure preset: windows
  - Build preset: windows-release
  - Test preset: windows-tests

Notes:
  - This wrapper does not call Bash.
  - This wrapper only runs the Windows platform-neutral fallback flow.
  - Linux-specific groups and Linux Bash-first retained-artifact flows remain in ./test.sh.
"@ | Write-Host
}

function Invoke-CheckedCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$StepName,
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [Parameter(Mandatory = $true)]
        [string[]]$ArgumentList
    )

    Write-Host ""
    Write-Host "============================================================"
    Write-Host $StepName
    Write-Host "============================================================"
    Write-Host "$FilePath $($ArgumentList -join ' ')"

    & $FilePath @ArgumentList
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        throw "$StepName failed with exit code $exitCode."
    }
}

if ($Help) {
    Show-Usage
    exit 0
}

$selectedStepCount = 0
if ($Configure) { $selectedStepCount++ }
if ($Build) { $selectedStepCount++ }
if ($Test) { $selectedStepCount++ }
if ($All) { $selectedStepCount++ }

if ($selectedStepCount -eq 0) {
    $All = $true
}

$projectRoot = Split-Path -Parent $PSCommandPath

Write-Host "Windows platform-neutral fallback validation"
Write-Host "Project root: $projectRoot"
Write-Host "Linux primary entry remains: ./test.sh"
Write-Host "PowerShell fallback presets: $script:ConfigurePreset / $script:BuildPreset / $script:TestPreset"

Push-Location $projectRoot
try {
    if ($All -or $Configure) {
        Invoke-CheckedCommand `
            -StepName "Configuring with Windows preset" `
            -FilePath "cmake" `
            -ArgumentList @("--preset", $script:ConfigurePreset)
    }

    if ($All -or $Build) {
        Invoke-CheckedCommand `
            -StepName "Building with Windows release preset" `
            -FilePath "cmake" `
            -ArgumentList @("--build", "--preset", $script:BuildPreset)
    }

    if ($All -or $Test) {
        Invoke-CheckedCommand `
            -StepName "Running Windows platform-neutral CTest preset" `
            -FilePath "ctest" `
            -ArgumentList @("--preset", $script:TestPreset)
    }

    Write-Host ""
    Write-Host "Windows platform-neutral fallback validation completed successfully."
    exit 0
}
catch {
    Write-Error $_
    exit 1
}
finally {
    Pop-Location
}
