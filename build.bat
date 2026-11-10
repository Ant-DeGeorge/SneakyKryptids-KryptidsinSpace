@echo off
rem Kryptids in Space: The Sneaky Kryptid - Windows build (needs GBDK-2020 4.5.0 at C:\gbdk)
rem Generated sources (src\assets*.c, src\music\*.c, script_data.c, stage_defs.c) are included,
rem so Python is only needed if you change art, music or the script (see README).
setlocal enabledelayedexpansion
if "%GBDK_HOME%"=="" set GBDK_HOME=C:\gbdk
set LCC=%GBDK_HOME%\bin\lcc
set SRC=src\main0.c src\core.c src\sound.c src\script_data.c src\script_scenes.c src\stage_defs.c src\objs.c src\stage.c src\s1.c src\s2.c src\s3.c src\s4.c src\cockpit.c src\scenes.c
for %%f in (src\assets_b*.c) do set SRC=!SRC! %%f
for %%f in (src\cockpit_b*.c) do set SRC=!SRC! %%f
for %%f in (src\music\*.c) do set SRC=!SRC! %%f
set SRC=!SRC! src\hUGE\hUGEDriver.o
set CF=-Wf--opt-code-size -Wl-m -Wl-j -Wm-yC -Wm-yn"SNEAKYKRYPT" -Wm-yo32 -autobank -Isrc -Isrc\hUGE
if not exist build\card mkdir build\card
rem studio card module: compiled for speed, autobanked
for %%f in (studio_card studio_card_home studio_card_gfx studio_card_scx) do (
  %LCC% -Wm-yC -autobank -Wf--opt-code-speed -c -o build\card\%%f.o src\studio_card\%%f.c
  if errorlevel 1 goto :fail
)
set SRC=%SRC% build\card\studio_card.o build\card\studio_card_home.o build\card\studio_card_gfx.o build\card\studio_card_scx.o
%LCC% %CF% -Wm-ya1 -Wm-yt0x1B -o build\sneaky_kryptid.gbc %SRC%
if errorlevel 1 goto :fail
%LCC% %CF% -Wm-yt0x19 -o build\sneaky_kryptid_nosave.gbc %SRC%
if errorlevel 1 goto :fail
rem Flatwoods Drift standalone gun-cam demo (same sources, -DFLATWOODS_DEMO, cart title FLATWOODS)
set FW=-Wf--opt-code-size -Wl-m -Wl-j -Wm-yC -Wm-yn"FLATWOODS" -Wm-yo32 -autobank -Isrc -Isrc\hUGE -Wf-DFLATWOODS_DEMO
%LCC% %FW% -Wm-ya1 -Wm-yt0x1B -o build\flatwoods_drift.gbc %SRC%
if errorlevel 1 goto :fail
%LCC% %FW% -Wm-yt0x19 -o build\flatwoods_drift_nosave.gbc %SRC%
if errorlevel 1 goto :fail
echo Built build\sneaky_kryptid.gbc, build\sneaky_kryptid_nosave.gbc, build\flatwoods_drift.gbc and build\flatwoods_drift_nosave.gbc
goto :eof
:fail
echo BUILD FAILED
exit /b 1
