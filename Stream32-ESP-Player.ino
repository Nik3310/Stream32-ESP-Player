// =====================================================================================
// English implementation note
// =====================================================================================

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>            
#include <XPT2046_Touchscreen.h> 
#include <Preferences.h>         // English implementation note
#include "BluetoothA2DPSource.h"
#include "esp_gap_bt_api.h"

#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

// =====================================================================================
// English implementation note
// =====================================================================================
constexpr uint8_t SD_CS = 5;
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

TFT_eSPI tft = TFT_eSPI();
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
Preferences preferences;

// =====================================================================================
// English implementation note
// =====================================================================================
enum UIState {
    STATE_MENU,
    STATE_PLAYER,
    STATE_TRACK_LIST,
    STATE_LIKES_LIST,
    STATE_BT_MANAGER,
    STATE_SETTINGS,
    STATE_BT_SCAN,
    STATE_SCREEN_SETTINGS,
    STATE_PLAYBACK_SETTINGS,
    STATE_INFO,
    STATE_RESET_CONFIRM,
    STATE_THEME_SETTINGS
};
UIState currentState = STATE_MENU;

// =====================================================================================
// English implementation note
// =====================================================================================
static uint16_t COLOR_BG;             
static uint16_t COLOR_PANEL;          
static uint16_t COLOR_PANEL_BORDER;   
static uint16_t COLOR_ACCENT;         
static uint16_t COLOR_ACCENT_DIM;     
static uint16_t COLOR_TEXT_PRIMARY;   
static uint16_t COLOR_TEXT_SECONDARY; 
static uint16_t COLOR_HEART_ACTIVE;   
static uint16_t COLOR_HEART_INACTIVE; 
static uint16_t COLOR_TRACK_BG;       
static uint16_t COLOR_BT_CONNECTED;
static uint16_t COLOR_BT_DISCONNECTED;

constexpr uint8_t TFT_BL_PIN = 21;
uint8_t screenBrightness = 220;
uint8_t screenTimeoutMinutes = 0;
uint8_t themeIndex = 0;
bool screenLocked = false;
unsigned long lastScreenActivity = 0;

void applyScreenBrightness() {
    ledcWrite(TFT_BL_PIN, screenBrightness);
}

void initColorTheme() {
    COLOR_BG             = tft.color565(16, 18, 22);
    COLOR_PANEL          = tft.color565(28, 31, 37);
    COLOR_PANEL_BORDER   = tft.color565(45, 50, 58);
    COLOR_ACCENT         = tft.color565(0, 224, 193);
    COLOR_ACCENT_DIM     = tft.color565(0, 120, 105);
    COLOR_TEXT_PRIMARY   = tft.color565(235, 238, 240);
    COLOR_TEXT_SECONDARY = tft.color565(140, 146, 156);
    COLOR_HEART_ACTIVE   = tft.color565(255, 82, 110);
    COLOR_HEART_INACTIVE = tft.color565(78, 83, 92);
    COLOR_TRACK_BG       = tft.color565(40, 44, 51);
    COLOR_BT_CONNECTED   = tft.color565(52, 145, 255);
    COLOR_BT_DISCONNECTED = tft.color565(235, 78, 78);
}

void applyTheme(uint8_t theme) {
    themeIndex = theme % 5;
    if (themeIndex == 0) {
        COLOR_ACCENT = tft.color565(112, 226, 125);
        COLOR_ACCENT_DIM = tft.color565(55, 130, 70);
    } else if (themeIndex == 1) {
        COLOR_ACCENT = tft.color565(78, 150, 255);
        COLOR_ACCENT_DIM = tft.color565(45, 88, 160);
    } else if (themeIndex == 2) {
        COLOR_ACCENT = tft.color565(190, 105, 255);
        COLOR_ACCENT_DIM = tft.color565(105, 55, 155);
    } else if (themeIndex == 3) {
        COLOR_ACCENT = tft.color565(255, 170, 70);
        COLOR_ACCENT_DIM = tft.color565(155, 90, 35);
    } else {
        COLOR_ACCENT = tft.color565(255, 90, 145);
        COLOR_ACCENT_DIM = tft.color565(155, 45, 85);
    }
}

// =====================================================================================
// English implementation note
// =====================================================================================
// English implementation note
constexpr int SCREEN_W = 240;
constexpr int SCREEN_H = 320;

constexpr int HEADER_H = 35;
constexpr int MENU_BTN_W = 105;
constexpr int MENU_BTN_H = 70;
constexpr int MENU_BTN_X_LEFT = 10;
constexpr int MENU_BTN_X_RIGHT = 125;
constexpr int MENU_BTN_Y[3] = {48, 128, 208};
constexpr int NAV_Y = 280;
constexpr int NAV_H = 40;

// English implementation note
constexpr int PL_ART_X = 65, PL_ART_Y = 38, PL_ART_W = 110, PL_ART_H = 110;

// English implementation note
constexpr int PL_INFO_X = 20;
constexpr int PL_TITLE_Y = 153;
constexpr int PL_ARTIST_Y = 174;
constexpr int PL_HEART_CX = 220, PL_HEART_CY = 158, PL_HEART_R = 9;

// English implementation note
constexpr int PL_PROGRESS_X = 20, PL_PROGRESS_Y = 191, PL_PROGRESS_W = 200, PL_PROGRESS_H = 8;

// English implementation note
constexpr int PL_BTN_Y = 207, PL_BTN_H = 32;
constexpr int PL_PREV_X = 25, PL_PREV_W = 45;
constexpr int PL_PLAY_X = 97, PL_PLAY_W = 46;
constexpr int PL_NEXT_X = 170, PL_NEXT_W = 45;

// English implementation note
constexpr int PL_VOL_Y = 245, PL_VOL_H = 25;
constexpr int PL_VOLDOWN_X = 20, PL_VOLDOWN_W = 40;
constexpr int PL_VOLUP_X = 180, PL_VOLUP_W = 40;
constexpr int PL_VOLTEXT_X = 70, PL_VOLTEXT_W = 100;

// English implementation note
constexpr int LIST_ITEMS_PER_PAGE = 6;
constexpr int LIST_ITEM_H = 26;
constexpr int LIST_ITEM_Y_START = 45;
constexpr int LIST_ITEM_GAP = 2;

// =====================================================================================
// English implementation note
// =====================================================================================
#define READ_BUF_SIZE 2048
static uint8_t mp3_read_buf[READ_BUF_SIZE];
static int16_t pcm_output_buffer[MINIMP3_MAX_SAMPLES_PER_FRAME * 2];

#define PCM_QUEUE_SIZE 2048
static QueueHandle_t pcm_frame_queue = NULL;

static BluetoothA2DPSource a2dp_source;
static File audioFile;
static mp3dec_t mp3d;
static mp3dec_frame_info_t info;

static TaskHandle_t mp3DecoderTaskHandle = NULL;
static volatile bool is_playing = false;
static volatile bool decoder_reset_requested = false;

static volatile size_t g_file_size = 0;
static volatile size_t g_file_bytes_read = 0;

// =====================================================================================
// English implementation note
// =====================================================================================
#define MAX_TRACKS 50
String playlist[MAX_TRACKS];
bool   isLiked[MAX_TRACKS];           
int total_tracks = 0;
int current_track_index = 0;
volatile bool need_next_track = false;

const char* LIKES_FILE_PATH = "/tracks/likes.txt";

int trackList_currentPage = 0;
int likesList_currentPage = 0;

static uint8_t bt_volume = 70;
static bool autoPlayEnabled = false;
constexpr const char* PLAYER_VERSION = "0.0.10-beta";

// =============================================================================
// English implementation note
// English implementation note
// English implementation note
// =============================================================================
const char* const BT_DEVICES[] = {
    "BD2",
    "CMF Buds Pro 2",
    "G435 Bluetooth Gaming Headset"
};
constexpr uint8_t BT_DEVICE_COUNT = sizeof(BT_DEVICES) / sizeof(BT_DEVICES[0]);

uint8_t savedBTDeviceIndex = 0;
String savedBTDevice = BT_DEVICES[0];
static volatile bool bt_connected = false;
static volatile bool bt_audio_started = false;
static volatile bool play_requested = false;

constexpr uint8_t MAX_BT_SCAN_RESULTS = 6;
String btScanNames[MAX_BT_SCAN_RESULTS];
esp_bd_addr_t btScanAddresses[MAX_BT_SCAN_RESULTS];
volatile uint8_t btScanCount = 0;
volatile bool btScanActive = false;
unsigned long btScanStartedAt = 0;
bool hasSavedBTAddress = false;
esp_bd_addr_t savedBTAddress = {0, 0, 0, 0, 0, 0};

void onBluetoothConnectionState(esp_a2d_connection_state_t state, void *obj) {
    bool connected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);
    bt_connected = connected;
    if (!connected) {
        if (is_playing) play_requested = true;
        is_playing = false;
        bt_audio_started = false;
    }
}

void onBluetoothAudioState(esp_a2d_audio_state_t state, void *obj) {
    bt_audio_started = (state == ESP_A2D_AUDIO_STATE_STARTED);
    if (bt_audio_started && play_requested) {
        decoder_reset_requested = true;
        if (pcm_frame_queue) xQueueReset(pcm_frame_queue);
        is_playing = true;
    } else if (!bt_audio_started) {
        is_playing = false;
    }
}

// English implementation note
void drawCurrentStateUI();
void updatePlayPauseButton();
void updateLikeButton();
void updateProgressBar(bool forceRedraw = false);
void updateVolumeUI();
String truncateToWidth(const String& text, int maxWidth, uint8_t font);
int32_t get_sound_data(Frame *data, int32_t frame_count);
void mp3DecoderTask(void *pvParameters);
bool pointInRect(int x, int y, int rx, int ry, int rw, int rh);
void startBluetoothDevice(uint8_t deviceIndex);
bool bluetoothScanCallback(const char* name, esp_bd_addr_t address, int rssi);
void startBluetoothScan();
void selectBluetoothScanResult(uint8_t index);

