param(
    [switch]$Regenerate,
    [switch]$Run,
    [string]$Rom = 'Super Bomberman 5 (USA).sfc',
    [ValidateSet('LLE', 'HLE')][string]$FrameImpl = 'LLE'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$packRoot = Join-Path $projectRoot '.tools/toolchain'
$cmakeExe = Join-Path $packRoot 'bin/cmake.exe'
if (-not (Test-Path -LiteralPath $cmakeExe)) {
    throw 'Local toolchain missing. See docs/BUILD_WINDOWS.md.'
}
$env:CARGO_HOME = Join-Path $projectRoot '.tools/cargo'
$env:RUSTUP_HOME = Join-Path $projectRoot '.tools/rustup'
$env:RUSTUP_TOOLCHAIN = '1.97.1-x86_64-pc-windows-gnullvm'
$env:CARGO_TARGET_X86_64_PC_WINDOWS_GNULLVM_LINKER = Join-Path $packRoot 'bin/x86_64-w64-mingw32-clang.exe'
$env:PATH = "$env:CARGO_HOME/bin;$packRoot/bin;$env:PATH"
$env:RETCOMM_TOOLCHAIN_DIR = $packRoot
$romPath = if ([IO.Path]::IsPathRooted($Rom)) { $Rom } else { Join-Path $projectRoot $Rom }
$buildPath = Join-Path $projectRoot ('build-' + $FrameImpl.ToLowerInvariant())

Push-Location $projectRoot
try {
    if ($Regenerate) {
        & 'C:/Program Files/Git/bin/bash.exe' tools/regen.sh --rom ($romPath -replace '\\', '/')
        if ($LASTEXITCODE -ne 0) { throw 'ROM code generation failed.' }
    }
    & $cmakeExe -S $projectRoot -B $buildPath -G Ninja `
        '-DCMAKE_BUILD_TYPE=Release' "-DSNESRECOMP_FRAME_IMPL=$FrameImpl" `
        "-DCMAKE_C_COMPILER=$packRoot/bin/clang.exe" `
        "-DCMAKE_CXX_COMPILER=$packRoot/bin/clang++.exe" `
        "-DCMAKE_MAKE_PROGRAM=$packRoot/bin/ninja.exe" `
        "-DSDL3_DIR=$packRoot/deps/lib/cmake/SDL3"
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
    & $cmakeExe --build $buildPath --parallel 4
    if ($LASTEXITCODE -ne 0) { throw 'Native build failed.' }
    Copy-Item -LiteralPath (Join-Path $buildPath 'SuperBomberman5Recomp.exe') -Destination (Join-Path $projectRoot 'SuperBomberman5Recomp.exe') -Force
    if ($Run) {
        & (Join-Path $buildPath 'SuperBomberman5SNESRecomp.exe') $romPath
        if ($LASTEXITCODE -ne 0) { throw "Game exited with code $LASTEXITCODE." }
    }
} finally {
    Pop-Location
}
