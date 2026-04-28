# CardputerMusicDownloader
# Music Downloader for M5Cardputer

## 🇷🇺 Русский

### Описание
Прошивка для устройства **M5Cardputer**, позволяющая:
- Искать и скачивать музыку с сайта `ru.drivemusic.me`.
- Вводить названия треков на русском языке с помощью **цифр 2–9 + стрелки** (раскладка как на кнопочных телефонах).
- Переключать раскладку **Ctrl+Пробел** (RU / EN).
- Автоматически подключаться к Wi-Fi: выбор сети, ввод пароля, сохранение на SD-карту (`/wifi.txt`).
- Сохранять скачанные MP3-файлы в папку **/msc** на SD-карте.

> **Примечание:** Проект распространяется по лицензии MIT. Вы можете свободно использовать, модифицировать и распространять код. Однако помните об авторских правах на музыкальные произведения – скачивайте только легально доступные треки.

### Управление

#### Ввод строки поиска
- **Ctrl+Пробел** – переключение раскладки (RU / EN).
- **Цифры 2–9** (в русской раскладке) – выбор группы букв, затем стрелка:
  - `,` (влево) – первая буква
  - `/` (вверх) – вторая буква
  - `;` (вправо) – третья буква
  - `.` (вниз) – четвёртая буква
- **Enter** – завершить ввод и начать поиск.
- **Del** – удалить последний символ.

#### Навигация по результатам
- `,` / `/` – предыдущая / следующая страница.
- `;` / `.` – перемещение курсора вверх/вниз по трекам.
- **Enter** – скачать выбранный трек.
- **0** – вернуться к новому поиску.

### Первый запуск
1. Вставьте SD-карту (формат FAT32).
2. При отсутствии `/wifi.txt` устройство просканирует доступные сети Wi-Fi.
3. Выберите номер сети (1–9), при необходимости введите пароль.
4. После успешного подключения данные сохранятся на SD.
5. Вводите название трека и скачивайте.

### Структура SD-карты
```
/wifi.txt          – SSID (строка 1) и пароль (строка 2)
/msc/              – папка со скачанными MP3-файлами
```

### Сборка прошивки (Arduino IDE)
1. Установите библиотеки:
   - `M5Cardputer` (через менеджер библиотек)
   - `SD` (встроенная)
   - `WiFi`, `HTTPClient` (стандартные)
2. Дополнительно для кириллического шрифта (опционально):
   - `U8g2` и подключите файл шрифта `u8g2_font_6x12_t_cyrillic`.
3. Выберите плату **M5Stack-Cardputer**.
4. Загрузите скетч.

### Примечания
- Отображение русских букв на дисплее требует шрифта с кириллицей. Без него буквы будут квадратиками, но поиск работает корректно (запрос кодируется в URL).
- Сайт `ru.drivemusic.me` может менять структуру HTML – парсинг результатов может перестать работать. В таком случае обновите селекторы в функции `parseSearchResults`.

---

## 🇬🇧 English

### Description
Firmware for **M5Cardputer** that allows:
- Search and download music from `ru.drivemusic.me`.
- Enter Russian song names using **digits 2–9 + arrow keys** (like old mobile phone keypad).
- Switch layout with **Ctrl+Space** (RU / EN).
- Auto Wi-Fi setup: select network, enter password, save to SD card (`/wifi.txt`).
- Save downloaded MP3 files to **/msc** folder on SD card.

> **Note:** This project is released under the MIT license. You are free to use, modify, and distribute the code. However, respect music copyrights – download only legally available tracks.

### Controls

#### Search input
- **Ctrl+Space** – toggle layout (RU / EN).
- **Digits 2–9** (in RU layout) – choose a letter group, then press arrow:
  - `,` (left) – first letter
  - `/` (up) – second letter
  - `;` (right) – third letter
  - `.` (down) – fourth letter
- **Enter** – finish input and start search.
- **Del** – delete last character.

#### Results navigation
- `,` / `/` – previous / next page.
- `;` / `.` – move cursor up/down.
- **Enter** – download selected track.
- **0** – new search.

### First run
1. Insert SD card (FAT32 formatted).
2. If `/wifi.txt` is missing, device scans available Wi-Fi networks.
3. Select network number (1–9), enter password if required.
4. Credentials are saved to SD upon successful connection.
5. Enter song name and download.

### SD card structure
```
/wifi.txt          – SSID (line 1) and password (line 2)
/msc/              – downloaded MP3 files
```

### Compilation (Arduino IDE)
1. Install libraries:
   - `M5Cardputer` (via Library Manager)
   - `SD` (built-in)
   - `WiFi`, `HTTPClient` (built-in)
2. Optional for Cyrillic font:
   - `U8g2` and include `u8g2_font_6x12_t_cyrillic`.
3. Select board **M5Stack-Cardputer**.
4. Upload sketch.

### Notes
- Cyrillic display requires a font with Cyrillic glyphs. Without it, Russian letters appear as boxes, but search still works (query is URL-encoded).
- The website `ru.drivemusic.me` may change HTML structure – parsing might break. Update selectors in `parseSearchResults` if needed.

---

## Лицензия / License

## Лицензия / License

Этот проект лицензирован под **MIT License** – подробности в файле [LICENSE](LICENSE).  
This project is licensed under the **MIT License** – see the [LICENSE](LICENSE) file for details.

---
