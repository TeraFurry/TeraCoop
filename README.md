# LEGO The Lord of the Rings — online co-op mod (work in progress)

**English** · [Русский](#русский)

An experimental mod that tries to let two people play **LEGO® The Lord of the Rings™** (PC, Steam app 214510) together **online** over Steam, without port forwarding. The game itself only has local (same-screen) co-op. Nobody has published an online co-op for it, so this is built from scratch by reverse engineering. It follows the approach of [cranebeaker/coop-anything](https://github.com/cranebeaker/coop-anything).

> **Status: early prototype.** Two players can connect and the second player can be controlled over the network (movement, jump, attack, special). The world (enemies, coins, deaths, breakables) is **not** synchronized yet. See [docs/STATUS.md](docs/STATUS.md).

This repository contains **only our own source code and notes**. It does **not** contain any game files, game code, decompiled code, the game's data, Valve's `steam_api.dll`, or any third-party binaries. You need your own legal copy of the game.

## What is here
| Path | What |
|---|---|
| `src/module/` | The in-game module (`dinput8.dll`, a proxy DLL loaded by the game): reads/writes game memory, injects input for player 2, data-collection tool (F11), logging |
| `src/program/` | `TeraCoop.exe`: a small window program: Steam P2P connection, room codes, relays actions between the PCs, talks to the module through shared memory |
| `docs/STATUS.md` | What works / what does not / what is untested |
| `docs/DEVELOPMENT.md` | What you need to develop, build and test; how the pieces fit |
| `docs/GAME-NOTES.md` | Technical findings about the game (structures, offsets, file formats, packets) |
| `build.sh` | Cross-build with LLVM 18 (32-bit Windows) |

## How it works (short)
Host-authoritative design. The host plays normally. The guest's actions (axes, jump, attack, special) are sent to the host over Steam networking and applied to player 2 in the host's game; the host's state is sent back. Everything is installed on both PCs: the module goes next to `LEGOLOTR.exe`, the program runs in a window. **Only experiment on a copy of the game folder.**

## Build
See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md). Needs LLVM 18 and Valve's 32-bit `steam_api.dll` (not included).

## License
MIT (see `LICENSE`): anyone may use, copy, modify and distribute the code. Game, names and trademarks belong to their owners; this project is unofficial and not affiliated with TT Games, Warner Bros., LEGO or Valve.

---

<a name="русский"></a>
# LEGO Властелин Колец — мод онлайн-кооператива (в разработке)

Экспериментальный мод, который пытается дать двум игрокам играть в **LEGO® The Lord of the Rings™** (PC, Steam, app 214510) вместе **по сети** через Steam, без проброса портов. В самой игре есть только кооператив на одном экране. Онлайн-кооператив для неё никто не публиковал, поэтому мы делаем его с нуля, разбирая игру. Подход взят из [cranebeaker/coop-anything](https://github.com/cranebeaker/coop-anything).

> **Состояние: ранний прототип.** Два игрока могут подключиться, второй игрок управляется по сети (движение, прыжок, удар, особое действие). Мир (враги, монетки, смерти, ломающиеся предметы) **пока не синхронизируется**. Подробнее в [docs/STATUS.md](docs/STATUS.md).

В репозитории **только наш исходный код и заметки**. Здесь **нет** файлов игры, кода игры, декомпилированного кода, данных игры, `steam_api.dll` от Valve и чужих программ. Нужна своя легальная копия игры.

## Что здесь
| Путь | Что |
|---|---|
| `src/module/` | Модуль внутри игры (`dinput8.dll`, подменяющая DLL): читает и пишет память игры, подаёт ввод второму игроку, сбор данных (F11), журналы |
| `src/program/` | `TeraCoop.exe`: небольшая программа с окном: Steam-соединение, коды комнат, пересылка действий между ПК, связь с модулем через общую память |
| `docs/STATUS.md` | Что работает, что нет, что не проверено |
| `docs/DEVELOPMENT.md` | Что нужно для разработки, сборки и тестов, как всё устроено |
| `docs/GAME-NOTES.md` | Технические находки об игре (структуры, смещения, форматы, пакеты) |
| `build.sh` | Кросс-сборка LLVM 18 (32-битный Windows) |

## Как это работает (кратко)
Главный — хост. Хост играет как обычно. Действия гостя (оси, прыжок, удар, особое) по сети Steam идут к хосту и применяются ко второму игроку в игре хоста; состояние хоста отправляется обратно. Ставится на оба ПК: модуль рядом с `LEGOLOTR.exe`, программа запускается отдельным окном. **Экспериментируйте только на копии папки игры.**

## Сборка
См. [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md). Нужны LLVM 18 и 32-битный `steam_api.dll` от Valve (в репозитории его нет).

## Лицензия
MIT (см. `LICENSE`): код может свободно использовать, копировать, изменять и распространять любой желающий. Игра, названия и торговые марки принадлежат владельцам; проект неофициальный и не связан с TT Games, Warner Bros., LEGO и Valve.
