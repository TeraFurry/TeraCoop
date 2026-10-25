# Status / Состояние (2026-10-25, program v0.16, module v2.1c)

## English
### Works (tested by two real players)
- Steam P2P connection (uses Spacewar app id 480 for networking; no port forwarding), room codes, host/guest roles, ping display.
- The module loads into the game (proxy `dinput8.dll`) and talks to the program through shared memory.
- Player 2 can be driven from the other PC: movement axes, jump, attack, special. Actions reach the game 100% in both directions; each PC uses **its own** key bindings (parsed from the game's `Mappings.dat`).
- Position of player 2 is made visible by rewriting every copy of the position field (the game keeps several).
- Logs named `host_… / guest_… / solo_…` plus a one-key data-collection tool (F11, markers F3/F4/F5).
- The game's archives (`GAME*.DAT`, PakDat v1.01) can be unpacked with community QuickBMS scripts; text data (`krawlietypes.txt`, level scripts, `game.txt`) is readable.

### Not done
- **Enemies**: not synchronized. Our first attempt mirrored the game's "character list", which turned out to be the player party (two heroes + a story character such as Merry), not enemies. The mirror is now off by default (F7). Real enemies are "Krawlie" objects; how they are created and tracked is still being investigated.
- Enemy death / health, player hearts, coins (studs), breakable objects, level transitions, loading the friend into the host's world and save, per-player cameras, installer.
- Roll animation after a double jump and occasional sliding of player 2.

### Untested / experimental
- F6 "pad injection" (writes input straight into player 2's pad record instead of key presses).
- Heap scan used by the F11 tool on very different PCs.

### Ideas / next steps
1. Record on ordinary levels (not the horse-riding Pelennor level) and track Krawlie objects.
2. Read the decompiled game code around `DeathContext`, `MechCharacterBaseHealthInfo`, `LotrPickup`, `ScrAction_EmitCoins` and hook those events.
3. Synchronize deaths, coins and breakables as events from the host.

## Русский
### Работает (проверено двумя реальными игроками)
- Соединение через Steam (для сети используется app id 480 Spacewar; без проброса портов), коды комнат, роли хост/гость, пинг.
- Модуль загружается в игру (подменная `dinput8.dll`) и общается с программой через общую память.
- Вторым игроком можно управлять с другого ПК: оси движения, прыжок, удар, особое. Действия доходят до игры на 100% в обе стороны; каждый ПК использует **свои** клавиши (читаются из `Mappings.dat` игры).
- Позиция второго игрока видна за счёт перезаписи всех копий поля позиции (игра хранит их несколько).
- Журналы `host_… / guest_… / solo_…` и сбор данных по одной клавише (F11, метки F3/F4/F5).
- Архивы игры (`GAME*.DAT`, PakDat v1.01) распаковываются скриптами сообщества для QuickBMS; текстовые данные (`krawlietypes.txt`, сценарии уровней, `game.txt`) читаются.

### Не сделано
- **Враги**: не синхронизируются. Первая попытка зеркалила «список персонажей» игры, а он оказался отрядом игроков (два героя и сюжетный персонаж, например Мерри), а не врагами. Зеркало теперь выключено по умолчанию (F7). Настоящие враги — объекты «Krawlie»; как они создаются и отслеживаются, пока выясняется.
- Смерть и здоровье врагов, сердца игрока, монетки, ломающиеся предметы, переходы между уровнями, загрузка друга в мир и сохранение хоста, отдельные камеры, установщик.
- Анимация переката после двойного прыжка и иногда скольжение второго игрока.

### Не проверено / эксперимент
- F6 «подача в геймпад» (ввод пишется прямо в запись геймпада второго игрока вместо нажатий клавиш).
- Сканирование кучи в инструменте F11 на сильно разных ПК.

### Планы
1. Записи на обычных уровнях (не на «Пеленноре» верхом) и отслеживание объектов Krawlie.
2. Прочитать декомпилированный код вокруг `DeathContext`, `MechCharacterBaseHealthInfo`, `LotrPickup`, `ScrAction_EmitCoins` и подключиться к этим событиям.
3. Передавать смерти, монетки и ломающееся от хоста как события.
