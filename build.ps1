#Requires -Version 5.1
<#
.SYNOPSIS
  GBS-Control local build helper (Windows / PowerShell).

.EXAMPLES
  .\build.ps1                    # ESP8266 firmware (d1_mini)
  .\build.ps1 -Board esp32dev
  .\build.ps1 -Target upload -Port COM5
  .\build.ps1 -Target firmware-only
  .\build.ps1 -Help
#>
param(
    [ValidateSet("firmware", "firmware-only", "webui", "upload", "monitor", "clean", "submodules", "help")]
    [string]$Target = "firmware",
    [string]$Board = "d1_mini",
    [string]$Port = "",
    [switch]$Help
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
Set-Location $Root

function Write-Help {
    Write-Host @"

GBS-Control build.ps1

  .\build.ps1 [-Target firmware] [-Board d1_mini]
  .\build.ps1 -Target firmware-only          skip web UI
  .\build.ps1 -Target upload [-Port COM5]
  .\build.ps1 -Target monitor
  .\build.ps1 -Target webui                  needs Node.js + Git Bash (html2h.sh)
  .\build.ps1 -Target submodules
  .\build.ps1 -Target clean

Boards: d1_mini, esp32dev, esp32-s3-devkitc-1, esp32-c3-devkitm-1, esp32-c6-devkitc-1

Setup (once):
  py -3 -m pip install -U platformio
  Add Python Scripts to PATH (optional):
    %LOCALAPPDATA%\Programs\Python\Python313\Scripts

Output: .pio\build\<board>\firmware.bin

"@
}

function Get-Pio {
    if (Get-Command pio -ErrorAction SilentlyContinue) { return "pio" }
    if (Get-Command platformio -ErrorAction SilentlyContinue) { return "platformio" }
    return "py -3 -m platformio"
}

function Invoke-Pio {
    param([string[]]$PioArgs)
    $prevEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    if ((Get-Pio) -eq "py -3 -m platformio") {
        & py -3 -m platformio @PioArgs 2>&1 | Write-Host
        $code = $LASTEXITCODE
    } else {
        $pio = Get-Pio
        & $pio @PioArgs 2>&1 | Write-Host
        $code = $LASTEXITCODE
    }
    $ErrorActionPreference = $prevEap
    if ($code -ne 0) { exit $code }
}

if ($Help -or $Target -eq "help") {
    Write-Help
    exit 0
}

switch ($Target) {
    "submodules" {
        git submodule update --init --recursive
        exit 0
    }
    "webui" {
        if (-not (Get-Command npm -ErrorAction SilentlyContinue)) {
            Write-Error "npm not found. Install Node.js from https://nodejs.org/"
        }
        Push-Location public
        npm install
        npm run build
        Pop-Location
        exit 0
    }
    "clean" {
        Invoke-Pio @("run", "-t", "clean")
        if (Test-Path webui.html) { Remove-Item webui.html }
        exit 0
    }
    "firmware-only" {
        Invoke-Pio @("run", "-e", $Board)
        $bin = Join-Path $Root ".pio\build\$Board\firmware.bin"
        if (Test-Path $bin) {
            Write-Host "`nOK: $bin" -ForegroundColor Green
        }
        exit 0
    }
    "firmware" {
        & $PSCommandPath -Target firmware-only -Board $Board
        exit $LASTEXITCODE
    }
    "upload" {
        $args = @("run", "-e", $Board, "-t", "upload")
        if ($Port) { $args += @("--upload-port", $Port) }
        Invoke-Pio $args
        exit 0
    }
    "monitor" {
        $args = @("device", "monitor", "-e", $Board)
        if ($Port) { $args += @("--port", $Port) }
        Invoke-Pio $args
        exit 0
    }
}

Write-Help
exit 1
