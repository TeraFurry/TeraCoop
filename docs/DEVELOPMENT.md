# Development / Разработка

## English
### What you need
- A legal copy of LEGO The Lord of the Rings (Steam, app 214510, 2012 build: exe timestamp `0x50d201cc`, size `0x015e6000`). **Work on a copy of the game folder**, never the original.
- Two PCs with Steam and the game (for network tests); one PC is enough for solo data collection.
- LLVM 18: `clang`, `lld-link`, `llvm-dlltool`, `llvm-rc` (cross-build from Linux/WSL, or install LLVM on Windows). Targets are **32-bit Windows** (`i686-pc-windows-msvc`), freestanding (no C runtime), see `build.sh`.
- Valve's 32-bit `steam_api.dll` from the Steamworks SDK, saved as `src/program/steam_api_x86.dll` (embedded in the exe as a resource; **not in this repo**).
- For reading game data: [QuickBMS](https://aluigi.org/quickbms.htm) + `ttgames.bms` from [linterniGamer/Tt-Games-quickbms-scripts](https://github.com/linterniGamer/Tt-Games-quickbms-scripts) (use `quickbms_4gb_files`); `ttmodding.com` and [JaanDev/lego-tt-nxg-formats](https://github.com/JaanDev/lego-tt-nxg-formats) describe the formats.
- For reading the game code: a disassembler/decompiler such as Ghidra (the exe keeps RTTI class names, which helps a lot). Not included.

### Build
`./build.sh` produces `src/module/dinput8.dll` and `src/program/TeraCoop.exe`. The program embeds the module (resource 100) and `steam_api.dll` (resource 101). To install for testing: put `dinput8.dll` next to `LEGOLOTR.exe` in the **copy** of the game and run `TeraCoop.exe`.

### Test routine
- Solo: start the game from the copy with the module, press **F11** to record up to 120 s (markers F3 = health changed, F4 = enemy died, F5 = coin picked up). Files go to `logs\` next to the game: `solo_data_*_recNN.bin`, `solo_classes_*.txt`, `solo_objects_*.bin`, `solo_module_*.log`.
- Two players: both run `TeraCoop.exe`; one creates a room and gives the code, the other joins. Logs are named `host_…` / `guest_…`.
- There is no automated in-game test yet; the long-term plan is a scripted bot run that spawns/hits enemies and checks results (as in coop-anything's QA playbook).

### Architecture
- Module ⇄ program: shared memory `Local\TeraCoopLink` (4096 B, struct in `link.h`, version 5).
- Program ⇄ program: Steam P2P messages (12-byte packets). Host is the authority.
- Code style: single C file per binary, freestanding, no CRT; small helper log functions.

## Русский
### Что нужно
- Легальная копия LEGO The Lord of the Rings (Steam, app 214510, сборка 2012: метка exe `0x50d201cc`, размер `0x015e6000`). **Работайте на копии папки игры**, не на оригинале.
- Два ПК со Steam и игрой для сетевых тестов; для одиночного сбора данных хватит одного.
- LLVM 18: `clang`, `lld-link`, `llvm-dlltool`, `llvm-rc` (кросс-сборка из Linux/WSL или LLVM под Windows). Цель — **32-битный Windows** (`i686-pc-windows-msvc`), без библиотеки C, см. `build.sh`.
- 32-битный `steam_api.dll` от Valve из Steamworks SDK, сохранить как `src/program/steam_api_x86.dll` (вшивается в exe ресурсом; **в репозитории его нет**).
- Для чтения данных игры: [QuickBMS](https://aluigi.org/quickbms.htm) + `ttgames.bms` из [linterniGamer/Tt-Games-quickbms-scripts](https://github.com/linterniGamer/Tt-Games-quickbms-scripts) (`quickbms_4gb_files`); форматы описаны на `ttmodding.com` и в [JaanDev/lego-tt-nxg-formats](https://github.com/JaanDev/lego-tt-nxg-formats).
- Для чтения кода игры: дизассемблер/декомпилятор вроде Ghidra (в exe остались имена классов RTTI, это сильно помогает). В репозитории не лежит.

### Сборка
`./build.sh` собирает `src/module/dinput8.dll` и `src/program/TeraCoop.exe`. Программа вшивает модуль (ресурс 100) и `steam_api.dll` (ресурс 101). Для проверки: положите `dinput8.dll` рядом с `LEGOLOTR.exe` в **копии** игры и запустите `TeraCoop.exe`.

### Как тестируем
- Одному: запустить игру из копии с модулем, нажать **F11** — запись до 120 с (метки F3 = здоровье изменилось, F4 = враг умер, F5 = подобрана монетка). Файлы лежат в `logs\` рядом с игрой: `solo_data_*_recNN.bin`, `solo_classes_*.txt`, `solo_objects_*.bin`, `solo_module_*.log`.
- Вдвоём: оба запускают `TeraCoop.exe`; один создаёт комнату и передаёт код, другой входит. Журналы называются `host_…` / `guest_…`.
- Автоматического теста в игре пока нет; цель — скриптовый бот, который создаёт/бьёт врагов и проверяет результат (как в QA-руководстве coop-anything).

### Устройство
- Модуль ⇄ программа: общая память `Local\TeraCoopLink` (4096 Б, структура в `link.h`, версия 5).
- Программа ⇄ программа: сообщения Steam P2P (пакеты по 12 байт). Главный — хост.
- Стиль кода: один C-файл на бинарник, без CRT, маленькие функции журнала.
