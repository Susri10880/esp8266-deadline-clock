# ESP8266 Deadline Clock ⏰

A minimalist DIY desk clock built using the ESP8266 NodeMCU and a 0.96-inch OLED display. It displays the current time and date, while also allowing users to set deadlines remotely through a web interface hosted on the ESP8266.

The clock synchronizes time over Wi-Fi using NTP and supports Indian Standard Time (IST).

## ✨ Features

- **Minimalist clock display:** Large, centered time with the date displayed at the bottom.
- **Automatic time synchronization:** Uses NTP servers to maintain accurate time.
- **IST support:** Configured for UTC+5:30.
- **Deadline management:** Set a deadline and its title using a phone or computer browser.
- **Countdown display:** Automatically displays the remaining time for 5 seconds every 30 seconds.
- **Persistent storage:** Saves deadline information using EEPROM so it survives restarts.
- **No physical buttons:** All deadline configuration is handled through a web interface.

## 🛠️ Hardware Components

| Component | Specification |
|---|---|
| Microcontroller | NodeMCU ESP8266 |
| Display | 0.96-inch OLED |
| Display resolution | 128 × 64 pixels |
| Display driver | SSD1306 |
| Communication | I²C |
| Power | USB / 5V NodeMCU input |

## 🔌 Circuit Connections

| OLED Pin | NodeMCU Pin |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | D2 (GPIO4) |
| SCL | D1 (GPIO5) |

OLED I²C address: `0x3C`

## 💻 Software Requirements

- Arduino IDE
- ESP8266 Arduino Core
- Adafruit GFX Library
- Adafruit SSD1306 Library

## ⚙️ Configuration

Update the Wi-Fi credentials in the Arduino sketch:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

Configure Indian Standard Time:

```cpp
configTime(
  "IST-5:30",
  "pool.ntp.org",
  "time.google.com"
);
```

## 🚀 Setup Instructions

1. Connect the OLED display to the NodeMCU using the wiring table.
2. Install the required Arduino libraries.
3. Select the NodeMCU ESP8266 board in Arduino IDE.
4. Enter your Wi-Fi credentials in the sketch.
5. Upload the code to the ESP8266.
6. Open the Serial Monitor at 115200 baud.
7. Wait for the ESP8266 to connect to Wi-Fi and print its IP address.
8. Open that IP address in a phone or computer browser connected to the same Wi-Fi network.
9. Set a deadline and optionally provide a title.

## 🖥️ Display Behaviour

### Normal Clock
- Displays large HH:MM time in the center.
- Displays the date at the bottom.
- Uses a clean interface without unnecessary headings or status labels.

### Deadline Countdown
- Displays the deadline title.
- Shows the remaining days, hours, minutes, and seconds.
- Appears for 5 seconds every 30 seconds when a deadline is configured.
- Returns to the normal clock automatically.

If no deadline is configured, only the normal clock is displayed.

## 🌐 Web Interface

The ESP8266 hosts a local web server that allows users to:

- Set a deadline name.
- Select a date and time.
- Save the deadline.
- Clear the saved deadline.

The web interface is accessible through the ESP8266's local IP address.

## 💾 Data Storage

Deadline information is stored in EEPROM emulation, allowing the configured deadline to remain available after restarting the device.

## 🔮 Future Improvements

- Adjustable countdown display intervals.
- Multiple deadline support.
- Custom OLED themes and typography.
- Configurable display brightness.
- Automatic Wi-Fi configuration portal.
- Improved countdown animations.

## 📜 License

This project is open-source. You can use, modify, and improve it for personal and educational purposes.
