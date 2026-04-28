/*
 * Music Downloader for M5Cardputer – with Russian layout (Ctrl+Space)
 * Auto Wi-Fi selection with saving to SD card
 */

#include <M5Cardputer.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <vector>

// ========== НАСТРОЙКИ ==========
const char* wifiConfigFile = "/wifi.txt";
const char* baseUrl = "https://ru.drivemusic.me";
const String searchPath = "/?do=search&subaction=search&story=";
const char* musicFolder = "/msc";

// Шрифт с кириллицей (подключаем внешний, если нужен)
// Если шрифт не используется, закомментируйте строки с setFont
extern const uint8_t u8g2_font_6x12_t_cyrillic[];
const lgfx::U8g2font myFont = { u8g2_font_6x12_t_cyrillic };

// ========== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ==========
struct TrackInfo {
    String name;
    String fileName;
    String mp3Url;
};
std::vector<TrackInfo> searchResults;
enum AppMode { MODE_INPUT_SEARCH, MODE_SHOW_RESULTS, MODE_DOWNLOADING };
AppMode currentMode = MODE_INPUT_SEARCH;
String searchQuery = "";

// ---------- Русская раскладка ----------
bool isRussianLayout = false;
bool waitingForArrow = false;
int pendingDigit = 0;
int hintStartX = 0, hintStartY = 0;
const char* russianUTF8[10][4] = {
    {}, {},
    {"а","б","в","г"},
    {"д","е","ж","з"},
    {"и","й","к","л"},
    {"м","н","о","п"},
    {"р","с","т","у"},
    {"ф","х","ц","ч"},
    {"ш","щ","ъ","ы"},
    {"ь","э","ю","я"}
};
bool tableVisible = false;
int tableStartY = 0;

// ========== UTF-8 УДАЛЕНИЕ ПОСЛЕДНЕГО СИМВОЛА ==========
void removeLastUTF8Char(String &str) {
    if (str.length() == 0) return;
    int len = str.length();
    char last = str[len - 1];
    if ((last & 0x80) && (len > 1)) {
        str.remove(len - 2);
    } else {
        str.remove(len - 1);
    }
}

// ========== ПРОГРЕСС-БАР ==========
void showProgressBar(int percent, int width = 20) {
    int filled = (percent * width) / 100;
    M5Cardputer.Display.print("[");
    for (int i = 0; i < width; i++) {
        M5Cardputer.Display.print(i < filled ? "#" : " ");
    }
    M5Cardputer.Display.print("] ");
    M5Cardputer.Display.print(percent);
    M5Cardputer.Display.print("%");
}

// ========== СОХРАНЕНИЕ HTML (ОТЛАДКА) ==========
void saveHtmlToSD(const String& filename, const String& content) {
    String fullPath = String(musicFolder) + "/" + filename;
    File file = SD.open(fullPath, FILE_WRITE);
    if (file) {
        file.print(content);
        file.close();
        M5Cardputer.Display.println("HTML saved: " + filename);
    } else {
        M5Cardputer.Display.println("Failed to save HTML");
    }
}

