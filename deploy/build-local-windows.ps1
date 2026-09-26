param(
    [string]$QtRoot = "$env:USERPROFILE\Qt\6.10.1\msvc2022_64",
    [int]$Jobs = 8
)

$ErrorActionPreference = 'Stop'
$projectDir = Split-Path $PSScriptRoot -Parent
$buildDir = Join-Path $PSScriptRoot 'build'
$originalPath = $env:PATH

function Invoke-CMake {
    param([string[]]$CMakeArgs)
    # Windows PowerShell treats native stderr as errors when output is redirected.
    $ErrorActionPreference = 'Continue'
    & cmake @CMakeArgs
    if ($LASTEXITCODE -ne 0) { throw "CMake failed ($LASTEXITCODE)." }
}

try {
    # Use the project's Python tools without changing the system PATH.
    $env:PATH = "$projectDir\.venv\Scripts;$originalPath"
    if (-not (Test-Path "$QtRoot\lib\cmake\Qt6\Qt6Config.cmake")) {
        throw "Qt was not found at $QtRoot. Pass -QtRoot with your Qt MSVC directory."
    }

    # Build missing dependencies locally when Amnezia's package server is unavailable.
    Invoke-CMake @('-S', $projectDir, '-B', $buildDir, '-D_CONAN_INSTALL_ARGS=-r=conancenter',
        '-DCONAN_INSTALL_BUILD_CONFIGURATIONS=Release', '-DCMAKE_BUILD_TYPE=Release',
        "-DCMAKE_PREFIX_PATH=$QtRoot")

    Invoke-CMake @('--build', $buildDir, '--config', 'Release', '--parallel', "$Jobs")

    # Stage the executable and DLLs in the build folder without installing services.
    Invoke-CMake @('--install', $buildDir, '--config', 'Release', '--prefix', "$buildDir\stage")
    Write-Host "Build ready: $buildDir\stage"
}
finally {
    $env:PATH = $originalPath
}
