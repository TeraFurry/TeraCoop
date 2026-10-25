# Unpacking the game's text data / Распаковка текстовых данных игры

**EN.** The game keeps level scripts, enemy types and rules as plain text inside `GAME*.DAT` ("PakDat v1.01"). The repository does **not** contain this data (it belongs to TT Games / Warner Bros.). To read it yourself:
1. Get QuickBMS (`quickbms_4gb_files.exe`) from <https://aluigi.org/quickbms.htm> and `ttgames.bms` from <https://github.com/linterniGamer/Tt-Games-quickbms-scripts>; put both next to `extract_scripts.bat`.
2. Run `extract_scripts.bat "<game folder>" "<output folder>"` (about 10–20 minutes). Always unpack from a **copy** of the game.
3. Read `docs/GAME-DATA.md` and `docs/extracted-file-list.txt` to see what is where.

**RU.** Сценарии уровней, типы врагов и правила хранятся в `GAME*.DAT` обычным текстом. Самих данных в репозитории **нет** (они принадлежат TT Games / Warner Bros.). Как прочитать самому:
1. Скачай QuickBMS (`quickbms_4gb_files.exe`) с <https://aluigi.org/quickbms.htm> и `ttgames.bms` с <https://github.com/linterniGamer/Tt-Games-quickbms-scripts>; положи оба рядом с `extract_scripts.bat`.
2. Запусти `extract_scripts.bat "<папка игры>" "<папка вывода>"` (около 10–20 минут). Распаковывай только из **копии** игры.
3. Смотри `docs/GAME-DATA.md` и `docs/extracted-file-list.txt`: что где лежит.
