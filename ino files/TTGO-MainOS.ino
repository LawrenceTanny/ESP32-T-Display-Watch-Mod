// ========== TTGO T-Display Smartwatch OS ==========
// Features: Clock, Image Viewer, System Monitor, Stopwatch, WiFi Analyzer
// Hardware: ESP32 TTGO T-Display (240x135 LCD, 2 buttons, battery monitoring)
// Author: Lawrence Tan
// License: MIT License

#include <TFT_eSPI.h>
#include <SPI.h>
#include <FS.h>
#include <SPIFFS.h>
#include <TJpg_Decoder.h>
#include <WiFi.h>
#include <time.h>
#include <esp_sleep.h>
#include <vector>

// ================= CONFIGURATION =================
const char* ssid     = "Tan Family";
const char* password = "88888888";

// Screen settings
#define AUTO_SLEEP_SEC 30  // Turn off display after 30 seconds of inactivity
#define MAX_SLIDES     20  // Maximum number of JPEG images to load from SPIFFS

// NTP Time sync - adjust for your timezone
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800; // Philippines (UTC+8)
const int   daylightOffset_sec = 0;

// Button GPIO pins
#define BTN_LEFT  0
#define BTN_RIGHT 35

// Custom color palette (16-bit RGB565)
#define TFT_DARKGREY 0x2124
#define TFT_ORANGE   0xFDA0
#define TFT_CYAN     0x07FF

// Initialize display and app state variables
TFT_eSPI tft = TFT_eSPI();
int currentSlide = 1;
unsigned long lastActivityTime = 0;  // Track time for auto-sleep
unsigned long btnPressTime = 0;      // Track button press duration
bool longPressTriggered = false;     // Flag for long press (1.5s hold)

// WiFi network data structure - stores info about detected networks
struct WiFiNetwork {
  String ssid;
  uint8_t bssid[6];
  int channel;
  int rssi;
  String encryption;
};
std::vector<WiFiNetwork> wifiNetworks;

// App modes - switch between different screens/features
enum AppMode { MODE_CLOCK, MODE_VIEWER, MODE_SYSTEM_INFO, MODE_STOPWATCH, MODE_WIFI_ANALYZER };
AppMode currentMode = MODE_CLOCK;
int wifiSelection = 0;

// Stopwatch timer state variables
bool stopwatchRunning = false;
unsigned long stopwatchStartMs = 0;
unsigned long stopwatchElapsedMs = 0;
unsigned long lastStopwatchDraw = 0;

// Reset inactivity timer when user presses button
void resetSleepTimer() {
  lastActivityTime = millis();
}

// Enter deep sleep mode - press BTN_LEFT to wake
void goDeepSleep() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("Zzz...", 120, 60, 4);
  delay(1000);
  tft.writecommand(TFT_DISPOFF);
  tft.writecommand(TFT_SLPIN);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); 
  Serial.println("Entering Deep Sleep...");
  esp_deep_sleep_start();
}

// Callback function for JPEG decoder - draws image data to display
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

// Calculate battery percentage from analog pin reading (0-100%)
int getBatteryLevel() {
  uint16_t v = analogRead(34);
  float voltage = ((float)v / 4095.0) * 2.0 * 3.3 * 1.1;
  int percentage = map((long)(voltage * 100), 330, 420, 0, 100);
  return constrain(percentage, 0, 100);
}

// Read battery voltage in volts (3.3V - 4.2V range)
float getBatteryVoltage() {
  uint16_t v = analogRead(34);
  return ((float)v / 4095.0) * 2.0 * 3.3 * 1.1;
}

