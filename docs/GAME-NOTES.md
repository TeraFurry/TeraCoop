# Game notes / Заметки об игре

Valid for the Steam build with exe timestamp `0x50d201cc`, size `0x015e6000`, link base `0x400000` (the exe is relocated at run time, e.g. `0x00f10000`; add `runtime_base - 0x400000` to every VA below).
Верно для сборки Steam с меткой exe `0x50d201cc`, размером `0x015e6000`, базой `0x400000` (при запуске exe перемещается, например на `0x00f10000`; ко всем адресам VA ниже добавляйте `рабочая_база - 0x400000`).

These are interoperability notes made by observing a running copy of the game; no game data is included. / Это заметки о совместимости, полученные наблюдением за работающей копией игры; данных игры здесь нет.

## Memory layout / Память
| What | Where |
|---|---|
| Player table (two entries) | VA `0x015b30e0` |
| "Character list" head | VA `0x015b31e8`, next pointer at `+0x244`; bounded by two static marker nodes (vtable 0). **It is the player party** (both heroes + extra story characters, e.g. Merry on Pelennor), **not enemies** |
| Character position (several copies, all must be written) | `+0x190` (also `+0x64/+0x70/+0x7c`) as 3 floats; yaw `+0x1b4`; matrix `+0x160` |
| Character → body | `ch+0x1b0` |
| Pad object of a player | `ch+0x4C0`, vtable VA `0x010df7d8`; `+4` owner, `+0x18/+0x1c` axis floats, `+0xC` → record, held-button bits at `record+4`: `0x4000` jump, `0x8000` action, `0x2000` special |
| Health info | class `MechCharacterBaseHealthInfo` (vtable exe+`0xcdfabc`), `+0xC` → owner. Freed (zeroed) when an enemy dies. Player hearts are **not** here |
| Death of a player | an object of class `DeathContext` appears under the character (~`+0x4d0`); afterwards `InvulnerableAddon` (~`+0x684`) |
| Class names | the exe contains MSVC RTTI (`.?AV…`): find a vtable by following `vtable-4 → COL → TypeDescriptor` |
| Target of an AI character | fields `+0x230/+0x238/+0x270/+0x144` (and `+0x5ac` on player 0) point to the character currently targeted; they change when an enemy dies |

## Game data / Данные игры
- Archives `GAME.DAT … GAME4.DAT`, header `PakDat v1.01`. Unpack with QuickBMS + `ttgames.bms`. File list: `filetable.txt` in the game folder.
- Text data worth reading: `stuff\krawlietypes.txt` (enemy types: `KrawlieType … CharDef … Baddie/Ranged`, add-ons), `stuff\game.txt` (rules, e.g. `hearts display on max 4 extramax 6`), `levels\<chapter>\<level>\<level>.txt` (which characters a level loads: `character "Name" player|resident`), `chars\chardata.txt`.
- Level objects (`*.giz`) are binary; enemy spawners (`MechKrawlieSpawner`, `MechKrawlieSpawnPoint`) live there.
- Enemies are "Krawlies" (crowd units); coins are pickups (`LotrPickup`, `GizmoPickup`, type names like `COIN_BLUE`).
- The 2012 PC build has no online mode.

## Controls / Управление
`%APPDATA%\Warner Bros. Interactive Entertainment\LEGO The Lord of the Rings\Mappings.dat`: blocks `Keyboard1` / `Keyboard2` (0x9C bytes each, starting with the name); four axes at `+0x64` (3 bytes each: `0A minus plus`), button slots at `+0x70` (2 bytes each: `07 <DirectInput code>`; slot 11 special, 12 jump, 13 action). Defaults: P1 = WASD + H/J/Space, P2 = arrows + `.` `'` `/`.

## Network packets / Пакеты (12 bytes: tag, w1, w2)
`PING PONG HELO PXZ PYA BTN` (position x/z, y/yaw, buttons). Enemy-mirror packets (`ENA ENB ENI ENS`, tag = kind<<16 | id) exist in the source but are obsolete: they mirrored the party list, see STATUS.

## Link struct (shared memory) / Общая память
`link.h` (`LINK_VERSION 5`): program tick, wanted player 2, injected axes/buttons, role (1 host / 2 guest), room role, test/capture counters.
