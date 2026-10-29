@echo off
setlocal EnableExtensions
set "FAILS=0"
for /f "delims=" %%c in ('where cmake') do (set "CMAKE=%%c" & goto gotcmake)
:gotcmake
powershell -NoProfile -Command "Compress-Archive -Path '%~dp0fc\dep' -DestinationPath '%~dp0dep.zip' -Force"
set "DEPZIP=%~dp0dep.zip"
set "DEPZIP=%DEPZIP:\=/%"
mkdir C:\aliasbase 2>NUL
mklink /J C:\aliasbase\A "%~dp0fc" >NUL
mklink /J C:\aliasbase\B "%~dp0fc" >NUL
rem first build through alias A
"%CMAKE%" -S C:\aliasbase\A -B C:\aliasbase\A\build "-DDEP_ZIP=%DEPZIP%" -G Ninja -DCMAKE_C_COMPILER=gcc > "%TEMP%\first.txt" 2>&1 || (echo FAIL first configure & type "%TEMP%\first.txt" & exit /b 1)
rem the old alias no longer resolves (folder renamed: its junction points nowhere)
rmdir C:\aliasbase\A
set "BUILD_DIR=C:\aliasbase\B\build"
rem the alias step drops the top-level cache configured through A
del /q "%BUILD_DIR%\CMakeCache.txt" & rmdir /s /q "%BUILD_DIR%\CMakeFiles" 2>NUL
rem without the fix: expect the reported error
"%CMAKE%" -S C:\aliasbase\B -B "%BUILD_DIR%" "-DDEP_ZIP=%DEPZIP%" -G Ninja -DCMAKE_C_COMPILER=gcc > "%TEMP%\nofix.txt" 2>&1
findstr /C:"is different than the directory" "%TEMP%\nofix.txt" >NUL && (echo PASS reproduced the player's error) || (echo FAIL could not reproduce & powershell -NoProfile -Command "Get-Content $env:TEMP\nofix.txt | Select-Object -Last 15 | ForEach-Object { 'NOFIX ' + $_ }" & set "FAILS=1")
del /q "%BUILD_DIR%\CMakeCache.txt" & rmdir /s /q "%BUILD_DIR%\CMakeFiles" 2>NUL
rem the helper's new lines
if not exist "%BUILD_DIR%\CMakeCache.txt" del /q "%BUILD_DIR%\build.ninja" 2>NUL
if not exist "%BUILD_DIR%\CMakeCache.txt" for /d %%D in ("%BUILD_DIR%\_deps\*-subbuild") do rmdir /s /q "%%D"
"%CMAKE%" -S C:\aliasbase\B -B "%BUILD_DIR%" "-DDEP_ZIP=%DEPZIP%" -G Ninja -DCMAKE_C_COMPILER=gcc > "%TEMP%\fix.txt" 2>&1
if errorlevel 1 (echo FAIL configure after fix & type "%TEMP%\fix.txt" & set "FAILS=1") else (echo PASS configure after fix)
exit /b %FAILS%