// =====================================================================================
// English implementation note
// =====================================================================================
void seekToRatio(float ratio) {
    if (g_file_size == 0 || !audioFile) return;

    bool was_playing = is_playing;
    is_playing = false;
    vTaskDelay(pdMS_TO_TICKS(40)); 

    size_t target_pos = (size_t)(ratio * g_file_size);
    audioFile.seek(target_pos);
    g_file_bytes_read = target_pos;

    xQueueReset(pcm_frame_queue);
    
    is_playing = was_playing;
    updateProgressBar(true);
}

// =====================================================================================
// English implementation note
// =====================================================================================
String trackDisplayName(const String& path) {
    String name = path;
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);
    if (name.endsWith(".mp3") || name.endsWith(".MP3")) name.remove(name.length() - 4);
    return name;
}

void sortPlaylist() {
    for (int i = 0; i < total_tracks - 1; i++) {
        for (int j = i + 1; j < total_tracks; j++) {
            String a = trackDisplayName(playlist[i]);
            String b = trackDisplayName(playlist[j]);
            a.toLowerCase();
            b.toLowerCase();
            if (a > b) {
                String path = playlist[i];
                playlist[i] = playlist[j];
                playlist[j] = path;
            }
        }
    }
}

void loadPlaylist() {
    File dir = SD.open("/tracks");
    if (!dir) {
        Serial.println("Папка /tracks не найдена!");
        SD.mkdir("/tracks"); 
        return;
    }

    total_tracks = 0;
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;

        if (!entry.isDirectory()) {
            String fileName = entry.name();
            if (fileName.endsWith(".mp3") || fileName.endsWith(".MP3")) {
                if (entry.size() > 0 && total_tracks < MAX_TRACKS) {
                    playlist[total_tracks] = "/tracks/" + fileName;
                    isLiked[total_tracks] = false;
                    total_tracks++;
                } else if (entry.size() == 0) {
                    Serial.printf("[SD] Skipping empty track: %s\n", fileName.c_str());
                }
            }
        }
        entry.close();
    }
    dir.close();
    sortPlaylist();
}

void loadLikes() {
    if (!SD.exists(LIKES_FILE_PATH)) return;
    File f = SD.open(LIKES_FILE_PATH, FILE_READ);
    if (!f) {
        Serial.println("[SD] Could not open likes file");
        return;
    }

    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        for (int i = 0; i < total_tracks; i++) {
            if (playlist[i] == line) {
                isLiked[i] = true;
                break;
            }
        }
    }
    f.close();
}

void saveLikes() {
    const char* tmpPath = "/tracks/likes.tmp";
    File f = SD.open(tmpPath, FILE_WRITE);
    if (!f) {
        Serial.println("[SD] Could not save likes");
        return;
    }

    for (int i = 0; i < total_tracks; i++) {
        if (isLiked[i]) f.println(playlist[i]);
    }
    f.close();
    SD.remove(LIKES_FILE_PATH);
    if (!SD.rename(tmpPath, LIKES_FILE_PATH)) {
        Serial.println("[SD] Could not replace likes file");
    }
}

void toggleLike() {
    if (total_tracks == 0) return;
    isLiked[current_track_index] = !isLiked[current_track_index];
    saveLikes();
    updateLikeButton();
}

void switchTrack(int direction) {
    if (total_tracks == 0) return;

    bool wasPlaying = is_playing;
    is_playing = false;
    decoder_reset_requested = true;
    vTaskDelay(pdMS_TO_TICKS(50)); 

    if (audioFile) audioFile.close();
    xQueueReset(pcm_frame_queue);

    current_track_index += direction;
    if (current_track_index >= total_tracks) current_track_index = 0;
    if (current_track_index < 0) current_track_index = total_tracks - 1;

    audioFile = SD.open(playlist[current_track_index].c_str());
    if (audioFile) {
        g_file_size = audioFile.size();
        g_file_bytes_read = 0;
        is_playing = wasPlaying;
    } else {
        Serial.printf("[SD] Could not open track: %s\n", playlist[current_track_index].c_str());
        g_file_size = 0;
        g_file_bytes_read = 0;
        is_playing = false;
    }

    if (currentState == STATE_PLAYER && !screenLocked) {
        drawCurrentStateUI();
    }
}

void startBluetoothDevice(uint8_t deviceIndex) {
    if (BT_DEVICE_COUNT == 0) return;
    savedBTDeviceIndex = deviceIndex % BT_DEVICE_COUNT;
    savedBTDevice = BT_DEVICES[savedBTDeviceIndex];
    hasSavedBTAddress = false;

    preferences.begin("bt_pref", false);
    preferences.putUChar("device_idx", savedBTDeviceIndex);
    preferences.putString("bt_name", savedBTDevice);
    preferences.end();

    Serial.print("[BT] Starting device: ");
    Serial.println(savedBTDevice);
    bt_connected = false;

    // English implementation note
    // English implementation note
    // English implementation note
    a2dp_source.end(false);
    delay(250);
    if (hasSavedBTAddress) {
        a2dp_source.set_auto_reconnect(savedBTAddress, 1);
    }
    a2dp_source.set_on_connection_state_changed(onBluetoothConnectionState);
    a2dp_source.set_on_audio_state_changed(onBluetoothAudioState);
    a2dp_source.start(savedBTDevice.c_str(), get_sound_data);
    a2dp_source.set_volume(bt_volume);
    xQueueReset(pcm_frame_queue);
    is_playing = false;
}

bool bluetoothScanCallback(const char* name, esp_bd_addr_t address, int rssi) {
    if (!btScanActive || name == nullptr || name[0] == '\0') return false;
    for (uint8_t i = 0; i < btScanCount; i++) {
        if (memcmp(btScanAddresses[i], address, ESP_BD_ADDR_LEN) == 0) return false;
    }
    if (btScanCount < MAX_BT_SCAN_RESULTS) {
        btScanNames[btScanCount] = String(name);
        memcpy(btScanAddresses[btScanCount], address, ESP_BD_ADDR_LEN);
        Serial.print("[BT] Found: ");
        Serial.print(name);
        Serial.print("  RSSI: ");
        Serial.println(rssi);
        btScanCount++;
    }
    return false; // English implementation note
}

