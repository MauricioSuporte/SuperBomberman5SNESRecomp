@echo off
rem Super Bomberman 5 Recompiled - netplay with diagnostics.
rem Copy this file next to SuperBomberman5Recomp.exe and open the game through it.
rem Logs go to saves\netplay\ (net_diag_*.jsonl and netplay_log.txt) in the
rem game folder (or its build\ folder): send that whole netplay folder after a match.
setlocal
cd /d "%~dp0"
set SNES_NET_DIAG=1
set SNES_NET_DIAG_HZ=4
if exist "%~dp0SuperBomberman5Recomp.exe" (
  start "" "%~dp0SuperBomberman5Recomp.exe" %*
) else if exist "%~dp0build-lle\SuperBomberman5SNESRecomp.exe" (
  start "" "%~dp0build-lle\SuperBomberman5SNESRecomp.exe" %*
) else (
  echo SuperBomberman5Recomp.exe not found next to this file.
  pause
)