// ========== URL-ENCODE ==========
String urlEncode(const String& str) {
    String encoded = "";
    for (size_t i = 0; i < str.length(); i++) {
        char c = str[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') encoded += c;
        else if (c == ' ') encoded += '+';
        else {
            encoded += '%';
            char hex[3];
            sprintf(hex, "%02X", (unsigned char)c);
            encoded += hex;
        }
    }
    return encoded;
}

// ========== ФУНКЦИИ РАБОТЫ С Wi-Fi ==========
bool loadWiFiCredentials(String &ssid, String &password) {
    File file = SD.open(wifiConfigFile, FILE_READ);
    if (!file) return false;
    ssid = file.readStringUntil('\n');
    ssid.trim();
    password = file.readStringUntil('\n');
    password.trim();
    file.close();
    return (ssid.length() > 0);
}

void saveWiFiCredentials(const String &ssid, const String &password) {
    File file = SD.open(wifiConfigFile, FILE_WRITE);
    if (file) {
        file.println(ssid);
        file.println(password);
        file.close();
        M5Cardputer.Display.println("WiFi credentials saved.");
    } else {
        M5Cardputer.Display.println("Failed to save WiFi credentials!");
    }
}

String readPasswordInput() {
    String pwd = "";
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.println("Enter Wi-Fi password:");
    M5Cardputer.Display.println("Press Enter when done");
    M5Cardputer.Display.println();
    M5Cardputer.Display.print("> ");
    uint16_t startX = M5Cardputer.Display.getCursorX();
    uint16_t startY = M5Cardputer.Display.getCursorY();

    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
            for (auto c : status.word) {
                if (c != 0 && c != '\r' && c != '\n') {
                    pwd += c;
                    M5Cardputer.Display.print('*');
                }
            }
            if (status.del && pwd.length() > 0) {
                pwd.remove(pwd.length() - 1);
                M5Cardputer.Display.fillRect(startX, startY, M5Cardputer.Display.width() - startX,
                                             M5Cardputer.Display.fontHeight(), BLACK);
                M5Cardputer.Display.setCursor(startX, startY);
                for (int i = 0; i < pwd.length(); i++) M5Cardputer.Display.print('*');
            }
            if (status.enter && pwd.length() > 0) {
                return pwd;
            }
        }
        delay(20);
    }
}

bool connectToWiFi(const String &ssid, const String &password) {
    WiFi.begin(ssid.c_str(), password.c_str());
    int attempts = 0;
    M5Cardputer.Display.print("Connecting");
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        M5Cardputer.Display.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        M5Cardputer.Display.println("\nConnected!");
        M5Cardputer.Display.print("IP: ");
        M5Cardputer.Display.println(WiFi.localIP());
        return true;
    } else {
        M5Cardputer.Display.println("\nConnection failed.");
        return false;
    }
}

void scanAndSelectWiFi() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.println("Scanning Wi-Fi...");
    int n = WiFi.scanNetworks();
    if (n == 0) {
        M5Cardputer.Display.println("No networks found.");
        delay(2000);
        return;
    }
    M5Cardputer.Display.printf("Found %d networks:\n", n);
    for (int i = 0; i < n && i < 9; i++) {
        M5Cardputer.Display.printf("%d: %s (%d dBm)\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    }
    M5Cardputer.Display.println("\nSelect network (1-9):");
    int selected = 0;
    while (selected == 0) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
            for (auto c : status.word) {
                if (c >= '1' && c <= '9') {
                    selected = c - '0';
                    break;
                }
            }
            if (status.enter && selected > 0) break;
        }
        delay(20);
    }
    String chosenSSID = WiFi.SSID(selected - 1);
    String password = "";
    if (WiFi.encryptionType(selected - 1) != WIFI_AUTH_OPEN) {
        password = readPasswordInput();
    }
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.printf("Connecting to %s...\n", chosenSSID.c_str());
    if (connectToWiFi(chosenSSID, password)) {
        saveWiFiCredentials(chosenSSID, password);
    } else {
        M5Cardputer.Display.println("Failed to connect. Check password.");
        delay(2000);
    }
}

void initWiFi() {
    String savedSSID, savedPassword;
    if (loadWiFiCredentials(savedSSID, savedPassword)) {
        M5Cardputer.Display.println("Loaded saved Wi-Fi credentials.");
        M5Cardputer.Display.printf("Connecting to %s...\n", savedSSID.c_str());
        if (connectToWiFi(savedSSID, savedPassword)) {
            return;
        } else {
            M5Cardputer.Display.println("Saved credentials invalid. Starting scan...");
            delay(1500);
        }
    } else {
        M5Cardputer.Display.println("No saved Wi-Fi. Starting scan...");
        delay(1500);
    }
    scanAndSelectWiFi();
    if (WiFi.status() != WL_CONNECTED) {
        M5Cardputer.Display.println("WiFi not connected. Reboot?");
        while (true) delay(1000);
    }
}

