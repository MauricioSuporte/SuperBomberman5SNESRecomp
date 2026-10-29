@echo off
setlocal EnableExtensions
rem Lines copied from snesrecomp's Windows rebuild helper (codegen host).
for /f "delims=" %%c in ('where cmake') do (set "CMAKE=%%c" & goto gotcmake)
:gotcmake
set "ROOT=%~dp0proj"
set "TARGET=demo"
set "FAILS=0"

rem 1) cache dropped by the alias step, build.ninja left behind -> must be removed
set "BUILD_DIR=%~dp0Área de Trabalho\b1"
mkdir "%BUILD_DIR%" 2>NUL
echo stale> "%BUILD_DIR%\build.ninja"
if not exist "%BUILD_DIR%\CMakeCache.txt" del /q "%BUILD_DIR%\build.ninja" 2>NUL
if exist "%BUILD_DIR%\build.ninja" (echo FAIL stale build.ninja kept & set "FAILS=1") else (echo PASS stale build.ninja removed)

rem 2) failing build (no cache) must set errorlevel
powershell -NoProfile -Command "& $env:CMAKE --build $env:BUILD_DIR --parallel --target $env:TARGET 2>&1 | ForEach-Object { [string]$_ } | Tee-Object -FilePath (Join-Path $env:BUILD_DIR 'snesrecomp_rebuild.log') -Append; exit $LASTEXITCODE"
if errorlevel 1 (echo PASS failed build detected) else (echo FAIL failed build passed as success & set "FAILS=1")
if exist "%BUILD_DIR%\snesrecomp_rebuild.log" (echo PASS log written & type "%BUILD_DIR%\snesrecomp_rebuild.log") else (echo FAIL no log & set "FAILS=1")

rem 3) the old pipe form, for contrast (expected to MISS the failure)
"%CMAKE%" --build "%BUILD_DIR%" --parallel --target "%TARGET%" 2>&1 | powershell -NoProfile -Command "$input | Tee-Object -FilePath '%BUILD_DIR%\old.log' -Append"
if errorlevel 1 (echo note: old form detected it) else (echo note: old form missed the failure, as reported)

rem 4) a successful build must leave errorlevel 0
set "BUILD_DIR=%~dp0Área de Trabalho\b2"
"%CMAKE%" -S "%ROOT%" -B "%BUILD_DIR%" >NUL
powershell -NoProfile -Command "& $env:CMAKE --build $env:BUILD_DIR --parallel --target $env:TARGET 2>&1 | ForEach-Object { [string]$_ } | Tee-Object -FilePath (Join-Path $env:BUILD_DIR 'snesrecomp_rebuild.log') -Append; exit $LASTEXITCODE"
if errorlevel 1 (echo FAIL good build reported as failure & set "FAILS=1") else (echo PASS good build ok)

rem 5) a compile error must be detected
echo int main(void){ return x; }> "%ROOT%\main.c"
powershell -NoProfile -Command "& $env:CMAKE --build $env:BUILD_DIR --parallel --target $env:TARGET 2>&1 | ForEach-Object { [string]$_ } | Tee-Object -FilePath (Join-Path $env:BUILD_DIR 'snesrecomp_rebuild.log') -Append; exit $LASTEXITCODE"
if errorlevel 1 (echo PASS compile error detected) else (echo FAIL compile error passed & set "FAILS=1")
exit /b %FAILS%
