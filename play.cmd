@echo off
setlocal
set "GAME_EXE=%~dp0build-lle\SuperBomberman5SNESRecomp.exe"
if not exist "%GAME_EXE%" (
  echo Compile primeiro: powershell -ExecutionPolicy Bypass -File "%~dp0tools\build-windows.ps1" -Regenerate
  pause
  exit /b 1
)
"%GAME_EXE%" --launcher --rom "%~dp0Super Bomberman 5 (USA).sfc"