void startBluetoothScan() {
    btScanCount = 0;
    btScanActive = true;
    btScanStartedAt = millis();
    if (a2dp_source.is_connected()) {
        a2dp_source.disconnect();
        delay(300);
    }
    a2dp_source.set_auto_reconnect(false);
    a2dp_source.set_valid_cod_service(ESP_BT_COD_SRVC_RENDERING |
                                      ESP_BT_COD_SRVC_AUDIO |
                                      ESP_BT_COD_SRVC_TELEPHONY);
    a2dp_source.set_ssid_callback(bluetoothScanCallback);
    esp_err_t scanResult = esp_bt_gap_start_discovery(
        ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    Serial.printf("[BT] SCAN START result=%d\n", (int)scanResult);
    currentState = STATE_BT_SCAN;
    drawCurrentStateUI();
}

void selectBluetoothScanResult(uint8_t index) {
    if (index >= btScanCount) return;
    memcpy(savedBTAddress, btScanAddresses[index], ESP_BD_ADDR_LEN);
    hasSavedBTAddress = true;
    savedBTDevice = btScanNames[index];
    preferences.begin("bt_pref", false);
    preferences.putString("bt_name", savedBTDevice);
    preferences.putBytes("bt_addr", savedBTAddress, ESP_BD_ADDR_LEN);
    preferences.end();
    btScanActive = false;
    a2dp_source.set_ssid_callback(nullptr);
    a2dp_source.end(false);
    delay(250);
    a2dp_source.set_auto_reconnect(savedBTAddress, 1);
    a2dp_source.set_on_connection_state_changed(onBluetoothConnectionState);
    a2dp_source.set_on_audio_state_changed(onBluetoothAudioState);
    a2dp_source.start(savedBTDevice.c_str(), get_sound_data);
    a2dp_source.set_volume(bt_volume);
    xQueueReset(pcm_frame_queue);
    is_playing = false;
    currentState = STATE_BT_MANAGER;
    drawCurrentStateUI();
}

// =====================================================================================
// English implementation note
// =====================================================================================
void drawPlayIcon(int cx, int cy, int size, uint16_t color) {
    tft.fillTriangle(cx - size / 2, cy - size / 2, cx - size / 2, cy + size / 2, cx + size / 2, cy, color);
}

void drawPauseIcon(int cx, int cy, int size, uint16_t color) {
    int barW = size / 3;
    int gap  = size / 4;
    tft.fillRoundRect(cx - gap - barW, cy - size / 2, barW, size, 2, color);
    tft.fillRoundRect(cx + gap, cy - size / 2, barW, size, 2, color);
}

void drawPrevIcon(int cx, int cy, int size, uint16_t color) {
    int offset = size / 3;
    tft.fillTriangle(cx, cy - size / 2, cx, cy + size / 2, cx - size / 2, cy, color);
    tft.fillTriangle(cx + offset, cy - size / 2, cx + offset, cy + size / 2, cx + offset - size / 2, cy, color);
}

void drawNextIcon(int cx, int cy, int size, uint16_t color) {
    int offset = size / 3;
    tft.fillTriangle(cx, cy - size / 2, cx, cy + size / 2, cx + size / 2, cy, color);
    tft.fillTriangle(cx - offset, cy - size / 2, cx - offset, cy + size / 2, cx - offset + size / 2, cy, color);
}

void drawHeartIcon(int cx, int cy, int r, uint16_t color) {
    tft.fillCircle(cx - r / 2, cy - r / 4, r / 2, color);
    tft.fillCircle(cx + r / 2, cy - r / 4, r / 2, color);
    tft.fillTriangle(cx - r, cy - r / 6, cx + r, cy - r / 6, cx, cy + r, color);
}

void drawVectorMusicNote(int x, int y, int w, int h, uint16_t color) {
    int note1_x = x + 35;
    int note1_y = y + h - 30;
    int note2_x = x + w - 35;
    int note2_y = y + h - 42;
    
    tft.fillEllipse(note1_x, note1_y, 10, 8, color);
    tft.fillEllipse(note2_x, note2_y, 10, 8, color);
    
    tft.fillRect(note1_x + 6, y + 25, 4, note1_y - y - 25, color);
    tft.fillRect(note2_x + 6, y + 13, 4, note2_y - y - 13, color);
    
    tft.fillTriangle(note1_x + 6, y + 25, note2_x + 10, y + 13, note2_x + 10, y + 23, color);
    tft.fillTriangle(note1_x + 6, y + 25, note1_x + 6, y + 35, note2_x + 10, y + 23, color);
}

void drawBluetoothStatusIcon() {
    const int cx = SCREEN_W - 18;
    const int cy = HEADER_H / 2;
    uint16_t color = bt_connected ? COLOR_BT_CONNECTED : COLOR_BT_DISCONNECTED;
    int s = 8;
    tft.drawLine(cx, cy - s, cx, cy + s, color);
    tft.drawLine(cx, cy - s, cx + 6, cy - 3, color);
    tft.drawLine(cx + 6, cy - 3, cx, cy, color);
    tft.drawLine(cx, cy, cx + 6, cy + 3, color);
    tft.drawLine(cx + 6, cy + 3, cx, cy + s, color);
    tft.drawLine(cx - 5, cy - 5, cx, cy, color);
    tft.drawLine(cx - 5, cy + 5, cx, cy, color);
}

void drawBluetoothLogo(int cx, int cy, int size, uint16_t color) {
    int s = size / 2;
    tft.drawLine(cx, cy - s, cx, cy + s, color);
    tft.drawLine(cx, cy - s, cx + s, cy - s / 2, color);
    tft.drawLine(cx + s, cy - s / 2, cx, cy, color);
    tft.drawLine(cx, cy, cx + s, cy + s / 2, color);
    tft.drawLine(cx + s, cy + s / 2, cx, cy + s, color);
    tft.drawLine(cx - s, cy - s / 2, cx, cy, color);
    tft.drawLine(cx - s, cy + s / 2, cx, cy, color);
}

void drawBackIcon(int cx, int cy, uint16_t color) {
    tft.drawLine(cx + 7, cy, cx - 6, cy, color);
    tft.drawLine(cx - 6, cy, cx, cy - 6, color);
    tft.drawLine(cx - 6, cy, cx, cy + 6, color);
}

void drawNavIcon(uint8_t index, int cx, int cy, uint16_t color) {
    if (index == 0) {
        tft.drawLine(cx - 8, cy - 1, cx, cy - 8, color);
        tft.drawLine(cx, cy - 8, cx + 8, cy - 1, color);
        tft.drawLine(cx - 6, cy - 2, cx - 6, cy + 8, color);
        tft.drawLine(cx + 6, cy - 2, cx + 6, cy + 8, color);
        tft.drawFastHLine(cx - 6, cy + 8, 12, color);
        tft.drawFastVLine(cx, cy + 3, 5, color);
    } else if (index == 1) {
        for (int i = -1; i <= 1; i++) { tft.drawFastHLine(cx - 2, cy + i * 5, 10, color); tft.fillCircle(cx - 7, cy + i * 5, 1, color); }
    } else if (index == 2) {
        drawHeartIcon(cx, cy, 7, color);
    } else if (index == 3) {
        tft.drawCircle(cx, cy, 2, color); tft.drawCircle(cx, cy, 6, color); tft.drawFastVLine(cx, cy + 6, 5, color);
    } else {
        tft.drawCircle(cx, cy, 3, color);
        for (int i = 0; i < 8; i++) { float a = i * 0.7854f; tft.drawLine(cx + (int)(7 * cos(a)), cy + (int)(7 * sin(a)), cx + (int)(9 * cos(a)), cy + (int)(9 * sin(a)), color); }
    }
}

void maskCoverCorners(int x, int y, int w, int h, int radius) {
    // English implementation note
    // English implementation note
    for (int py = 0; py < radius; py++) {
        for (int px = 0; px < radius; px++) {
            int dx = radius - 1 - px;
            int dy = radius - 1 - py;
            if (dx * dx + dy * dy >= radius * radius) {
                tft.drawPixel(x + px, y + py, COLOR_BG);
                tft.drawPixel(x + w - 1 - px, y + py, COLOR_BG);
                tft.drawPixel(x + px, y + h - 1 - py, COLOR_BG);
                tft.drawPixel(x + w - 1 - px, y + h - 1 - py, COLOR_BG);
            }
        }
    }
}

void drawHeader(const char* title, bool hasBackButton = true) {
    tft.fillRect(0, 0, SCREEN_W, HEADER_H, COLOR_PANEL);
    tft.drawFastHLine(0, HEADER_H - 1, SCREEN_W, COLOR_PANEL_BORDER);
    
    if (hasBackButton) {
        tft.fillRoundRect(8, 4, 30, 26, 7, COLOR_BG);
        tft.drawRoundRect(8, 4, 30, 26, 7, COLOR_PANEL_BORDER);
        drawBackIcon(23, 17, COLOR_TEXT_PRIMARY);
    }

    drawBluetoothStatusIcon();
}

void drawBottomNavigation(uint8_t activeTab) {
    tft.fillRect(0, NAV_Y, SCREEN_W, NAV_H, COLOR_PANEL);
    tft.drawFastHLine(0, NAV_Y, SCREEN_W, COLOR_PANEL_BORDER);
    const char* labels[] = {"Home", "List", "Likes", "Radio", "Settings"};
    const int centers[] = {24, 72, 120, 168, 216};
    for (uint8_t i = 0; i < 5; i++) {
        uint16_t color = (i == activeTab) ? COLOR_ACCENT : COLOR_TEXT_SECONDARY;
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(color, COLOR_PANEL);
        tft.drawCentreString(labels[i], centers[i], NAV_Y + 29, 1);
        drawNavIcon(i, centers[i], NAV_Y + 14, color);
    }
}

// English implementation note
void drawCoverArt(int x, int y, int w, int h) {
    // English implementation note
    tft.fillRect(x, y, w, h, COLOR_BG);
    if (total_tracks == 0) {
        drawVectorMusicNote(x, y, w, h, COLOR_ACCENT);
        return;
    }

    String rawPath = playlist[current_track_index];
    rawPath.replace(".mp3", ".raw");
    rawPath.replace(".MP3", ".raw");

    if (!SD.exists(rawPath)) {
        drawVectorMusicNote(x, y, w, h, COLOR_ACCENT);
        return;
    }

    File imgFile = SD.open(rawPath, FILE_READ);
    if (!imgFile) {
        drawVectorMusicNote(x, y, w, h, COLOR_ACCENT);
        return;
    }

    uint16_t lineBuffer[110]; 
    for (int row = 0; row < h; row++) {
        imgFile.read((uint8_t*)lineBuffer, w * 2);
        tft.pushImage(x, y + row, w, 1, lineBuffer);
    }
    imgFile.close();
    maskCoverCorners(x, y, w, h, 12);
}

// =====================================================================================
// English implementation note
// =====================================================================================
void drawMainMenu() {
    tft.fillScreen(COLOR_BG);
    drawHeader("STREAM 32 PLAYER", false);
    const char* menuLabelsNew[] = {"PLAYER", "TRACKS", "LIKES", "SETTINGS"};
    for (int i = 0; i < 4; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = col == 0 ? MENU_BTN_X_LEFT : MENU_BTN_X_RIGHT;
        int y = MENU_BTN_Y[row];
        
        tft.fillRoundRect(x, y, MENU_BTN_W, MENU_BTN_H, 10, COLOR_PANEL);
        tft.drawRoundRect(x, y, MENU_BTN_W, MENU_BTN_H, 10, COLOR_PANEL_BORDER);
        uint16_t iconColor = COLOR_ACCENT;
        if (i == 0) drawPlayIcon(x + MENU_BTN_W / 2, y + 25, 22, iconColor);
        else if (i == 1) {
            int cx = x + MENU_BTN_W / 2, cy = y + 25;
            for (int row = -1; row <= 1; row++) {
                tft.drawFastHLine(cx - 2, cy + row * 7, 15, iconColor);
                tft.fillCircle(cx - 10, cy + row * 7, 2, iconColor);
            }
        } else if (i == 2) {
            drawHeartIcon(x + MENU_BTN_W / 2, y + 25, 11, iconColor);
        } else {
            int cx = x + MENU_BTN_W / 2, cy = y + 25;
            tft.drawCircle(cx, cy, 4, iconColor);
            for (int tick = 0; tick < 8; tick++) {
                float angle = tick * 0.7854f;
                tft.drawLine(cx + (int)(8 * cos(angle)), cy + (int)(8 * sin(angle)), cx + (int)(11 * cos(angle)), cy + (int)(11 * sin(angle)), iconColor);
            }
        }
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setTextDatum(MC_DATUM);
        tft.drawCentreString(menuLabelsNew[i], x + MENU_BTN_W / 2, y + 51, 1);
    }
    tft.fillRoundRect(10, 208, SCREEN_W - 20, 62, 10, COLOR_PANEL);
    tft.drawRoundRect(10, 208, SCREEN_W - 20, 62, 10, COLOR_PANEL_BORDER);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.drawString("NOW PLAYING", 20, 216, 1);
    String homeTrack = total_tracks > 0 ? trackDisplayName(playlist[current_track_index]) : "No tracks on SD";
    homeTrack = truncateToWidth(homeTrack, 175, 2);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.drawString(homeTrack, 20, 234, 2);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.drawString(is_playing ? "Playing" : "Paused", 20, 252, 2);
    drawBottomNavigation(0);
}

void drawSettingsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("SETTINGS");
    const char* labels[] = {"Bluetooth", "Screen", "Playback", "Theme", "Information", "Reset settings"};
    for (uint8_t i = 0; i < 6; i++) {
        int y = 48 + i * 34;
        tft.fillRoundRect(10, y, SCREEN_W - 20, 28, 8, COLOR_PANEL);
        tft.drawRoundRect(10, y, SCREEN_W - 20, 28, 8, COLOR_PANEL_BORDER);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.drawString(labels[i], 20, y + 8, 2);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.drawCentreString(">", 218, y + 10, 2);
    }
    drawBottomNavigation(4);
}

void drawPlaybackSettingsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("PLAYBACK");
    tft.fillRoundRect(10, 55, SCREEN_W - 20, 38, 8, COLOR_PANEL);
    tft.drawRoundRect(10, 55, SCREEN_W - 20, 38, 8, COLOR_PANEL_BORDER);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.drawString("Auto-play on connect", 20, 67, 2);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(autoPlayEnabled ? COLOR_ACCENT : COLOR_TEXT_SECONDARY);
    tft.drawCentreString(autoPlayEnabled ? "ON" : "OFF", 205, 75, 2);
    drawBottomNavigation(4);
}

void drawScreenSettingsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("SCREEN");
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.drawString("Brightness", 18, 52, 2);
    tft.setTextColor(COLOR_ACCENT);
    tft.drawRightString(String((int)((screenBrightness * 100UL) / 255)) + "%", 220, 52, 2);
    tft.fillRoundRect(18, 78, 204, 8, 4, COLOR_TRACK_BG);
    tft.fillRoundRect(18, 78, (screenBrightness * 204UL) / 255, 8, 4, COLOR_ACCENT);
    tft.fillCircle(18 + (screenBrightness * 204UL) / 255, 82, 6, COLOR_ACCENT);

    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.drawString("Screen timeout", 18, 112, 2);
    const char* timeoutLabel = screenTimeoutMinutes == 0 ? "OFF" : (screenTimeoutMinutes == 1 ? "1 min" : (screenTimeoutMinutes == 5 ? "5 min" : "10 min"));
    tft.setTextColor(COLOR_ACCENT);
    tft.drawRightString(timeoutLabel, 220, 112, 2);
    const char* options[] = {"1 min", "5 min", "10 min", "OFF"};
    const uint8_t values[] = {1, 5, 10, 0};
    for (uint8_t i = 0; i < 4; i++) {
        int x = 12 + i * 57;
        bool selected = screenTimeoutMinutes == values[i];
        tft.fillRoundRect(x, 145, 51, 30, 7, selected ? COLOR_ACCENT_DIM : COLOR_PANEL);
        tft.drawRoundRect(x, 145, 51, 30, 7, selected ? COLOR_ACCENT : COLOR_PANEL_BORDER);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.drawCentreString(options[i], x + 25, 153, 1);
    }
    drawBottomNavigation(4);
}

void drawThemeSettingsScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("THEME");
    const char* names[] = {"Classic Green", "Blue", "Purple", "Amber", "Rose"};
    for (uint8_t i = 0; i < 5; i++) {
        int y = 48 + i * 44;
        bool selected = themeIndex == i;
        tft.fillRoundRect(18, y, SCREEN_W - 36, 36, 9, selected ? COLOR_ACCENT_DIM : COLOR_PANEL);
        tft.drawRoundRect(18, y, SCREEN_W - 36, 36, 9, selected ? COLOR_ACCENT : COLOR_PANEL_BORDER);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.drawCentreString(names[i], SCREEN_W / 2, y + 10, 2);
    }
    drawBottomNavigation(4);
}

void drawScreenLocked() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("TAP", SCREEN_W / 2, SCREEN_H / 2 - 16, 4);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.drawCentreString("TO WAKE", SCREEN_W / 2, SCREEN_H / 2 + 18, 2);
}

void drawInfoScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("INFORMATION");
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_ACCENT);
    tft.drawCentreString("Stream32 Player", SCREEN_W / 2, 88, 4);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.drawCentreString(String("Version ") + PLAYER_VERSION, SCREEN_W / 2, 125, 2);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.drawCentreString("Developed with love", SCREEN_W / 2, 165, 2);
    tft.setTextColor(COLOR_HEART_ACTIVE);
    tft.drawCentreString("by Stream32 Team", SCREEN_W / 2, 190, 2);
    drawBottomNavigation(4);
}

void drawResetConfirmScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("SETTINGS");
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.drawCentreString("Reset all settings?", SCREEN_W / 2, 105, 2);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.drawCentreString("Bluetooth and volume", SCREEN_W / 2, 128, 2);
    tft.drawCentreString("will return to defaults", SCREEN_W / 2, 146, 2);
    tft.fillRoundRect(18, 178, 96, 34, 9, COLOR_PANEL);
    tft.drawRoundRect(18, 178, 96, 34, 9, COLOR_PANEL_BORDER);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.drawCentreString("CANCEL", 66, 187, 2);
    tft.fillRoundRect(126, 178, 96, 34, 9, COLOR_ACCENT_DIM);
    tft.drawRoundRect(126, 178, 96, 34, 9, COLOR_ACCENT);
    tft.drawCentreString("RESET", 174, 187, 2);
    drawBottomNavigation(4);
}

void drawPlayerScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("NOW PLAYING");

    // English implementation note
    drawCoverArt(PL_ART_X, PL_ART_Y, PL_ART_W, PL_ART_H);

    // English implementation note
    String rawName = (total_tracks > 0) ? trackDisplayName(playlist[current_track_index]) : "No tracks!";

    String artist = "Unknown Artist";
    String title = rawName;

    int hyphenIdx = rawName.indexOf(" - ");
    if (hyphenIdx != -1) {
        artist = rawName.substring(0, hyphenIdx);
        title = rawName.substring(hyphenIdx + 3);
    }

    title = truncateToWidth(title, SCREEN_W - PL_INFO_X - 35, 4);
    artist = truncateToWidth(artist, SCREEN_W - PL_INFO_X - 35, 2);

    tft.setTextDatum(TL_DATUM); 
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.drawString(title, PL_INFO_X, PL_TITLE_Y, 4);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.drawString(artist, PL_INFO_X, PL_ARTIST_Y, 2);

    updateLikeButton();

    // English implementation note
    tft.fillRoundRect(PL_PROGRESS_X, PL_PROGRESS_Y, PL_PROGRESS_W, PL_PROGRESS_H, PL_PROGRESS_H / 2, COLOR_TRACK_BG);
    updateProgressBar(true);

    // English implementation note
    tft.fillRoundRect(PL_PREV_X, PL_BTN_Y, PL_PREV_W, PL_BTN_H, 10, COLOR_PANEL);
    tft.drawRoundRect(PL_PREV_X, PL_BTN_Y, PL_PREV_W, PL_BTN_H, 10, COLOR_PANEL_BORDER);
    drawPrevIcon(PL_PREV_X + PL_PREV_W / 2, PL_BTN_Y + PL_BTN_H / 2, 16, COLOR_TEXT_PRIMARY);

    tft.fillRoundRect(PL_NEXT_X, PL_BTN_Y, PL_NEXT_W, PL_BTN_H, 10, COLOR_PANEL);
    tft.drawRoundRect(PL_NEXT_X, PL_BTN_Y, PL_NEXT_W, PL_BTN_H, 10, COLOR_PANEL_BORDER);
    drawNextIcon(PL_NEXT_X + PL_NEXT_W / 2, PL_BTN_Y + PL_BTN_H / 2, 16, COLOR_TEXT_PRIMARY);

    updatePlayPauseButton();

    // English implementation note
    tft.fillRoundRect(PL_VOLDOWN_X, PL_VOL_Y, PL_VOLDOWN_W, PL_VOL_H, 10, COLOR_PANEL);
    tft.drawRoundRect(PL_VOLDOWN_X, PL_VOL_Y, PL_VOLDOWN_W, PL_VOL_H, 10, COLOR_PANEL_BORDER);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setTextDatum(MC_DATUM); 
    tft.drawCentreString("-", PL_VOLDOWN_X + PL_VOLDOWN_W / 2, PL_VOL_Y + PL_VOL_H / 2 - 7, 4);

    tft.fillRoundRect(PL_VOLUP_X, PL_VOL_Y, PL_VOLUP_W, PL_VOL_H, 10, COLOR_PANEL);
    tft.drawRoundRect(PL_VOLUP_X, PL_VOL_Y, PL_VOLUP_W, PL_VOL_H, 10, COLOR_PANEL_BORDER);
    tft.drawCentreString("+", PL_VOLUP_X + PL_VOLUP_W / 2, PL_VOL_Y + PL_VOL_H / 2 - 7, 4);

    updateVolumeUI();
    drawBottomNavigation(0);
}

void updateLikeButton() {
    if (currentState != STATE_PLAYER) return;
    tft.fillRect(PL_HEART_CX - PL_HEART_R - 2, PL_HEART_CY - PL_HEART_R - 2, (PL_HEART_R + 2) * 2, (PL_HEART_R + 2) * 2, COLOR_BG);
    bool liked = (total_tracks > 0) ? isLiked[current_track_index] : false;
    drawHeartIcon(PL_HEART_CX, PL_HEART_CY, PL_HEART_R, liked ? COLOR_HEART_ACTIVE : COLOR_HEART_INACTIVE);
}