// Display system information: battery, uptime, memory, and SPIFFS storage
void drawSystemInfo() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.drawCentreString("System Info", 120, 8, 2);
  tft.drawLine(0, 22, 240, 22, TFT_DARKGREY);

  int bat = getBatteryLevel();
  float volts = getBatteryVoltage();
  tft.setTextSize(1);

  // Battery info - compact
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 28);
  tft.print("Bat:");
  tft.setTextColor(TFT_WHITE);
  tft.printf("%d%% %.2fV", bat, volts);

  // Battery bar - smaller
  tft.drawRect(130, 28, 100, 6, TFT_WHITE);
  tft.fillRect(130, 28, (100 * bat) / 100, 6, bat < 20 ? TFT_RED : TFT_GREEN);

  // Uptime
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 40);
  tft.print("Uptime:");
  tft.setTextColor(TFT_WHITE);
  unsigned long secs = millis() / 1000;
  unsigned long mins = secs / 60;
  unsigned long hrs = mins / 60;
  secs %= 60;
  mins %= 60;
  tft.printf("%02lu:%02lu:%02lu", hrs, mins, secs);

  // Heap
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 52);
  tft.print("Heap:");
  tft.setTextColor(TFT_WHITE);
  tft.printf("%u KB", ESP.getFreeHeap() / 1024);

  // CPU
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 64);
  tft.print("CPU:");
  tft.setTextColor(TFT_WHITE);
  tft.printf("%u MHz", getCpuFrequencyMhz());

  // SPIFFS Storage - shows free/total space for image storage
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 76);
  tft.print("SPIFFS:");
  tft.setTextColor(TFT_WHITE);
  size_t spiffsTotalBytes = SPIFFS.totalBytes();
  size_t spiffsUsedBytes = SPIFFS.usedBytes();
  size_t spiffsFreeBytes = spiffsTotalBytes - spiffsUsedBytes;
  // Convert to MB if total > 1MB for cleaner display
  if (spiffsTotalBytes > 1048576) {
    tft.printf("%.1f/%.1f MB", spiffsFreeBytes / 1048576.0, spiffsTotalBytes / 1048576.0);
  } else {
    tft.printf("%d/%d KB", spiffsFreeBytes / 1024, spiffsTotalBytes / 1024);
  }

  // Instructions
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(5, 120);
  tft.print("L:Back R:WiFi H:Exit");
}

// Display stopwatch timer - format: HH:MM:SS.cs (centiseconds)
void drawStopwatch() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.drawCentreString("Stopwatch", 120, 10, 2);
  tft.drawLine(0, 30, 240, 30, TFT_DARKGREY);

  // Calculate elapsed time including current running time
  unsigned long elapsed = stopwatchElapsedMs;
  if (stopwatchRunning) {
    elapsed += (millis() - stopwatchStartMs);
  }

  unsigned long ms = (elapsed % 1000) / 10;
  unsigned long totalSeconds = elapsed / 1000;
  unsigned long seconds = totalSeconds % 60;
  unsigned long minutes = (totalSeconds / 60) % 60;
  unsigned long hours = totalSeconds / 3600;

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(30, 70);
  tft.printf("%02lu:%02lu:%02lu.%02lu", hours, minutes, seconds, ms);

  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(5, 215);
  tft.print("L:Start/Stop R:Reset H:Exit");
}


// Display main clock face - shows time, date, and battery status
void drawClock() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;
  tft.fillScreen(TFT_BLACK);

  // Header bar with watch name and battery percentage
  tft.fillRoundRect(8, 6, 224, 24, 6, TFT_DARKGREY);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(14, 12);
  tft.print("TannyGO Watch");

  int bat = getBatteryLevel();
  tft.setTextColor(TFT_GREEN, TFT_DARKGREY);
  tft.setCursor(165, 12);
  tft.printf("%d%%", bat);
  tft.drawRect(200, 12, 24, 10, TFT_WHITE);
  tft.fillRect(200, 12, (24 * bat) / 100, 10, bat < 20 ? TFT_RED : TFT_GREEN);

  // Main time card
  tft.fillRoundRect(10, 36, 220, 70, 10, TFT_DARKGREY);
  tft.drawRoundRect(10, 36, 220, 70, 10, TFT_CYAN);
  tft.setTextSize(3);
  tft.setTextColor(TFT_CYAN, TFT_DARKGREY);
  tft.setCursor(28, 55);
  
  int hour12 = timeinfo.tm_hour;
  if (hour12 == 0) hour12 = 12;
  else if (hour12 > 12) hour12 -= 12;
  
  tft.printf("%02d", hour12);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.print(":");
  tft.setTextColor(TFT_ORANGE, TFT_DARKGREY);
  tft.printf("%02d", timeinfo.tm_min);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(168, 60);
  tft.println(timeinfo.tm_hour >= 12 ? "PM" : "AM");

  char dateStr[20];
  strftime(dateStr, 20, "%a, %B %d", &timeinfo);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawString(dateStr, 120, 115, 2);
}

