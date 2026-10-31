# Status / Состояние (2026-11-01, program v0.16, module v2.14)

## English
### Works
Tested by two real players over the network (program v0.16, module v2.1c, October 2026):
- Steam P2P connection (Spacewar app id 480 for networking; no port forwarding) and IP mode (e.g. Radmin VPN), room codes, host/guest roles, ping display.
- Player 2 can be driven from the other PC: movement, jump, attack, special. Each PC uses **its own** key bindings (parsed from the game's `Mappings.dat`).

Tested on two copies of the game on one PC (module v2.x, not yet with a friend):
- Direct link between two copies (settings file `teracoop_local.txt` = `host` / `guest`): the other copy's player 1 is shown as my player 2, at the exact position and heading.
- Hero sync both ways by hero name, only when the other player **changes** hero.
- F8: change player 1 into the hero named in `teracoop_hero.txt` through the game's own loader (`CharLoaderUtilSwap`: load the model, then swap).
- **Own screen** (`teracoop_view.txt` = `own`): one screen in each copy, the camera follows only player 1, and zone transitions still work (they no longer wait for the camera of player 2). Verified in the hub on both copies.
- Level changes, added/killed characters and the game's current level are written to the log; window mode (`teracoop_windowed.txt`); running two copies at once (`teracoop_multi.txt`).

### Broken / not done
- **v2.14 smoothing** (speed prediction for the other hero): after this version the owner saw no sync at all. Not investigated yet; v2.13 is the last version where sync was seen working.
- Riding (horses and other mounts) breaks the sync.
- Transitions into buildings and inside story levels have problems (the own-screen patch was verified in the hub only).
- The transition fade (darkening) is missing in own-screen mode.
- Both copies can end up with the same hero for player 1, so a level shows duplicate heroes.
- Not synchronized: enemies, health/hearts, coins, breakables, loading the guest into the host's world without his own save.

### Untested / experimental
- F6 pad injection; the F11 heap scan on very different PCs.

### Next steps
1. Find out why v2.14 loses the sync (logs of both copies).
2. Map the systems to real function names using the public symbol list of LEGO City Undercover (Switch), which runs on the same engine branch ("Gotham", like LEGO Batman 2 and this game), see docs/DEVELOPMENT.md.
3. Then, one system at a time: guest joins the host's world, then health, coins, enemies, objects.

## Русский
### Работает
Проверено двумя реальными игроками по сети (программа v0.16, модуль v2.1c, октябрь 2026):
- Соединение через Steam (для сети app id 480 Spacewar, без проброса портов) и по IP (например, Radmin VPN), коды комнат, роли хост/гость, пинг.
- Вторым игроком можно управлять с другого ПК: движение, прыжок, удар, особое. Каждый ПК использует **свои** клавиши (читаются из `Mappings.dat` игры).

Проверено на двух копиях игры на одном ПК (модуль v2.x, с другом ещё нет):
- Прямая связь двух копий (файл `teracoop_local.txt` = `host` / `guest`): игрок 1 другой копии показывается моим игроком 2, точно по позиции и направлению.
- Синхронизация героя в обе стороны по имени, только когда другой игрок **сам меняет** героя.
- F8: превратить игрока 1 в героя из `teracoop_hero.txt` через загрузчик самой игры (`CharLoaderUtilSwap`: сначала загрузка модели, потом замена).
- **Свой экран** (`teracoop_view.txt` = `own`): в каждой копии один экран, камера следит только за игроком 1, а переходы между зонами работают (они больше не ждут камеру игрока 2). Проверено в хабе на обеих копиях.
- В журнал пишутся смены уровня, появление и гибель персонажей, текущий уровень игры; оконный режим (`teracoop_windowed.txt`); две копии одновременно (`teracoop_multi.txt`).

### Сломано / не сделано
- **Сглаживание v2.14** (предсказание по скорости для другого героя): после этой версии владелица не увидела синхронизации совсем. Ещё не разобрано; v2.13 — последняя версия, где синхронизация работала.
- Езда верхом (лошади и другие животные) ломает синхронизацию.
- Переходы в здания и внутри сюжетных уровней работают с ошибками (правка «свой экран» проверена только в хабе).
- В режиме «свой экран» нет затемнения при переходе.
- В обеих копиях у игрока 1 может оказаться один и тот же герой, и на уровне появляются двойники.
- Не синхронизируется: враги, здоровье/сердечки, монетки, ломающиеся предметы, вход гостя в мир хоста без своего сохранения.

### Не проверено / эксперимент
- F6 «подача в геймпад»; сканирование кучи в F11 на сильно разных ПК.

### Дальше
1. Выяснить, почему в v2.14 пропадает синхронизация (журналы обеих копий).
2. Сопоставить системы игры с настоящими именами функций по открытому списку символов LEGO City Undercover (Switch). Это та же ветка движка («Gotham», как LEGO Batman 2 и наша игра), см. docs/DEVELOPMENT.md.
3. Потом по одной системе: вход гостя в мир хоста, затем здоровье, монетки, враги, объекты.