void updatePlayPauseButton() {
    if (currentState != STATE_PLAYER) return;
    tft.fillRoundRect(PL_PLAY_X, PL_BTN_Y, PL_PLAY_W, PL_BTN_H, 10, is_playing ? COLOR_ACCENT_DIM : COLOR_ACCENT);
    tft.drawRoundRect(PL_PLAY_X, PL_BTN_Y, PL_PLAY_W, PL_BTN_H, 10, COLOR_PANEL_BORDER);
    if (is_playing) {
        drawPauseIcon(PL_PLAY_X + PL_PLAY_W / 2, PL_BTN_Y + PL_BTN_H / 2, 16, COLOR_BG);
    } else {
        drawPlayIcon(PL_PLAY_X + PL_PLAY_W / 2, PL_BTN_Y + PL_BTN_H / 2, 16, COLOR_BG);
    }
}

static int last_drawn_progress_pct = -1;
void updateProgressBar(bool forceRedraw) {
    if (currentState != STATE_PLAYER) return;
    int pct = 0;
    size_t size = g_file_size;
    size_t readBytes = g_file_bytes_read;
    if (size > 0) {
        pct = (int)((uint64_t)readBytes * 100 / size);
        if (pct > 100) pct = 100;
    }

    if (!forceRedraw && pct == last_drawn_progress_pct) return;
    last_drawn_progress_pct = pct;

    int fillWidth = (PL_PROGRESS_W * pct) / 100;
    if (fillWidth > 0) {
        tft.fillRoundRect(PL_PROGRESS_X, PL_PROGRESS_Y, fillWidth, PL_PROGRESS_H, PL_PROGRESS_H / 2, COLOR_ACCENT);
    }
    if (fillWidth < PL_PROGRESS_W) {
        tft.fillRect(PL_PROGRESS_X + fillWidth, PL_PROGRESS_Y, PL_PROGRESS_W - fillWidth, PL_PROGRESS_H, COLOR_TRACK_BG);
    }
}

void updateVolumeUI() {
    if (currentState != STATE_PLAYER) return;
    tft.fillRect(PL_VOLTEXT_X, PL_VOL_Y, PL_VOLTEXT_W, PL_VOL_H, COLOR_BG);
    
    int volPercent = (bt_volume * 100) / 127;
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    tft.setTextDatum(MC_DATUM); 
    tft.drawCentreString(String(volPercent) + "%", PL_VOLTEXT_X + PL_VOLTEXT_W / 2, PL_VOL_Y + PL_VOL_H / 2 - 6, 2);
}

void drawTrackListScreen(bool onlyLikes) {
    tft.fillScreen(COLOR_BG);
    drawHeader(onlyLikes ? "FAVORITES" : "ALL TRACKS");

    int matchingTracks[MAX_TRACKS];
    int matchCount = 0;
    for (int i = 0; i < total_tracks; i++) {
        if (!onlyLikes || isLiked[i]) {
            matchingTracks[matchCount++] = i;
        }
    }

    int page = onlyLikes ? likesList_currentPage : trackList_currentPage;
    int maxPages = (matchCount + LIST_ITEMS_PER_PAGE - 1) / LIST_ITEMS_PER_PAGE;
    if (maxPages == 0) maxPages = 1;

    int startIdx = page * LIST_ITEMS_PER_PAGE;
    int endIdx = min(startIdx + LIST_ITEMS_PER_PAGE, matchCount);

    for (int i = 0; i < LIST_ITEMS_PER_PAGE; i++) {
        int listY = LIST_ITEM_Y_START + i * (LIST_ITEM_H + LIST_ITEM_GAP);
        int itemIdx = startIdx + i;

        if (itemIdx < endIdx) {
            int originalTrackIdx = matchingTracks[itemIdx];
            bool isCurrent = (originalTrackIdx == current_track_index);

            tft.fillRoundRect(10, listY, SCREEN_W - 20, LIST_ITEM_H, 8, isCurrent ? COLOR_PANEL_BORDER : COLOR_PANEL);
            tft.drawRoundRect(10, listY, SCREEN_W - 20, LIST_ITEM_H, 8, isCurrent ? COLOR_ACCENT : COLOR_PANEL_BORDER);

            String name = trackDisplayName(playlist[originalTrackIdx]);
            name = truncateToWidth(name, SCREEN_W - 55, 2);
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setTextDatum(TL_DATUM); 
            tft.drawString(name, 20, listY + 10, 2);
        }
    }

    if (matchCount == 0) {
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.drawCentreString(onlyLikes ? "No liked tracks" : "No MP3 files", SCREEN_W / 2, 140, 2);
    }

    tft.fillRoundRect(20, 230, 50, 30, 6, COLOR_PANEL);
    tft.drawRoundRect(20, 230, 50, 30, 6, COLOR_PANEL_BORDER);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setTextDatum(MC_DATUM); 
    drawPrevIcon(45, 245, 14, COLOR_TEXT_SECONDARY);

    tft.setTextDatum(MC_DATUM);
    tft.drawCentreString(String(page + 1) + "/" + String(maxPages), 120, 245, 2);

    tft.fillRoundRect(170, 230, 50, 30, 6, COLOR_PANEL);
    tft.drawRoundRect(170, 230, 50, 30, 6, COLOR_PANEL_BORDER);
    drawNextIcon(195, 245, 14, COLOR_TEXT_SECONDARY);
    drawBottomNavigation(onlyLikes ? 2 : 1);
}

void drawBTManagerScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("BLUETOOTH");
    tft.drawCircle(120, 92, 43, COLOR_PANEL_BORDER);
    tft.drawCircle(120, 92, 37, COLOR_ACCENT_DIM);
    drawBluetoothLogo(120, 92, 42, bt_connected ? COLOR_BT_CONNECTED : COLOR_BT_DISCONNECTED);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.drawCentreString(bt_connected ? "Connected" : "Disconnected", SCREEN_W / 2, 145, 2);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.drawCentreString(savedBTDevice, SCREEN_W / 2, 164, 2);
    tft.fillRoundRect(20, 182, SCREEN_W - 40, 34, 9, COLOR_PANEL);
    tft.drawRoundRect(20, 182, SCREEN_W - 40, 34, 9, COLOR_ACCENT_DIM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.drawCentreString(bt_connected ? "DISCONNECT" : "CONNECT", SCREEN_W / 2, 191, 2);
    tft.fillRoundRect(20, 224, SCREEN_W - 40, 34, 9, COLOR_PANEL);
    tft.drawRoundRect(20, 224, SCREEN_W - 40, 34, 9, COLOR_PANEL_BORDER);
    tft.drawCentreString("SCAN DEVICES", SCREEN_W / 2, 233, 2);
    drawBottomNavigation(4);
    return;
#if 0
    tft.fillRoundRect(10, 45, SCREEN_W - 20, 75, 10, COLOR_PANEL);
    tft.drawRoundRect(10, 45, SCREEN_W - 20, 75, 10, COLOR_PANEL_BORDER);

    tft.setTextDatum(TL_DATUM); 
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.drawString("Target Device:", 20, 52, 2);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.drawString(savedBTDevice, 20, 72, 4);

    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.drawString("Status:", 20, 98, 2);
    tft.setTextColor(bt_connected ? COLOR_ACCENT : COLOR_HEART_INACTIVE);
    tft.drawString(bt_connected ? "CONNECTED" : "DISCONNECTED", 80, 98, 2);

    // English implementation note
    tft.fillRoundRect(20, 130, SCREEN_W - 40, 40, 8, bt_connected ? COLOR_HEART_ACTIVE : COLOR_ACCENT);
    tft.drawRoundRect(20, 130, SCREEN_W - 40, 40, 8, COLOR_PANEL_BORDER);
    tft.setTextColor(COLOR_BG);
    tft.setTextDatum(MC_DATUM); 
    tft.drawCentreString(bt_connected ? "DISCONNECT" : "CONNECT NOW", SCREEN_W / 2, 142, 4);

    // English implementation note
    tft.fillRoundRect(20, 185, SCREEN_W - 40, 40, 8, COLOR_PANEL);
    tft.drawRoundRect(20, 185, SCREEN_W - 40, 40, 8, COLOR_PANEL_BORDER);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.drawCentreString("SWITCH DEVICE", SCREEN_W / 2, 197, 4);
#endif
}

void drawBluetoothScanScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader("BLUETOOTH");
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.drawCentreString(btScanActive ? "Scanning..." : "Select device", SCREEN_W / 2, 48, 2);
    for (uint8_t i = 0; i < btScanCount; i++) {
        int y = 68 + i * 31;
        tft.fillRoundRect(10, y, SCREEN_W - 20, 26, 6, COLOR_PANEL);
        tft.drawRoundRect(10, y, SCREEN_W - 20, 26, 6, COLOR_PANEL_BORDER);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.drawString(btScanNames[i], 20, y + 7, 2);
    }
    drawBottomNavigation(4);
}

void drawCurrentStateUI() {
    last_drawn_progress_pct = -1; 
    switch (currentState) {
        case STATE_MENU: drawMainMenu(); break;
        case STATE_PLAYER: drawPlayerScreen(); break;
        case STATE_TRACK_LIST: drawTrackListScreen(false); break;
        case STATE_LIKES_LIST: drawTrackListScreen(true); break;
        case STATE_BT_MANAGER: drawBTManagerScreen(); break; 
        case STATE_SETTINGS: drawSettingsScreen(); break;
        case STATE_BT_SCAN: drawBluetoothScanScreen(); break;
        case STATE_SCREEN_SETTINGS: drawScreenSettingsScreen(); break;
        case STATE_PLAYBACK_SETTINGS: drawPlaybackSettingsScreen(); break;
        case STATE_INFO: drawInfoScreen(); break;
        case STATE_RESET_CONFIRM: drawResetConfirmScreen(); break;
        case STATE_THEME_SETTINGS: drawThemeSettingsScreen(); break;
    }
}