// ========== SD КАРТА ==========
bool initSD() {
    if (!SD.begin()) {
        M5Cardputer.Display.println("SD Card mount failed!");
        return false;
    }
    M5Cardputer.Display.println("SD Card OK");
    if (!SD.exists(musicFolder)) {
        if (SD.mkdir(musicFolder)) {
            M5Cardputer.Display.print("Folder created: ");
            M5Cardputer.Display.println(musicFolder);
        } else {
            M5Cardputer.Display.println("Cannot create folder!");
            return false;
        }
    } else {
        M5Cardputer.Display.print("Folder exists: ");
        M5Cardputer.Display.println(musicFolder);
    }
    delay(1000);
    return true;
}

// ========== ПАРСИНГ РЕЗУЛЬТАТОВ ==========
int parseSearchResults(const String& html, std::vector<TrackInfo>& tracks) {
    tracks.clear();
    const String wrapperOpen = "<div class=\"music-popular-wrapper\">";
    const String dataUrlAttr = "data-url=\"";

    int pos = 0;
    int count = 0;
    while (true) {
        int start = html.indexOf(wrapperOpen, pos);
        if (start == -1) break;

        int end = html.indexOf(wrapperOpen, start + wrapperOpen.length());
        if (end == -1) end = html.length();
        String block = html.substring(start, end);

        String mp3Url = "";
        int dataPos = block.indexOf(dataUrlAttr);
        if (dataPos != -1) {
            dataPos += dataUrlAttr.length();
            int dataEnd = block.indexOf('"', dataPos);
            if (dataEnd != -1) {
                mp3Url = block.substring(dataPos, dataEnd);
            }
        }
        String title = mp3Url.substring(mp3Url.lastIndexOf('/') + 1);
        String displayName = title.substring(0, title.length() - 4);
        displayName.replace("-", " ");

        if (title.length() > 0 && mp3Url.length() > 0) {
            TrackInfo t;
            t.name = displayName;
            t.fileName = title;
            t.mp3Url = mp3Url;
            tracks.push_back(t);
            count++;
        }
        pos = end;
    }
    return count;
}

// ========== САНИТАРИЯ ИМЕНИ ФАЙЛА ==========
String sanitizeFileName(String fileName) {
    fileName.replace("/", "_");
    fileName.replace("\\", "_");
    fileName.replace(":", "_");
    fileName.replace("*", "_");
    fileName.replace("?", "_");
    fileName.replace("\"", "_");
    fileName.replace("<", "_");
    fileName.replace(">", "_");
    fileName.replace("|", "_");
    fileName.trim();
    if (fileName.length() == 0) fileName = "track.mp3";
    else if (!fileName.endsWith(".mp3")) fileName += ".mp3";
    return fileName;
}

// ========== ЗАГРУЗКА ТРЕКА ==========
bool downloadTrack(const TrackInfo& track, int trackNum = 1, int totalTracks = 1) {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.printf("Track %d/%d\n", trackNum, totalTracks);
    M5Cardputer.Display.println(track.name);
    M5Cardputer.Display.println();

    String mp3Url = track.mp3Url;
    if (mp3Url.length() == 0) {
        M5Cardputer.Display.println("No direct link");
        delay(1500);
        return false;
    }

    String fileName = sanitizeFileName(track.fileName);
    String fullPath = String(musicFolder) + "/" + fileName;

    if (SD.exists(fullPath)) {
        M5Cardputer.Display.println("File already exists!");
        M5Cardputer.Display.println("Skipping...");
        delay(1500);
        return false;
    }

    M5Cardputer.Display.println("Downloading...");
    HTTPClient http;
    http.begin(mp3Url);
    http.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    int httpCode = http.GET();
    if (httpCode != 200) {
        http.end();
        M5Cardputer.Display.println("HTTP error");
        delay(1500);
        return false;
    }

    File file = SD.open(fullPath, FILE_WRITE);
    if (!file) {
        http.end();
        M5Cardputer.Display.println("SD write error");
        delay(1500);
        return false;
    }

    int totalLen = http.getSize();
    int received = 0;
    uint8_t buffer[1024];
    WiFiClient* stream = http.getStreamPtr();
    int lastPercent = -1;

    while (http.connected() && received < totalLen) {
        int available = stream->available();
        if (available) {
            int readBytes = stream->readBytes(buffer, min(available, 1024));
            file.write(buffer, readBytes);
            received += readBytes;
            int percent = (received * 100) / totalLen;
            if (percent != lastPercent) {
                lastPercent = percent;
                M5Cardputer.Display.setCursor(0, M5Cardputer.Display.height() - 10);
                showProgressBar(percent, 20);
            }
        }
        delay(1);
    }
    file.close();
    http.end();
    M5Cardputer.Display.println();
    bool success = (received == totalLen);
    M5Cardputer.Display.println(success ? "OK!" : "FAILED");
    delay(800);
    return success;
}

