# TTGO-T-Display-Smartwatch OS

A fully-featured smartwatch firmware for the **LILYGO T-Display** ESP32 board. Features a beautiful clock display, image viewer, system monitor, stopwatch, and WiFi analyzer - all powered by a colorful 240x135 LCD screen.

## Features

✨ **Clock Display** - 12-hour time with date and battery status  
🖼️ **Image Viewer** - Browse JPEG images stored on SPIFFS  
📊 **System Monitor** - Real-time battery, memory, uptime, and storage info  
⏱️ **Stopwatch** - Track time with HH:MM:SS.cs format  
📡 **WiFi Analyzer** - Scan and view detailed info about nearby networks  
🔋 **Battery Monitoring** - Live voltage and percentage display  
😴 **Auto-Sleep** - 30-second inactivity timeout to save power  
💤 **Deep Sleep** - Long button hold to save maximum battery  

## Hardware Requirements

- **LILYGO T-Display ESP32** (240x135 TFT LCD, 2 buttons, battery connector)
- **USB-C Cable** - For programming and charging
- **Battery** - LiPo/Polymer battery (recommended):
  - **Current Setup**: 502025-200mAh Polymer Battery (~8-10 hours runtime)
  - **Upgrade Option**: 5030 Polymer Battery (~15-20 hours runtime, check fit in case)
- **SPIFFS Partition** - Built-in flash storage for images (recommend 1-4 MB)

### Pin Configuration
- **Button LEFT (GPIO 0)** - Menu navigation, deep sleep trigger
- **Button RIGHT (GPIO 35)** - Feature navigation, mode switching  
- **Battery Monitor (GPIO 34)** - Analog input for battery voltage
- **TFT Display** - SPI interface (configured in TFT_eSPI_Setup.h)

## Software Requirements

### Arduino IDE Setup

1. **Install ESP32 Board Package**
   - Open Arduino IDE → Preferences
   - Add to "Additional Boards Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Tools → Board Manager → Search "esp32" → Install latest version

2. **Install Required Libraries**
   - Go to Sketch → Include Library → Manage Libraries
   - Search and install:
     - **TFT_eSPI** by Bodmer (v2.4.0+)
     - **TJpg_Decoder** by Bodmer
     - Built-in: SPI, FS, SPIFFS, WiFi, time, esp_sleep

3. **Configure TFT_eSPI**
   - Locate `TFT_eSPI_Setup.h` in your Arduino libraries folder
   - Uncomment or set:
     ```cpp
     #define ST7789_DRIVER
     #define TFT_WIDTH 240
     #define TFT_HEIGHT 135
     #define TFT_MOSI 19
     #define TFT_SCLK 18
     #define TFT_CS 5
     #define TFT_DC 16
     #define TFT_RST 23
     #define TFT_BL 4
     ```

4. **Select Board & Upload Port**
   - Tools → Board → ESP32 → ESP32 Dev Module
   - Tools → Port → Select your COM port
   - Tools → Upload Speed → 921600 (or 460800)

5. **Configure Partition Scheme (Important!)**
   - Tools → Partition Scheme → **Huge APP (3MB No OTA)**
   - This gives maximum space for firmware and SPIFFS storage
   - ⚠️ Required for full functionality with all features enabled

### Alternative: PlatformIO (Recommended)

```ini
[env:esp32]
platform = espressif32
board = lilygo-t-display
framework = arduino
lib_deps =
    bodmer/TFT_eSPI@^2.4.0
    bodmer/TJpg_Decoder@^0.1.0
```

## Installation Steps

1. **Download/Clone this repository**
   ```bash
   git clone https://github.com/your-username/ESP32-T-Display-Watch-Mod.git
   cd ESP32-T-Display-Watch-Mod
   ```

2. **Open the firmware**
   - Open `ino files/TTGO-MainOS.ino` in Arduino IDE

3. **Configure WiFi (Optional)**
   - Edit these lines in the code:
     ```cpp
     const char* ssid     = "Your WiFi Name";
     const char* password = "Your WiFi Password";
     ```
   - Leave empty if you don't want time sync via WiFi

4. **Upload to device**
   - Plug in TTGO T-Display via USB
   - Click Upload (Ctrl+U)
   - Wait for "Hard resetting via RTS pin..."

