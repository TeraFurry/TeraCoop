#!/bin/sh
# Cross-build on Linux/macOS/WSL with LLVM 18 (clang, lld-link, llvm-dlltool, llvm-rc). Produces dinput8.dll and TeraCoop.exe (32-bit Windows).
# Кросс-сборка из Linux/WSL инструментами LLVM 18. Получаются dinput8.dll и TeraCoop.exe (32-битный Windows).
set -e
CF="--target=i686-pc-windows-msvc -msse2 -O1 -fno-math-errno -ffreestanding -fno-builtin -fno-stack-protector -mno-stack-arg-probe -Wall -c"
# 1) module / модуль
cd src/module
llvm-dlltool -m i386 -k -d k32.def -l kernel32.lib
llvm-dlltool -m i386 -k -d u32.def -l user32.lib
llvm-rc /C 65001 /FO mod.res mod.rc
clang $CF coop_mod.c -o coop_mod.obj
lld-link /dll /nodefaultlib /machine:x86 /entry:DllMain /safeseh:no /dynamicbase /nxcompat /def:dinput8.def /out:dinput8.dll coop_mod.obj mod.res kernel32.lib user32.lib
cd ../program
# the program embeds the module (resource 100) and Valve's steam_api.dll (resource 101, from the Steamworks SDK, NOT in this repo)
# программа вшивает модуль (ресурс 100) и steam_api.dll от Valve (ресурс 101, из Steamworks SDK, в репозитории его НЕТ)
cp ../module/dinput8.dll mod_dinput8.dll
[ -f steam_api_x86.dll ] || { echo "Put 32-bit steam_api.dll as src/program/steam_api_x86.dll (see docs/DEVELOPMENT.md)"; exit 1; }
llvm-dlltool -m i386 -k -d k32.def -l kernel32.lib
llvm-dlltool -m i386 -k -d u32.def -l user32.lib
llvm-dlltool -m i386 -k -d g32.def -l gdi32.lib
llvm-dlltool -m i386 -k -d w32.def -l ws2_32.lib
llvm-rc /C 65001 /FO app.res app.rc
clang $CF teracoop.c -o teracoop.obj
lld-link /subsystem:windows /nodefaultlib /machine:x86 /entry:WinMainCRTStartup /safeseh:no /dynamicbase /nxcompat /out:TeraCoop.exe teracoop.obj app.res kernel32.lib user32.lib gdi32.lib ws2_32.lib
echo OK