// Display JPEG image from SPIFFS storage
void showSlide(int n) {
  // Load image file from SPIFFS (1.jpg, 2.jpg, etc.)
  String f = "/" + String(n) + ".jpg";
  if (SPIFFS.exists(f)) {
    TJpgDec.drawJpg(0, 0, f);
  } else {
    tft.fillScreen(TFT_RED);
    tft.setCursor(0,0);
    tft.setTextColor(TFT_WHITE);
    tft.println("Missing: " + f);
  }
}

// Connect to WiFi and sync time from NTP server
void syncTime() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.drawCentreString("Syncing Time...", 120, 60, 2);
  
  // Attempt WiFi connection to reach NTP server
  WiFi.begin(ssid, password);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(200); retry++;
  }
  if(WiFi.status() == WL_CONNECTED) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    tft.setTextColor(TFT_GREEN);
    tft.drawCentreString("Updated!", 120, 80, 2);
  } else {
    tft.setTextColor(TFT_RED);
    tft.drawCentreString("Failed!", 120, 80, 2);
  }
  delay(1000);
  drawClock();
}

// ============= WIFI ANALYZER FUNCTIONS =============

// Scan for all nearby WiFi networks and store in vector
void scanWiFiNetworks() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.drawCentreString("Scanning WiFi...", 120, 60, 2);
  
  Serial.println("[WIFI] Starting WiFi scan...");
  wifiNetworks.clear();
  wifiSelection = 0;
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  int numNetworks = WiFi.scanNetworks();
  
  Serial.printf("[WIFI] Found %d networks\n", numNetworks);
  for (int i = 0; i < numNetworks; i++) {
    WiFiNetwork net;
    net.ssid = WiFi.SSID(i);
    net.channel = WiFi.channel(i);
    net.rssi = WiFi.RSSI(i);
    memcpy(net.bssid, WiFi.BSSID(i), 6);
    
    // Map WiFi encryption type to human-readable string
    uint8_t encType = WiFi.encryptionType(i);
    if (encType == WIFI_AUTH_OPEN) {
      net.encryption = "OPEN";
    } else if (encType == WIFI_AUTH_WEP) {
      net.encryption = "WEP";
    } else if (encType == WIFI_AUTH_WPA_PSK) {
      net.encryption = "WPA";
    } else if (encType == WIFI_AUTH_WPA2_PSK) {
      net.encryption = "WPA2";
    } else if (encType == WIFI_AUTH_WPA_WPA2_PSK) {
      net.encryption = "WPA/WPA2";
    } else if (encType == WIFI_AUTH_WPA2_ENTERPRISE) {
      net.encryption = "Enterprise";
    } else {
      net.encryption = "Unknown";
    }
    
    wifiNetworks.push_back(net);
    
    Serial.printf("[WIFI] Network %d: %s (Ch: %d, RSSI: %d, Sec: %s)\n", 
                  i, net.ssid.c_str(), net.channel, net.rssi, net.encryption.c_str());
  }
  
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  tft.fillScreen(TFT_BLACK);
  if (wifiNetworks.size() == 0) {
    tft.setTextColor(TFT_ORANGE);
    tft.drawCentreString("No WiFi Networks", 120, 60, 2);
    delay(2000);
    currentMode = MODE_CLOCK;
    drawClock();
  }
}