// ========== ПОИСК ==========
void performSearch(const String& query) {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.println("Searching...");
    M5Cardputer.Display.println(query);
    M5Cardputer.Display.println();

    HTTPClient http;
    String url = String(baseUrl) + searchPath + urlEncode(query);
    http.begin(url);
    http.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
    http.addHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
    http.addHeader("Accept-Language", "ru-RU,ru;q=0.8,en-US;q=0.5,en;q=0.3");
    http.addHeader("Connection", "keep-alive");

    M5Cardputer.Display.print("HTTP request...");
    int httpCode = http.GET();
    M5Cardputer.Display.println(" done");

    if (httpCode != 200) {
        M5Cardputer.Display.printf("HTTP error: %d", httpCode);
        delay(3000);
        http.end();
        currentMode = MODE_INPUT_SEARCH;
        return;
    }

    M5Cardputer.Display.println("Reading page...");
    String html = http.getString();
    http.end();
    delay(500);
    M5Cardputer.Display.println("HTML size: " + String(html.length()));
    // saveHtmlToSD("debug_search.html", html);

    M5Cardputer.Display.println("Parsing results...");
    int count = parseSearchResults(html, searchResults);
    M5Cardputer.Display.printf("Found %d tracks", count);
    delay(2000);
    currentMode = MODE_SHOW_RESULTS;
}

// ---------- ПОДСКАЗКИ ДЛЯ РУССКОЙ РАСКЛАДКИ ----------
void showRussianHint(int digit) {
    if (hintStartX != 0 || hintStartY != 0) {
        M5Cardputer.Display.fillRect(hintStartX, hintStartY, 120, 20, BLACK);
    }
    if (digit < 2 || digit > 9) return;
    int y = tableVisible ? (tableStartY - 15) : (M5Cardputer.Display.height() - 30);
    if (y < 0) y = M5Cardputer.Display.height() - 30;
    hintStartX = 5;
    hintStartY = y;
    M5Cardputer.Display.setCursor(hintStartX, hintStartY);
    M5Cardputer.Display.setTextColor(YELLOW, BLACK);
    M5Cardputer.Display.printf("[%d] <%s ^%s >%s v%s",
                               digit,
                               russianUTF8[digit][0],
                               russianUTF8[digit][1],
                               russianUTF8[digit][2],
                               russianUTF8[digit][3]);
    M5Cardputer.Display.setTextColor(WHITE, BLACK);
}

void showRussianLayoutTable() {
    if (tableVisible) return;
    tableStartY = M5Cardputer.Display.height() - 40;
    M5Cardputer.Display.setCursor(0, tableStartY);
    M5Cardputer.Display.setTextColor(CYAN, BLACK);
    M5Cardputer.Display.println("2 абвг   3 дежз   4 ийкл   5 мноп");
    M5Cardputer.Display.println("6 рсту   7 фхцч   8 шщъы   9 ьэюя");
    M5Cardputer.Display.setTextColor(WHITE, BLACK);
    tableVisible = true;
}

void hideRussianLayoutTable() {
    if (!tableVisible) return;
    M5Cardputer.Display.fillRect(0, tableStartY, M5Cardputer.Display.width(), 40, BLACK);
    tableVisible = false;
}

void clearHint() {
    if (hintStartX != 0 || hintStartY != 0) {
        M5Cardputer.Display.fillRect(hintStartX, hintStartY, 120, 20, BLACK);
        hintStartX = hintStartY = 0;
    }
}

