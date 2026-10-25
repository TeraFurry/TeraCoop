@echo off
chcp 65001 >nul
rem Usage: extract_scripts.bat "<game folder>" "<output folder>"
rem Применение: extract_scripts.bat "<папка игры>" "<папка вывода>"
cd /d "%~dp0"
set QB=quickbms_4gb_files.exe
if not exist "%QB%" (
  echo %QB% not found. Download QuickBMS from https://aluigi.org/quickbms.htm and put it here.
  echo Не найден %QB%. Скачай QuickBMS с https://aluigi.org/quickbms.htm и положи сюда.
  pause & exit /b 1
)
if not exist ttgames.bms (
  echo ttgames.bms not found. Get it from https://github.com/linterniGamer/Tt-Games-quickbms-scripts
  echo Не найден ttgames.bms. Возьми его тут: https://github.com/linterniGamer/Tt-Games-quickbms-scripts
  pause & exit /b 1
)
set GAME=%~1
set OUT=%~2
if "%GAME%"=="" set GAME=C:\Program Files (x86)\Steam\steamapps\common\LEGO Lord of the Rings
if "%OUT%"=="" set OUT=%~dp0extracted
set FILTERS={}.txt;{}.giz;{}.as;{}.ats;{}.scp;{}.gin;{}.cfg;{}.cd;{}.ai
mkdir "%OUT%" 2>nul
for %%F in (GAME0.DAT GAME.DAT GAME1.DAT GAME2.DAT GAME3.DAT GAME4.DAT) do (
  if exist "%GAME%\%%F" (
    echo ---- %%F ----
    mkdir "%OUT%\%%F" 2>nul
    "%QB%" -o -f "%FILTERS%" ttgames.bms "%GAME%\%%F" "%OUT%\%%F" >> "%OUT%\log_extract.txt" 2>&1
  )
)
echo Done / Готово: %OUT%
pause
