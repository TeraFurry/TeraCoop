# Game data analysis / Разбор данных игры

Findings from unpacking the Steam build with QuickBMS (text and `.giz` only). **No game files are included**; only file names (`extracted-file-list.txt`) and our observations.
Результаты распаковки сборки Steam (только текст и `.giz`). **Файлов игры здесь нет**: только имена файлов (`extracted-file-list.txt`) и наши наблюдения.

## What was unpacked / Что распаковано
| Archive | Files | Content / Содержимое |
|---|---|---|
| `GAME.DAT` | 1913 | main data: `AUDIO` (11), `CHARS` (1688), `CUT` (35), `LEVELS` (54), `STUFF` (114), `SCRIPTS` (1), `FRAMEWORK-DATA` (1), `FILETABLE.TXT` |
| `GAME1.DAT` … `GAME3.DAT` | 281 / 226 / 439 | per-area data (text + `.giz`) |
| `GAME0.DAT`, `GAME4.DAT` | 7 / 89 | small config / misc |

About 2955 files, ~28 MB of text. `FILETABLE.TXT` inside `GAME.DAT` lists every file of the game.

## Useful files / Полезные файлы
- `STUFF/KRAWLIETYPES.TXT`: enemy ("Krawlie") types: `KrawlieType … CharDef … Baddie/Ranged`, add-ons.
- `STUFF/GAME.TXT`: global rules, e.g. the hearts line (`hearts display on max 4 extramax 6 2in1 off`).
- `STUFF/AI/AITYPES.TXT`, `SCRIPTS/SCRIPT.TXT`: AI types and scripted-action names (lists of names, not logic).
- `LEVELS/<area>/<level>.TXT`: level description; entries `character "X" player|resident` tell who is a player character.
- `LEVELS/<area>/*.GIZ`: binary level objects ("gizmos"). Enemy spawners `MechKrawlieSpawner` / `MechKrawlieSpawnPoint` live here. Formats: see <https://github.com/JaanDev/lego-tt-nxg-formats>.
- `LEVELS/MIDDLEEARTH/…`: the open world hub (`MIDDLEEARTH_A…E`), a candidate free-roam test area (untested).
- `STUFF/TEXT`, `CHARS`: strings and character definitions.

## What we learned / Что выяснили
- There is no developer test level in the data (`msg_TestAreaMsg` is a string in 69 files, not a map).
- Enemies are "Krawlies" (crowd units). Coins are pickups (`LotrPickup`, `COIN_BLUE`); breakables are gizmos.
- The exe carries MSVC RTTI, so class names can be found in memory (see `GAME-NOTES.md`); combined with the data files this replaces blind memory poking.

## Next steps / Дальше
1. Read the Ghidra decompilation around `DeathContext`, `MechCharacterBaseHealthInfo`, `LotrPickup`, `Krawlie*`.
2. Parse `.giz` with the community format notes to list spawners/pickups per level.
3. Use the data to build an automatic test (spawn, hit, check HP) in a free-roam area.