// ---------- ВВОД СТРОКИ ----------
String readKeyboardInput() {
    String input = "";
    waitingForArrow = false;
    pendingDigit = 0;
    clearHint();
    if (tableVisible) hideRussianLayoutTable();

    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.println("Enter song name:");
    M5Cardputer.Display.println("Ctrl+Space - switch layout");
    M5Cardputer.Display.println("Press Enter when done");
    M5Cardputer.Display.println();
    M5Cardputer.Display.print("> ");

    uint16_t inputStartX = M5Cardputer.Display.getCursorX();
    uint16_t inputStartY = M5Cardputer.Display.getCursorY();
    uint16_t inputAreaWidth = M5Cardputer.Display.width() - inputStartX;

    int layoutX = M5Cardputer.Display.width() - 30;
    int layoutY = 2;

    auto redrawInput = [&]() {
        M5Cardputer.Display.fillRect(inputStartX, inputStartY, inputAreaWidth,
                                     M5Cardputer.Display.fontHeight(), BLACK);
        M5Cardputer.Display.setCursor(inputStartX, inputStartY);
        M5Cardputer.Display.print(input);
    };

    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

            // Ctrl+Space
            if (status.ctrl && status.word.size() > 0 && status.word[0] == ' ') {
                isRussianLayout = !isRussianLayout;
                waitingForArrow = false;
                pendingDigit = 0;
                clearHint();

                if (isRussianLayout) showRussianLayoutTable();
                else hideRussianLayoutTable();

                M5Cardputer.Display.fillRect(layoutX, layoutY, 28, 12, BLACK);
                M5Cardputer.Display.setCursor(layoutX, layoutY + 8);
                M5Cardputer.Display.setTextColor(GREEN, BLACK);
                M5Cardputer.Display.print(isRussianLayout ? "RU" : "EN");
                delay(500);
                M5Cardputer.Display.fillRect(layoutX, layoutY, 28, 12, BLACK);
                M5Cardputer.Display.setTextColor(WHITE, BLACK);
                redrawInput();
                continue;
            }

            // Постоянный индикатор
            M5Cardputer.Display.fillRect(layoutX, layoutY, 28, 12, BLACK);
            M5Cardputer.Display.setCursor(layoutX, layoutY + 8);
            M5Cardputer.Display.setTextColor(isRussianLayout ? CYAN : WHITE, BLACK);
            M5Cardputer.Display.print(isRussianLayout ? "RU" : "EN");
            M5Cardputer.Display.setTextColor(WHITE, BLACK);

            // Режим ожидания стрелки
            if (waitingForArrow) {
                int arrow = -1;
                for (auto c : status.word) {
                    if (c == ',') arrow = 0;      // влево
                    else if (c == ';') arrow = 1; // вверх
                    else if (c == '/') arrow = 2; // вправо
                    else if (c == '.') arrow = 3; // вниз
                }
                if (arrow != -1 && pendingDigit >= 2 && pendingDigit <= 9) {
                    input += russianUTF8[pendingDigit][arrow];
                    redrawInput();
                    waitingForArrow = false;
                    pendingDigit = 0;
                    clearHint();
                } else if (status.del) {
                    waitingForArrow = false;
                    pendingDigit = 0;
                    clearHint();
                } else if (status.enter) {
                    waitingForArrow = false;
                    pendingDigit = 0;
                    clearHint();
                }
                continue;
            }

            // Цифры в русской раскладке
            if (isRussianLayout && !waitingForArrow) {
                int digit = -1;
                for (auto c : status.word) {
                    if (c >= '2' && c <= '9') {
                        digit = c - '0';
                        break;
                    }
                }
                if (digit != -1) {
                    pendingDigit = digit;
                    waitingForArrow = true;
                    showRussianHint(digit);
                    continue;
                }
            }

            // Обычный ввод
            bool inputChanged = false;
            for (auto c : status.word) {
                if (c == 0 || c == '\r' || c == '\n') continue;
                if (isRussianLayout && (c >= '2' && c <= '9')) continue;
                input += c;
                inputChanged = true;
            }
            if (inputChanged) redrawInput();

            // Backspace
            if (status.del && input.length() > 0) {
                removeLastUTF8Char(input);
                redrawInput();
            }

            // Enter
            if (status.enter && input.length() > 0) {
                clearHint();
                if (tableVisible) hideRussianLayoutTable();
                return input;
            }
        }
        delay(20);
    }
}