5. **Upload images (Optional)**
   - Use `ino files/TTGO-Uploader.ino` to add JPEG images:
     - Save and upload this sketch to your ESP32
     - Connect to WiFi network: `ESP32-Transfer` (password: `password`)
     - Open browser to `192.168.4.1`
     - Upload JPEG files (they'll be stored as `1.jpg`, `2.jpg`, etc.)
   - Or use Arduino IDE's SPIFFS tool to upload files manually

## Controls & Navigation

### Button Layout
```
LEFT Button (GPIO 0)  →  Secondary Features
RIGHT Button (GPIO 35) → Primary Features
Hold 1.5s (LEFT)      → Deep Sleep
Hold 1.0s (RIGHT)     → Image Viewer (from Clock/System)
```

### Screen Navigation Map

**Clock Screen** (Default)
- `LEFT Short` → System Info
- `RIGHT Short` → Stopwatch
- `RIGHT Hold 1.0s` → Image Viewer
- `LEFT Hold 1.5s` → Deep Sleep

**System Info**
- `LEFT Short` → Back to Clock
- `RIGHT Short` → WiFi Analyzer (scans networks)
- `RIGHT Hold 1.0s` → Image Viewer
- `LEFT Hold 1.5s` → Deep Sleep

**Stopwatch**
- `LEFT Short` → Start/Stop
- `RIGHT Short` → Reset
- `RIGHT Hold 1.0s` → Back to Clock
- `LEFT Hold 1.5s` → Deep Sleep

**Image Viewer**
- `LEFT Short` → Previous Image
- `RIGHT Short` → Next Image
- `RIGHT Hold 1.0s` → Back to Clock
- `LEFT Hold 1.5s` → Deep Sleep

**WiFi Analyzer**
- `RIGHT Short` → Scroll down (next network)
- `LEFT Short` → Show network details
- Back from Details: `LEFT Short` → Network list, `RIGHT Short` → Next network
- `LEFT Hold 1.5s` → Deep Sleep

## Configuration

Edit these settings at the top of `TTGO-MainOS.ino`:

```cpp
// Auto-sleep timeout (seconds)
#define AUTO_SLEEP_SEC 30

// Maximum images that can be loaded
#define MAX_SLIDES 20

// WiFi for NTP time sync (optional)
const char* ssid = "Your WiFi";
const char* password = "Your Password";

// Timezone offset (in seconds)
const long gmtOffset_sec = 28800; // UTC+8 (Philippines)
const int daylightOffset_sec = 0;
```

## File Structure

```
ESP32-T-Display-Watch-Mod/
├── ino files/
│   ├── TTGO-MainOS.ino       # Main smartwatch firmware
│   └── TTGO-Uploader.ino     # Web-based file uploader
├── LICENSE                   # MIT License
├── README.md                 # This file
└── 3D Case/                  # (Optional) 3D-printed case files
```

## Supported Image Formats

- **JPEG** (.jpg) - Recommended
- **Resolution** - 240x135 pixels (matches display exactly)
- **File Size** - ~15-50 KB per image (varies by compression)
- **Naming** - Store as `1.jpg`, `2.jpg`, `3.jpg`, etc. in SPIFFS

### Image Upload Tips
1. Compress images to 240x135 resolution
2. Use online JPEG optimizer for smaller file sizes
3. Upload via TTGO-Uploader.ino web interface
4. Watch SPIFFS storage indicator in System Info screen

## Troubleshooting

### Device won't upload
- Check COM port selection (Tools → Port)
- Try lower upload speed (460800 or 115200)
- Press Reset button during upload

### Display shows nothing
- Verify TFT_eSPI pin configuration matches your board
- Check USB cable connection and power supply
- Restart Arduino IDE after library installation

### Time/Date is wrong
- Enable WiFi in code and configure SSID/password
- Device syncs via NTP on startup (if time invalid)
- Manual time sync by pressing appropriate button combination

### Images not showing
- Verify images are stored as `1.jpg`, `2.jpg` in SPIFFS
- Use SPIFFS file manager in Arduino IDE to check storage
- Ensure images are exactly 240x135 pixels
- Watch System Info → SPIFFS usage to confirm space available

### Battery reading incorrect
- Calibrate `getBatteryLevel()` function for your battery chemistry
- Adjust map values: `map((long)(voltage * 100), 330, 420, 0, 100)`

## Performance Tips

- **Reduce refresh rate** - Increase `AUTO_SLEEP_SEC` for longer battery life
- **Disable WiFi scanning** - Remove WiFi Analyzer to save RAM
- **Use greyscale images** - Smaller file size = more images in storage
- **Optimize JPEG quality** - Use 70-80% compression for acceptable quality

## Power Consumption

- **Active (Clock)** - ~80mA
- **Auto-sleep (Display off)** - ~10-15mA
- **Deep sleep (full power down)** - ~0.5mA
- **Battery life** (with 502025-200mAh) - ~8-10 hours (depends on usage)
- **Battery life** (with 5030-upgrade) - ~15-20 hours (estimated)

### Battery Information

**Current Setup:**
- Model: 502025-200mAh Polymer LiPo Battery
- Typical runtime: 8-10 hours of normal usage
- Charge time: ~2-3 hours via USB

**Upgrade Option:**
- Model: 5030 Polymer LiPo Battery (larger capacity)
- Estimated runtime: 15-20 hours
- Note: Verify physical fit in your case before installing
- Check connector compatibility with your board

## Customization

### Change Watch Face Name
Edit line in `drawClock()`:
```cpp
tft.print("Your Watch Name");  // Replace "TannyGO Watch"
```

### Adjust Color Scheme
Modify color definitions at top:
```cpp
#define TFT_CYAN     0x07FF  // Custom 16-bit RGB565 color
#define TFT_ORANGE   0xFDA0
```

### Add Custom Features
The code is organized with clear function sections. Add new modes by:
1. Adding enum value to `AppMode`
2. Creating display function (e.g., `drawMyFeature()`)
3. Adding button handlers in `loop()`

## Known Limitations

- ❌ Bluetooth scanning removed (non-functional on this board revision)
- ❌ WiFi deauth not supported (hardware firmware limitation)
- ⚠️ 20 image maximum (can increase `MAX_SLIDES` if SPIFFS space permits)
- ⚠️ No network connection after scan (display-only, no data logging)

## Contributing

Found a bug or have an improvement? Feel free to submit issues and pull requests!

## Future Enhancements

Planned features for future releases:
- Weather display (OpenWeatherMap API)
- Alarm/Timer functionality
- Step counter (with pedometer)
- Custom watch faces
- Persistent storage of settings
- Bluetooth connectivity (when fixed)








## Credits & Acknowledgments

### Libraries Used
- **TFT_eSPI** by Bodmer - Display driver and graphics library
- **TJpg_Decoder** by Bodmer - JPEG image decoding
- **ESP-IDF** - Espressif's IoT Development Framework
- **Arduino Core for ESP32** - Arduino framework support

### Hardware & Design
- **LILYGO** - TTGO T-Display ESP32 board design
- **3D Case Design** - Remix of "T-Display Smartwatch Case" by tanmaychhatbar, used under CC BY 4.0 License
  - Original: https://www.thingiverse.com/thing:4730124
  - Modifications: Resized buttons and adjusted tolerances by Tanny Devs

### Inspiration & References
- TTGO T-Display community projects
- ESP32 smartwatch enthusiasts
- Arduino/PlatformIO development community

## License

This project is licensed under the **MIT License** - see [LICENSE](LICENSE) file for details.

You are free to:
- ✅ Use commercially
- ✅ Modify the code
- ✅ Distribute
- ✅ Use privately

Conditions:
- Include original license and copyright notice

### Third-Party Licenses
- **TFT_eSPI** - FreeBSD License
- **TJpg_Decoder** - FreeBSD License  
- **3D Case Design** - CC BY 4.0 (attribute to tanmaychhatbar)

## Support & Contact

For issues, questions, or suggestions:
- 📧 Create an issue on GitHub
- 💬 Check existing discussions
- 📖 Review the Troubleshooting section above

## Changelog

### v1.0.0 (2026-01-31)
- Initial release
- Clock display with battery monitoring
- Image viewer for SPIFFS storage
- System information monitor
- Stopwatch timer
- WiFi network analyzer
- Deep sleep and auto-sleep modes
- Web-based file uploader

---

**Made with ❤️ by Lawrence Tan**