// Display list of WiFi networks - scroll with RIGHT button to browse
void drawWiFiAnalyzerUI() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN);
  tft.drawCentreString("WiFi Analyzer", 120, 10, 2);
  
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, 35);
  tft.printf("Networks: %d", wifiNetworks.size());
  
  tft.drawLine(0, 55, 240, 55, TFT_DARKGREY);
  
  int itemHeight = 30;
  for (int i = 0; i < min(3, (int)wifiNetworks.size()); i++) {
    int displayIndex = (wifiSelection + i) % wifiNetworks.size();
    int yPos = 65 + (i * itemHeight);
    
    if (i == 0) {
      tft.fillRect(0, yPos - 2, 240, itemHeight, TFT_DARKGREY);
      tft.setTextColor(TFT_YELLOW);
    } else {
      tft.setTextColor(TFT_WHITE);
    }
    
    tft.setCursor(10, yPos);
    tft.setTextSize(1);
    tft.print(wifiNetworks[displayIndex].ssid.substring(0, 16));
    
    tft.setCursor(10, yPos + 10);
    tft.setTextSize(1);
    tft.setTextColor(TFT_ORANGE);
    tft.printf("Ch:%d ", wifiNetworks[displayIndex].channel);
    tft.print(wifiNetworks[displayIndex].encryption);
  }
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(5, 215);
  tft.print("R:DOWN L:DETAIL H:EXIT");
}

// Display detailed information about selected WiFi network
void showWiFiDetails() {
  // Show SSID, BSSID (MAC address), channel, signal strength, and security type
  WiFiNetwork &net = wifiNetworks[wifiSelection];
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN);
  tft.drawCentreString("WiFi Details", 120, 10, 2);
  tft.drawLine(0, 30, 240, 30, TFT_DARKGREY);
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 40);
  tft.print("SSID:");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(60, 40);
  tft.print(net.ssid.substring(0, 18));
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 55);
  tft.print("BSSID:");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(60, 55);
  tft.printf("%02X:%02X:%02X:%02X:%02X:%02X",
             net.bssid[0], net.bssid[1], net.bssid[2],
             net.bssid[3], net.bssid[4], net.bssid[5]);
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 70);
  tft.print("Channel:");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(80, 70);
  tft.print(net.channel);
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 85);
  tft.print("Signal:");
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(80, 85);
  tft.printf("%d dBm", net.rssi);
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 100);
  tft.print("Security:");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(80, 100);
  tft.print(net.encryption);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(5, 210);
  tft.print("L:Back R:Next H:Exit");
  
  delay(100);
  bool inDetail = true;
  while (inDetail) {
    if (digitalRead(BTN_LEFT) == LOW) {
      delay(100);
      while (digitalRead(BTN_LEFT) == LOW) delay(10);
      inDetail = false;
      drawWiFiAnalyzerUI();
      return;
    }
    if (digitalRead(BTN_RIGHT) == LOW) {
      delay(100);
      while (digitalRead(BTN_RIGHT) == LOW) delay(10);
      inDetail = false;
      if (wifiSelection < wifiNetworks.size() - 1) wifiSelection++;
      else wifiSelection = 0;
      showWiFiDetails();
      return;
    }
    delay(50);
  }
}

// Clean up WiFi mode and return to clock screen
void exitWiFiAnalyzer() {
  wifiNetworks.clear();
  WiFi.mode(WIFI_OFF);
  currentMode = MODE_CLOCK;
  drawClock();
}

// Initialize hardware and prepare app for first run
void setup() {
  // Initialize serial, buttons, and display
  Serial.begin(115200);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);  // 1 = landscape orientation
  tft.fillScreen(TFT_BLACK);

  // Initialize SPIFFS for image storage
  if (!SPIFFS.begin()) {
    tft.setTextColor(TFT_RED);
    tft.println("SPIFFS FAIL");
    while(1);
  }

  // Configure JPEG decoder for image display
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
  
  // Configure NTP time synchronization and check validity
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo) || timeinfo.tm_year < (2020 - 1900)) { 
    syncTime();  // Time invalid, sync from WiFi
  } else {
    drawClock();  // Time is valid, show clock
  }
  
  resetSleepTimer();
}