// English implementation note
String truncateToWidth(const String& text, int maxWidth, uint8_t font) {
    if (tft.textWidth(text, font) <= maxWidth) return text;
    String result = text;
    while (result.length() > 1 && tft.textWidth(result + "...", font) > maxWidth) {
        result.remove(result.length() - 1);
    }
    return result + "...";
}

int32_t get_sound_data(Frame *data, int32_t frame_count) {
    if (pcm_frame_queue == NULL) return 0;
    if (!is_playing) {
        // English implementation note
        // English implementation note
        memset(data, 0, frame_count * sizeof(Frame));
        return frame_count;
    }
    int32_t fetched = 0;
    while (fetched < frame_count) {
        if (xQueueReceive(pcm_frame_queue, &data[fetched], 0) == pdTRUE) {
            fetched++;
        } else break;
    }
    // Fill a temporarily empty callback frame with silence so A2DP never sees a short frame.
    if (fetched < frame_count) {
        memset(&data[fetched], 0, (frame_count - fetched) * sizeof(Frame));
    }
    return frame_count;
}

// =====================================================================================
// English implementation note
// =====================================================================================
void mp3DecoderTask(void *pvParameters) {
    mp3dec_init(&mp3d);
    size_t read_offset = 0;
    size_t read_len = 0;

    // English implementation note
    uint32_t last_hz = 0; 
    uint32_t step = 65536; // English implementation note

    while (true) {
        if (decoder_reset_requested) {
            read_offset = 0;
            read_len = 0;
            mp3dec_init(&mp3d);
            decoder_reset_requested = false;
        }
        if (!is_playing) { vTaskDelay(100 / portTICK_PERIOD_MS); continue; }

        if (uxQueueMessagesWaiting(pcm_frame_queue) > (PCM_QUEUE_SIZE - 512)) {
            vTaskDelay(pdMS_TO_TICKS(15));
            continue;
        }

        if (read_len - read_offset < 1024 && audioFile && audioFile.available()) {
            if (read_offset > 0) {
                memmove(mp3_read_buf, mp3_read_buf + read_offset, read_len - read_offset);
                read_len -= read_offset;
                read_offset = 0;
            }
            size_t bytesRead = audioFile.read(mp3_read_buf + read_len, READ_BUF_SIZE - read_len);
            read_len += bytesRead;
            g_file_bytes_read += bytesRead;
        }

        int samples = mp3dec_decode_frame(&mp3d, mp3_read_buf + read_offset, read_len - read_offset, pcm_output_buffer, &info);

        if (samples > 0) {
            read_offset += info.frame_bytes;

            // English implementation note
            if (info.hz != last_hz && info.hz > 0) {
                last_hz = info.hz;
                step = ((uint32_t)info.hz << 16) / 44100;
            }

            // English implementation note
            if (last_hz != 44100) {
                uint32_t acc = 0;
                while ((acc >> 16) < (uint32_t)(samples - 1)) {
                    uint32_t idx = acc >> 16;
                    uint32_t frac = acc & 0xFFFF; // English implementation note

                    Frame f;
                    if (info.channels == 2) {
                        // English implementation note
                        int32_t s1_l = pcm_output_buffer[idx * 2];
                        int32_t s2_l = pcm_output_buffer[(idx + 1) * 2];
                        f.channel1 = s1_l + (((s2_l - s1_l) * (int32_t)frac) >> 16);

                        // English implementation note
                        int32_t s1_r = pcm_output_buffer[idx * 2 + 1];
                        int32_t s2_r = pcm_output_buffer[(idx + 1) * 2 + 1];
                        f.channel2 = s1_r + (((s2_r - s1_r) * (int32_t)frac) >> 16);
                    } else {
                        // English implementation note
                        int32_t s1 = pcm_output_buffer[idx];
                        int32_t s2 = pcm_output_buffer[idx + 1];
                        f.channel1 = f.channel2 = s1 + (((s2 - s1) * (int32_t)frac) >> 16);
                    }
                    xQueueSend(pcm_frame_queue, &f, portMAX_DELAY);
                    acc += step;
                }
            } else {
                // English implementation note
                for (int i = 0; i < samples; i++) {
                    Frame f;
                    if (info.channels == 2) {
                        f.channel1 = pcm_output_buffer[i * 2];
                        f.channel2 = pcm_output_buffer[i * 2 + 1];
                    } else {
                        f.channel1 = f.channel2 = pcm_output_buffer[i];
                    }
                    xQueueSend(pcm_frame_queue, &f, portMAX_DELAY);
                }
            }
        } else {
            if (info.frame_bytes > 0) {
                read_offset += info.frame_bytes;
            }
            else if (!audioFile || !audioFile.available()) {
                need_next_track = true;
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            else {
                read_offset++;
            }
        }
    }
}

// =====================================================================================
// English implementation note
// =====================================================================================
bool pointInRect(int x, int y, int rx, int ry, int rw, int rh) {
    return (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh);
}

void processGlobalTouch(int x, int y) {
    // English implementation note
    if (y < HEADER_H && x >= SCREEN_W - 42) {
        currentState = STATE_BT_MANAGER;
        drawCurrentStateUI();
        return;
    }
    if (currentState != STATE_MENU) {
        if (pointInRect(x, y, 8, 5, 55, 24)) {
            if (currentState == STATE_SCREEN_SETTINGS || currentState == STATE_PLAYBACK_SETTINGS || currentState == STATE_INFO || currentState == STATE_RESET_CONFIRM || currentState == STATE_THEME_SETTINGS) {
                currentState = STATE_SETTINGS;
            } else if (currentState == STATE_BT_SCAN) {
                currentState = STATE_BT_MANAGER;
            } else {
                currentState = STATE_MENU;
            }
            drawCurrentStateUI();
            return;
        }
    }

    if (y >= NAV_Y) {
        if (x < 48) {
            currentState = STATE_MENU;
        } else if (x < 96) {
            trackList_currentPage = 0;
            currentState = STATE_TRACK_LIST;
        } else if (x < 144) {
            likesList_currentPage = 0;
            currentState = STATE_LIKES_LIST;
        } else if (x >= 192) {
            currentState = STATE_SETTINGS;
        } else {
            return; // English implementation note
        }
        drawCurrentStateUI();
        return;
    }

    switch (currentState) {
        case STATE_MENU: {
            if (pointInRect(x, y, MENU_BTN_X_LEFT, MENU_BTN_Y[0], MENU_BTN_W, MENU_BTN_H)) {
                currentState = STATE_PLAYER;
                drawCurrentStateUI();
            }
            else if (pointInRect(x, y, MENU_BTN_X_RIGHT, MENU_BTN_Y[0], MENU_BTN_W, MENU_BTN_H)) {
                trackList_currentPage = 0;
                currentState = STATE_TRACK_LIST;
                drawCurrentStateUI();
            }
            else if (pointInRect(x, y, MENU_BTN_X_LEFT, MENU_BTN_Y[1], MENU_BTN_W, MENU_BTN_H)) {
                likesList_currentPage = 0;
                currentState = STATE_LIKES_LIST;
                drawCurrentStateUI();
            }
            else if (pointInRect(x, y, MENU_BTN_X_RIGHT, MENU_BTN_Y[1], MENU_BTN_W, MENU_BTN_H)) {
                currentState = STATE_SETTINGS;
                drawCurrentStateUI();
            }
            else if (pointInRect(x, y, 10, 208, SCREEN_W - 20, 62)) {
                currentState = STATE_PLAYER;
                drawCurrentStateUI();
            }
            break;
        }

        case STATE_PLAYER: {
            if (pointInRect(x, y, PL_PREV_X, PL_BTN_Y, PL_PREV_W, PL_BTN_H)) {
                switchTrack(-1);
            }
            else if (pointInRect(x, y, PL_PLAY_X, PL_BTN_Y, PL_PLAY_W, PL_BTN_H)) {
                bool wasPlaying = is_playing;
                if (wasPlaying) {
                    is_playing = false;
                    play_requested = false;
                } else {
                    play_requested = true;
                    if (bt_audio_started) {
                        decoder_reset_requested = true;
                        xQueueReset(pcm_frame_queue);
                        is_playing = true;
                    }
                }
                updatePlayPauseButton();
            }
            else if (pointInRect(x, y, PL_NEXT_X, PL_BTN_Y, PL_NEXT_W, PL_BTN_H)) {
                switchTrack(1);
            }
            else if (pointInRect(x, y, PL_HEART_CX - PL_HEART_R - 6, PL_HEART_CY - PL_HEART_R - 6, (PL_HEART_R + 6) * 2, (PL_HEART_R + 6) * 2)) {
                toggleLike();
            }
            else if (pointInRect(x, y, PL_PROGRESS_X - 6, PL_PROGRESS_Y - 10, PL_PROGRESS_W + 12, PL_PROGRESS_H + 20)) {
                float ratio = (float)(x - PL_PROGRESS_X) / PL_PROGRESS_W;
                if (ratio < 0.0f) ratio = 0.0f;
                if (ratio > 1.0f) ratio = 1.0f;
                seekToRatio(ratio);
            }
            else if (pointInRect(x, y, PL_VOLDOWN_X, PL_VOL_Y, PL_VOLDOWN_W, PL_VOL_H)) {
                if (bt_volume >= 8) bt_volume -= 8; else bt_volume = 0;
                a2dp_source.set_volume(bt_volume);
                
                preferences.begin("vol_pref", false);
                preferences.putUChar("volume", bt_volume);
                preferences.end();
                updateVolumeUI();
            }
            else if (pointInRect(x, y, PL_VOLUP_X, PL_VOL_Y, PL_VOLUP_W, PL_VOL_H)) {
                if (bt_volume <= 119) bt_volume += 8; else bt_volume = 127;
                a2dp_source.set_volume(bt_volume);
                
                preferences.begin("vol_pref", false);
                preferences.putUChar("volume", bt_volume);
                preferences.end();
                updateVolumeUI();
            }
            break;
        }

        case STATE_TRACK_LIST:
        case STATE_LIKES_LIST: {
            bool onlyLikes = (currentState == STATE_LIKES_LIST);
            int matchingTracks[MAX_TRACKS];
            int matchCount = 0;
            for (int i = 0; i < total_tracks; i++) {
                if (!onlyLikes || isLiked[i]) matchingTracks[matchCount++] = i;
            }

            int page = onlyLikes ? likesList_currentPage : trackList_currentPage;
            int maxPages = (matchCount + LIST_ITEMS_PER_PAGE - 1) / LIST_ITEMS_PER_PAGE;
            if (maxPages == 0) maxPages = 1;

            for (int i = 0; i < LIST_ITEMS_PER_PAGE; i++) {
                int listY = LIST_ITEM_Y_START + i * (LIST_ITEM_H + LIST_ITEM_GAP);
                if (pointInRect(x, y, 10, listY, SCREEN_W - 20, LIST_ITEM_H)) {
                    int itemIdx = page * LIST_ITEMS_PER_PAGE + i;
                    if (itemIdx < matchCount) {
                        current_track_index = matchingTracks[itemIdx];
                        switchTrack(0);
                        drawCurrentStateUI();
                        return;
                    }
                }
            }

            if (pointInRect(x, y, 20, 230, 50, 30)) {
                if (page > 0) {
                    if (onlyLikes) likesList_currentPage--; else trackList_currentPage--;
                    drawCurrentStateUI();
                }
            }
            else if (pointInRect(x, y, 170, 230, 50, 30)) {
                if (page < maxPages - 1) {
                    if (onlyLikes) likesList_currentPage++; else trackList_currentPage++;
                    drawCurrentStateUI();
                }
            }
            break;
        }

        case STATE_SETTINGS: {
            if (pointInRect(x, y, 10, 48, SCREEN_W - 20, 28)) currentState = STATE_BT_MANAGER;
            else if (pointInRect(x, y, 10, 82, SCREEN_W - 20, 28)) currentState = STATE_SCREEN_SETTINGS;
            else if (pointInRect(x, y, 10, 116, SCREEN_W - 20, 28)) currentState = STATE_PLAYBACK_SETTINGS;
            else if (pointInRect(x, y, 10, 150, SCREEN_W - 20, 28)) currentState = STATE_THEME_SETTINGS;
            else if (pointInRect(x, y, 10, 184, SCREEN_W - 20, 28)) currentState = STATE_INFO;
            else if (pointInRect(x, y, 10, 218, SCREEN_W - 20, 28)) currentState = STATE_RESET_CONFIRM;
            else return;
            drawCurrentStateUI();
            break;
        }

        case STATE_PLAYBACK_SETTINGS: {
            if (pointInRect(x, y, 10, 55, SCREEN_W - 20, 38)) {
                autoPlayEnabled = !autoPlayEnabled;
                preferences.begin("play_pref", false);
                preferences.putBool("autoplay", autoPlayEnabled);
                preferences.end();
                drawCurrentStateUI();
            }
            break;
        }

        case STATE_SCREEN_SETTINGS: {
            if (pointInRect(x, y, 10, 68, SCREEN_W - 20, 30)) {
                screenBrightness = constrain(map(x, 18, 222, 0, 255), 20, 255);
                applyScreenBrightness();
                preferences.begin("screen_pref", false);
                preferences.putUChar("brightness", screenBrightness);
                preferences.end();
                drawCurrentStateUI();
            } else if (pointInRect(x, y, 12, 145, 51, 30) || pointInRect(x, y, 69, 145, 51, 30) || pointInRect(x, y, 126, 145, 51, 30) || pointInRect(x, y, 183, 145, 51, 30)) {
                uint8_t slot = (x - 12) / 57;
                const uint8_t values[] = {1, 5, 10, 0};
                screenTimeoutMinutes = values[constrain((int)slot, 0, 3)];
                preferences.begin("screen_pref", false);
                preferences.putUChar("timeout", screenTimeoutMinutes);
                preferences.end();
                drawCurrentStateUI();
            }
            break;
        }

        case STATE_THEME_SETTINGS: {
    for (uint8_t i = 0; i < 5; i++) {
                if (pointInRect(x, y, 18, 48 + i * 44, SCREEN_W - 36, 36)) {
                    themeIndex = i;
                    applyTheme(themeIndex);
                    preferences.begin("screen_pref", false);
                    preferences.putUChar("theme", themeIndex);
                    preferences.end();
                    drawCurrentStateUI();
                    break;
                }
            }
            break;
        }

        case STATE_RESET_CONFIRM: {
            if (pointInRect(x, y, 18, 178, 96, 34)) {
                currentState = STATE_SETTINGS;
                drawCurrentStateUI();
            } else if (pointInRect(x, y, 126, 178, 96, 34)) {
                preferences.begin("bt_pref", false); preferences.clear(); preferences.end();
                preferences.begin("vol_pref", false); preferences.clear(); preferences.end();
                preferences.begin("play_pref", false); preferences.clear(); preferences.end();
                preferences.begin("screen_pref", false); preferences.clear(); preferences.end();
                autoPlayEnabled = false;
                bt_volume = 70;
                screenBrightness = 220;
                screenTimeoutMinutes = 0;
                themeIndex = 0;
                applyTheme(themeIndex);
                applyScreenBrightness();
                savedBTDeviceIndex = 0;
                savedBTDevice = BT_DEVICES[0];
                hasSavedBTAddress = false;
                currentState = STATE_SETTINGS;
                drawCurrentStateUI();
            }
            break;
        }

        case STATE_BT_MANAGER: {
            if (pointInRect(x, y, 20, 182, SCREEN_W - 40, 34)) {
                if (bt_connected) {
                    Serial.println("[BT] Отключение...");
                    a2dp_source.disconnect();
                } else {
                    Serial.println("[BT] Безопасное переподключение...");
                    a2dp_source.reconnect();
                }
                delay(100);
                bt_connected = a2dp_source.is_connected();
                drawCurrentStateUI();
            }
            else if (pointInRect(x, y, 20, 224, SCREEN_W - 40, 34)) {
                startBluetoothScan();
                
                Serial.println("[BT] Смена целевого устройства...");
                drawCurrentStateUI();
            }
            break;
        }

        case STATE_BT_SCAN: {
            for (uint8_t i = 0; i < btScanCount; i++) {
                int rowY = 68 + i * 31;
                if (pointInRect(x, y, 10, rowY, SCREEN_W - 20, 26)) {
                    selectBluetoothScanResult(i);
                    return;
                }
            }
            break;
        }
    }
}

#if 0
// =====================================================================================
// English implementation note
// =====================================================================================
void setupWebUploader() {
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(WIFI_AP_NAME, WIFI_AP_PASSWORD) || WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
        Serial.println("[WiFi] AP start failed");
        webUploaderReady = false;
        return;
    }

    webServer.on("/", HTTP_GET, []() {
        String html = "<!doctype html><meta name='viewport' content='width=device-width'>";
        html += "<h2>ESP32 Player</h2><p>Wi-Fi: ESP32-Player &nbsp; IP: " + WiFi.softAPIP().toString() + "</p>";
        html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
        html += "<input type='file' name='file' accept='.mp3,audio/mpeg' required><button>Upload MP3</button></form><hr><ul>";
        File dir = SD.open("/tracks");
        if (dir) {
            while (true) {
                File entry = dir.openNextFile();
                if (!entry) break;
                if (!entry.isDirectory()) {
                    String name = String(entry.name());
                    if (name.endsWith(".mp3") || name.endsWith(".MP3")) {
                        html += "<li>" + name + " (" + String(entry.size() / 1024) + " KB)";
                        html += " <a href='/delete?name=" + name + "'>delete</a></li>";
                    }
                }
                entry.close();
            }
            dir.close();
        }
        html += "</ul>";
        webServer.send(200, "text/html; charset=utf-8", html);
    });

    webServer.on("/delete", HTTP_GET, []() {
        if (!webServer.hasArg("name")) { webServer.send(400, "text/plain", "name required"); return; }
        String name = webServer.arg("name");
        if (name.indexOf("..") >= 0 || name.indexOf('/') >= 0 || name.indexOf('\\') >= 0) {
            webServer.send(400, "text/plain", "invalid name"); return;
        }
        String path = "/tracks/" + name;
        is_playing = false;
        vTaskDelay(pdMS_TO_TICKS(20));
        if (audioFileMutex != NULL) xSemaphoreTake(audioFileMutex, portMAX_DELAY);
        if (audioFile) audioFile.close();
        audio_generation++;
        if (path.endsWith(".mp3") || path.endsWith(".MP3")) SD.remove(path.c_str());
        loadPlaylist();
        loadLikes();
        if (audioFileMutex != NULL) xSemaphoreGive(audioFileMutex);
        if (current_track_index >= total_tracks) current_track_index = 0;
        webServer.sendHeader("Location", "/");
        webServer.send(303);
    });

    webServer.on("/upload", HTTP_POST, []() {
        if (audioFileMutex != NULL) xSemaphoreTake(audioFileMutex, portMAX_DELAY);
        if (uploadFile) uploadFile.close();
        if (audioFileMutex != NULL) xSemaphoreGive(audioFileMutex);
        uploadInProgress = false;
        bool wasPlaying = is_playing;
        is_playing = false;
        vTaskDelay(pdMS_TO_TICKS(20));
        if (audioFileMutex != NULL) xSemaphoreTake(audioFileMutex, portMAX_DELAY);
        loadPlaylist();
        loadLikes();
        if (audioFileMutex != NULL) xSemaphoreGive(audioFileMutex);
        if (wasPlaying && total_tracks > 0) is_playing = true;
        webServer.sendHeader("Location", "/");
        webServer.send(303);
    }, []() {
        HTTPUpload& upload = webServer.upload();
        if (upload.status == UPLOAD_FILE_START) {
            String name = upload.filename;
            int slash = name.lastIndexOf('/');
            if (slash >= 0) name = name.substring(slash + 1);
            if (name.indexOf("..") >= 0 || (!name.endsWith(".mp3") && !name.endsWith(".MP3"))) return;
            if (audioFileMutex != NULL) xSemaphoreTake(audioFileMutex, portMAX_DELAY);
            String uploadPath = String("/tracks/") + name;
            SD.remove(uploadPath.c_str());
            uploadFile = SD.open(uploadPath.c_str(), FILE_WRITE);
            uploadInProgress = (bool)uploadFile;
            if (audioFileMutex != NULL) xSemaphoreGive(audioFileMutex);
        } else if (upload.status == UPLOAD_FILE_WRITE && uploadInProgress) {
            if (audioFileMutex != NULL) xSemaphoreTake(audioFileMutex, portMAX_DELAY);
            uploadFile.write(upload.buf, upload.currentSize);
            if (audioFileMutex != NULL) xSemaphoreGive(audioFileMutex);
        } else if (upload.status == UPLOAD_FILE_END && uploadInProgress) {
            if (audioFileMutex != NULL) xSemaphoreTake(audioFileMutex, portMAX_DELAY);
            uploadFile.close();
            if (audioFileMutex != NULL) xSemaphoreGive(audioFileMutex);
        }
    });
    webServer.begin();
    webUploaderReady = true;
    Serial.print("[WiFi] Upload page: http://");
    Serial.println(WiFi.softAPIP());
}

