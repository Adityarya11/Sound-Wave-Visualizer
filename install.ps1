<#
.SYNOPSIS
    Builds Sound Wave Visualizer and installs it as the `visualizer` command.

.DESCRIPTION
    Copies visualizer.exe into %LOCALAPPDATA%\Programs\Visualizer and puts that
    folder on the user PATH, so `visualizer` runs from any shell - the same way
    Rainmeter-style tools install themselves. No admin rights needed: nothing is
    written outside the user profile.

.PARAMETER SkipBuild
    Install the exe already in build-mingw instead of rebuilding.

.PARAMETER Uninstall
    Remove the install directory and take it back off PATH.

.EXAMPLE
    .\install.ps1
    .\install.ps1 -Uninstall
#>
[CmdletBinding()]
param(
    [switch]$SkipBuild,
    [switch]$Uninstall
)

$ErrorActionPreference = 'Stop'

$root       = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir   = Join-Path $root 'build-mingw'
$installDir = Join-Path $env:LOCALAPPDATA 'Programs\Visualizer'
$mingwBin   = 'C:\msys64\mingw64\bin'

function Write-Step($message) { Write-Host "==> $message" -ForegroundColor Magenta }
function Write-Note($message) { Write-Host "    $message" -ForegroundColor DarkGray }

function Get-UserPath { [Environment]::GetEnvironmentVariable('Path', 'User') }

function Remove-FromUserPath($dir) {
    $entries = (Get-UserPath) -split ';' | Where-Object { $_ -and $_.TrimEnd('\') -ne $dir.TrimEnd('\') }
    [Environment]::SetEnvironmentVariable('Path', ($entries -join ';'), 'User')
}

function Stop-Running {
    $running = Get-Process visualizer -ErrorAction SilentlyContinue
    if ($running) {
        Write-Note 'Stopping the running instance first.'
        $running | Stop-Process -Force
        Start-Sleep -Milliseconds 400
    }
}

if ($Uninstall) {
    Write-Step 'Uninstalling'
    Stop-Running
    if (Test-Path $installDir) {
        Remove-Item $installDir -Recurse -Force
        Write-Note "Removed $installDir"
    }
    Remove-FromUserPath $installDir
    Write-Note 'Removed from user PATH.'
    Write-Host 'Done. Config is still at %APPDATA%\Visualizer - delete it by hand if you want it gone.' -ForegroundColor Green
    return
}

# --- Build ----------------------------------------------------------------
if (-not $SkipBuild) {
    if (-not (Test-Path $mingwBin)) {
        throw "MinGW not found at $mingwBin. Install MSYS2, then: pacman -S mingw-w64-x86_64-sfml"
    }

    Write-Step 'Building (Release)'
    Stop-Running

    # Prepend MinGW so CMake finds this gcc and its SFML rather than anything
    # else on PATH.
    $env:PATH = "$mingwBin;$env:PATH"

    if (-not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))) {
        cmake -S $root -B $buildDir -G 'MinGW Makefiles' `
            -DCMAKE_BUILD_TYPE=Release `
            -DCMAKE_C_COMPILER="$mingwBin/gcc.exe" `
            -DCMAKE_CXX_COMPILER="$mingwBin/g++.exe" `
            -DCMAKE_MAKE_PROGRAM="$mingwBin/mingw32-make.exe" `
            -DCMAKE_PREFIX_PATH='C:/msys64/mingw64'
        if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
    }

    cmake --build $buildDir -j 4
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
}

$exe = Join-Path $buildDir 'visualizer.exe'
if (-not (Test-Path $exe)) { throw "No executable at $exe. Run without -SkipBuild." }

# --- Install --------------------------------------------------------------
Write-Step "Installing to $installDir"
Stop-Running
New-Item -ItemType Directory -Force -Path $installDir | Out-Null

Copy-Item $exe $installDir -Force
# Shipped alongside so colours can be retuned without a rebuild; the exe also
# carries a compiled-in copy and works fine without it.
Copy-Item (Join-Path $root 'assets\aurora.frag') $installDir -Force

$size = [math]::Round((Get-Item (Join-Path $installDir 'visualizer.exe')).Length / 1MB, 1)
Write-Note "visualizer.exe ($size MB, no DLLs required)"

# --- PATH -----------------------------------------------------------------
$userPath = Get-UserPath
$onPath = ($userPath -split ';' | Where-Object { $_.TrimEnd('\') -eq $installDir.TrimEnd('\') }).Count -gt 0

if ($onPath) {
    Write-Note 'Already on PATH.'
} else {
    Write-Step 'Adding to user PATH'
    [Environment]::SetEnvironmentVariable('Path', "$userPath;$installDir".Trim(';'), 'User')
    Write-Note 'Added. Open a new terminal for it to take effect.'
}

# Make it usable in this session too.
if (-not ($env:PATH -split ';' | Where-Object { $_.TrimEnd('\') -eq $installDir.TrimEnd('\') })) {
    $env:PATH = "$env:PATH;$installDir"
}

Write-Host ''
Write-Host 'Installed. Run it with:  visualizer' -ForegroundColor Green
Write-Host ''
Write-Host '  Ctrl+Alt+V     lock / unlock for moving' -ForegroundColor Gray
Write-Host '  Ctrl+Alt+B     switch orb <-> bars'      -ForegroundColor Gray
Write-Host '  Ctrl+Alt+Up    larger'                   -ForegroundColor Gray
Write-Host '  Ctrl+Alt+Down  smaller'                  -ForegroundColor Gray
Write-Host '  Ctrl+Alt+Q     quit'                     -ForegroundColor Gray
Write-Host ''
Write-Host 'While unlocked: drag to move, scroll to resize, Esc to lock again.' -ForegroundColor Gray
Write-Host 'Run `visualizer --help` any time to see this again.' -ForegroundColor Gray