// Main event loop - handles button input, mode switching, and display updates
void loop() {
  // Auto-sleep: turn off display if no button press for 30 seconds
  if (millis() - lastActivityTime > (AUTO_SLEEP_SEC * 1000)) {
    goDeepSleep();
  }

  // Refresh stopwatch display every 100ms when running
  if (currentMode == MODE_STOPWATCH && stopwatchRunning) {
    if (millis() - lastStopwatchDraw > 100) {
      drawStopwatch();
      lastStopwatchDraw = millis();
    }
  }

  // LEFT BUTTON - Secondary navigation & features
  if (digitalRead(BTN_LEFT) == LOW) {
    btnPressTime = millis();
    longPressTriggered = false;
    resetSleepTimer(); 
    
    // Hold LEFT for 1.5s to enter deep sleep
    while (digitalRead(BTN_LEFT) == LOW) {
       if (millis() - btnPressTime > 1500 && !longPressTriggered) {
         longPressTriggered = true;
         while(digitalRead(BTN_LEFT) == LOW) delay(10); 
         goDeepSleep();
       }
    }

     if (!longPressTriggered) {
      if (currentMode == MODE_VIEWER) {
        if (currentSlide > 1) { currentSlide--; showSlide(currentSlide); }
      }
      else if (currentMode == MODE_CLOCK) {
        currentMode = MODE_SYSTEM_INFO; drawSystemInfo();
      }
      else if (currentMode == MODE_SYSTEM_INFO) {
        currentMode = MODE_CLOCK; drawClock();
      }
      else if (currentMode == MODE_STOPWATCH) {
        if (stopwatchRunning) {
          stopwatchElapsedMs += (millis() - stopwatchStartMs);
          stopwatchRunning = false;
        } else {
          stopwatchStartMs = millis();
          stopwatchRunning = true;
        }
        drawStopwatch();
      }
      else if (currentMode == MODE_WIFI_ANALYZER) {
        showWiFiDetails();
      }
     }
  }

  // RIGHT BUTTON - Primary navigation & features
  if (digitalRead(BTN_RIGHT) == LOW) {
    btnPressTime = millis();
    longPressTriggered = false;
    resetSleepTimer(); 
    
    // Hold RIGHT for 1.0s to enter Image Viewer from clock/system screens
    while (digitalRead(BTN_RIGHT) == LOW) {
      if (millis() - btnPressTime > 1000 && !longPressTriggered) {
        longPressTriggered = true;
        while(digitalRead(BTN_RIGHT) == LOW) delay(10);
        
        if (currentMode == MODE_CLOCK || currentMode == MODE_SYSTEM_INFO) {
          currentMode = MODE_VIEWER; showSlide(currentSlide);
        }
        else if (currentMode == MODE_VIEWER) {
          currentMode = MODE_CLOCK; drawClock();
        }
        else if (currentMode == MODE_STOPWATCH) {
          currentMode = MODE_CLOCK; drawClock();
        }
        else if (currentMode == MODE_WIFI_ANALYZER) {
          exitWiFiAnalyzer();
        }
      }
    }
    
    if (!longPressTriggered) {
       if (currentMode == MODE_VIEWER) {
         if (currentSlide < MAX_SLIDES) { currentSlide++; showSlide(currentSlide); }
       }
       else if (currentMode == MODE_CLOCK) {
         currentMode = MODE_STOPWATCH;
         stopwatchRunning = false;
         stopwatchElapsedMs = 0;
         drawStopwatch();
       }
       else if (currentMode == MODE_SYSTEM_INFO) {
         scanWiFiNetworks();
         if (wifiNetworks.size() > 0) {
           currentMode = MODE_WIFI_ANALYZER;
           drawWiFiAnalyzerUI();
         }
       }
       else if (currentMode == MODE_STOPWATCH) {
         stopwatchRunning = false;
         stopwatchElapsedMs = 0;
         drawStopwatch();
       }
       else if (currentMode == MODE_WIFI_ANALYZER) {
         if (wifiSelection < wifiNetworks.size() - 1) wifiSelection++;
         else wifiSelection = 0;
         drawWiFiAnalyzerUI();
       }
    }
  }
  
  // Refresh clock display every 10 seconds to update time
  if (currentMode == MODE_CLOCK) {
    static unsigned long lastClockUpdate = 0;
    if (millis() - lastClockUpdate > 10000) { 
       drawClock();
       lastClockUpdate = millis();
    }
  }
}