void handleWebUploader() {
    if (webUploaderReady) webServer.handleClient();
}
#endif

// =====================================================================================
//  SETUP / LOOP
// =====================================================================================
void setup() {
    setCpuFrequencyMhz(240);
    Serial.begin(115200);

    tft.init();
    ledcAttach(TFT_BL_PIN, 5000, 8);
    ledcWrite(TFT_BL_PIN, screenBrightness);
    tft.setRotation(0); // English implementation note
    initColorTheme();
    
    tft.fillScreen(COLOR_BG);
    drawHeader("SYSTEM START", false);
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
    tft.drawCentreString("TF Initialization...", SCREEN_W / 2, 110, 2);

    touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreenSPI);
    touchscreen.setRotation(0);

    delay(200);

    // English implementation note
    preferences.begin("bt_pref", false);
    savedBTDeviceIndex = preferences.getUChar("device_idx", 255);
    String storedBTName = preferences.getString("bt_name", BT_DEVICES[0]);
    preferences.end();

    // English implementation note
    if (savedBTDeviceIndex >= BT_DEVICE_COUNT) {
        savedBTDeviceIndex = 0;
        for (uint8_t i = 0; i < BT_DEVICE_COUNT; i++) {
            if (storedBTName == BT_DEVICES[i]) {
                savedBTDeviceIndex = i;
                break;
            }
        }
    }
    savedBTDevice = BT_DEVICES[savedBTDeviceIndex];
    preferences.begin("bt_pref", true);
    if (preferences.getBytesLength("bt_addr") == ESP_BD_ADDR_LEN) {
        preferences.getBytes("bt_addr", savedBTAddress, ESP_BD_ADDR_LEN);
        hasSavedBTAddress = true;
        if (storedBTName.length() > 0) savedBTDevice = storedBTName;
    }
    preferences.end();

    // English implementation note
    preferences.begin("vol_pref", false);
    bt_volume = preferences.getUChar("volume", 70); 
    preferences.end();

    preferences.begin("play_pref", true);
    autoPlayEnabled = preferences.getBool("autoplay", false);
    preferences.end();

    preferences.begin("screen_pref", true);
    screenBrightness = preferences.getUChar("brightness", 220);
    screenTimeoutMinutes = preferences.getUChar("timeout", 0);
    themeIndex = preferences.getUChar("theme", 0);
    preferences.end();
    applyTheme(themeIndex);
    applyScreenBrightness();

    pcm_frame_queue = xQueueCreate(PCM_QUEUE_SIZE, sizeof(Frame));

    bool sdReady = SD.begin(SD_CS);
    if (!sdReady) {
        Serial.println("[SD] First init failed, retrying...");
        delay(250);
        sdReady = SD.begin(SD_CS);
    }
    if (!sdReady) {
        tft.fillScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE);
        tft.drawCentreString("SD Error!", SCREEN_W / 2, 110, 4);
        while (true) delay(1000);
    }

    loadPlaylist();
    if (total_tracks > 0) {
        loadLikes();
        audioFile = SD.open(playlist[0].c_str());
        if (audioFile) {
            g_file_size = audioFile.size();
            g_file_bytes_read = 0;
            is_playing = false;
        }
    }

    // English implementation note
    if (hasSavedBTAddress) {
        a2dp_source.set_auto_reconnect(savedBTAddress, 1);
    }
    a2dp_source.set_on_connection_state_changed(onBluetoothConnectionState);
    a2dp_source.set_on_audio_state_changed(onBluetoothAudioState);
    a2dp_source.start(savedBTDevice.c_str(), get_sound_data);
    a2dp_source.set_volume(bt_volume);
    if (hasSavedBTAddress) {
        Serial.println("[BT] Auto-connect requested for saved MAC");
    }

    // English implementation note
    BaseType_t decoderResult = xTaskCreatePinnedToCore(
        mp3DecoderTask, "MP3Dec", 20480, NULL, 3, &mp3DecoderTaskHandle, 1);
    Serial.printf("[Audio] decoder task: %s\n", decoderResult == pdPASS ? "started" : "FAILED");


    currentState = STATE_MENU;
    drawCurrentStateUI();
    lastScreenActivity = millis();
}