// ========== ОТОБРАЖЕНИЕ РЕЗУЛЬТАТОВ ==========
void showResultsAndSelect() {
    const int itemsPerPage = 10;
    int totalItems = searchResults.size();
    if (totalItems == 0) {
        M5Cardputer.Display.fillScreen(BLACK);
        M5Cardputer.Display.setCursor(0, 0);
        M5Cardputer.Display.println("No tracks found.");
        M5Cardputer.Display.println("Check debug_search.html");
        delay(5000);
        currentMode = MODE_INPUT_SEARCH;
        return;
    }

    int totalPages = (totalItems + itemsPerPage - 1) / itemsPerPage;
    int currentPage = 0;
    int cursorPos = 0;

    while (true) {
        M5Cardputer.Display.fillScreen(BLACK);
        M5Cardputer.Display.setCursor(0, 0);
        M5Cardputer.Display.printf("Tracks (page %d/%d):\n", currentPage + 1, totalPages);

        int startIdx = currentPage * itemsPerPage;
        int endIdx = startIdx + itemsPerPage;
        if (endIdx > totalItems) endIdx = totalItems;

        for (int i = startIdx; i < endIdx; i++) {
            M5Cardputer.Display.print((i - startIdx == cursorPos) ? "> " : "  ");
            M5Cardputer.Display.print(i + 1);
            M5Cardputer.Display.print(". ");
            String name = searchResults[i].name;
            if (name.length() > 28) name = name.substring(0, 25) + "...";
            M5Cardputer.Display.println(name);
        }

        M5Cardputer.Display.println();
        M5Cardputer.Display.println("[,] [/] - page");
        M5Cardputer.Display.println("[;] [.] - move cursor");
        M5Cardputer.Display.println("ENTER - download selected");
        M5Cardputer.Display.println("0 - new search");

        bool redraw = false;
        bool downloadSelected = false;
        while (!redraw && !downloadSelected) {
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

                int nav = 0;
                for (auto c : status.word) {
                    if (c == ',') nav = -1;
                    if (c == '/') nav = +1;
                }
                if (nav == -1 && currentPage > 0) {
                    currentPage--;
                    cursorPos = 0;
                    redraw = true;
                } else if (nav == +1 && currentPage < totalPages - 1) {
                    currentPage++;
                    cursorPos = 0;
                    redraw = true;
                }

                int move = 0;
                for (auto c : status.word) {
                    if (c == ';') move = -1;
                    if (c == '.') move = +1;
                }
                if (move == -1 && cursorPos > 0) {
                    cursorPos--;
                    redraw = true;
                } else if (move == +1 && cursorPos < (endIdx - startIdx - 1)) {
                    cursorPos++;
                    redraw = true;
                }

                if (status.enter) {
                    downloadSelected = true;
                    break;
                }

                for (auto c : status.word) {
                    if (c == '0') {
                        currentMode = MODE_INPUT_SEARCH;
                        return;
                    }
                }
            }
            delay(20);
        }

        if (downloadSelected) {
            int selectedIdx = currentPage * itemsPerPage + cursorPos;
            if (selectedIdx >= 0 && selectedIdx < totalItems) {
                downloadTrack(searchResults[selectedIdx], 1, 1);
                delay(1000);
            }
        }
    }
}

// ========== SETUP ==========
void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setFont(&myFont);

    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println("Music Downloader");

    if (!initSD()) {
        M5Cardputer.Display.println("SD Card error!");
        while (true) delay(1000);
    }
    initWiFi();      // <-- добавлено
    delay(1000);
    M5Cardputer.Display.fillScreen(BLACK);
}

// ========== LOOP ==========
void loop() {
    switch (currentMode) {
        case MODE_INPUT_SEARCH:
            searchQuery = readKeyboardInput();
            if (searchQuery.length() > 0) performSearch(searchQuery);
            break;
        case MODE_SHOW_RESULTS:
            showResultsAndSelect();
            break;
        case MODE_DOWNLOADING:
            break;
    }
    delay(50);
}