void loop() {
    static unsigned long last_touch_time = 0;
    static unsigned long last_progress_update = 0;
    static unsigned long last_bt_status_check = 0;
    static unsigned long last_audio_debug = 0;
    static uint8_t last_scan_count = 0;


    // English implementation note
    if (need_next_track) {
        need_next_track = false;
        switchTrack(1);
    }

    if (btScanActive && millis() - btScanStartedAt > 10000) {
        btScanActive = false;
        a2dp_source.cancel_discovery();
        a2dp_source.set_ssid_callback(nullptr);
        if (!screenLocked) drawCurrentStateUI();
    }
    if (currentState == STATE_BT_SCAN && btScanCount != last_scan_count) {
        last_scan_count = btScanCount;
        if (!screenLocked) drawCurrentStateUI();
    }

    // English implementation note
    unsigned long now = millis();
    if (!screenLocked && screenTimeoutMinutes > 0 && now - lastScreenActivity >= (unsigned long)screenTimeoutMinutes * 60000UL) {
        screenLocked = true;
        drawScreenLocked();
    }
    if (now - last_audio_debug > 2000) {
        last_audio_debug = now;
        Serial.printf("[Audio] playing=%d queue=%d file=%u/%u bt=%d\n",
                      is_playing ? 1 : 0,
                      pcm_frame_queue ? uxQueueMessagesWaiting(pcm_frame_queue) : 0,
                      (unsigned int)g_file_bytes_read,
                      (unsigned int)g_file_size,
                      bt_connected ? 1 : 0);
    }
    if (now - last_bt_status_check > 1500) {
        bool currentStatus = a2dp_source.is_connected();
        if (currentStatus != bt_connected) {
            bt_connected = currentStatus;
            if (currentStatus && autoPlayEnabled) {
                play_requested = true;
            }
            if (!currentStatus) {
                bt_audio_started = false;
                is_playing = false;
            }
            if (currentStatus && bt_audio_started && play_requested && !is_playing) {
                decoder_reset_requested = true;
                xQueueReset(pcm_frame_queue);
                is_playing = true;
            }
            if (!screenLocked) drawCurrentStateUI(); 
        }
        last_bt_status_check = now;
    }

    // English implementation note
    if (!screenLocked && is_playing && currentState == STATE_PLAYER && (now - last_progress_update > 400)) {
        updateProgressBar();
        last_progress_update = now;
    }

    // English implementation note
    if (touchscreen.touched()) {
        TS_Point p = touchscreen.getPoint();

        if (p.x == 8191 || p.y == 8191 || p.z < 250) {
            vTaskDelay(pdMS_TO_TICKS(5));
            return;
        }

        const bool INVERT_TOUCH_X = false; 
        const bool INVERT_TOUCH_Y = false; 

        // English implementation note
        int rawX = map(p.x, 200, 3700, 0, SCREEN_W - 1);
        int rawY = map(p.y, 240, 3800, 0, SCREEN_H - 1);
        rawX = constrain(rawX, 0, SCREEN_W - 1);
        rawY = constrain(rawY, 0, SCREEN_H - 1);

        // English implementation note
        int x = INVERT_TOUCH_X ? (SCREEN_W - rawX) : rawX;
        int y = INVERT_TOUCH_Y ? (SCREEN_H - rawY) : rawY;

        if (screenLocked) {
            screenLocked = false;
            lastScreenActivity = millis();
            applyScreenBrightness();
            drawCurrentStateUI();
            vTaskDelay(pdMS_TO_TICKS(10));
            return;
        }

        // English implementation note
        Serial.printf("[Touch] x=%d, y=%d (Current State: %d)\n", x, y, currentState);

        if (millis() - last_touch_time > 300) {
            lastScreenActivity = millis();
            processGlobalTouch(x, y);
            last_touch_time = millis();
        }
    }

    vTaskDelay(pdMS_TO_TICKS(10)); 
}