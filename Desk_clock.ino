/*
  ESP8266 Desk Deadline Clock
  - NodeMCU ESP8266 : NodeMCU 1.0 ( ESP-12E module) for uploading
  - 128x64 SSD1306 I2C OLED, address 0x3C
  - Wi-Fi + NTP time synchronization
  - India Standard Time (IST, UTC+5:30)
  - Set deadline from a phone browser
  - Save deadline across reboots using EEPROM
  - Countdown appears for 5 seconds every 30 seconds
  - No buttons required
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- Wi-Fi: EDIT THESE ----------
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";

// ---------- OLED ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define OLED_SDA D2
#define OLED_SCL D1

Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1
);

// ---------- WEB SERVER ----------
ESP8266WebServer server(80);

// ---------- EEPROM ----------
const int EEPROM_SIZE = 512;
const uint32_t SAVE_MAGIC = 0xC10C2026;

struct SavedData {
  uint32_t magic;
  int64_t deadline;
  char title[32];
};

SavedData saved;

int64_t deadlineEpoch = 0;
char deadlineTitle[32] = "";

bool oledReady = false;
unsigned long lastDisplayUpdate = 0;
unsigned long bootMillis = 0;

// ---------- TIME SETTINGS ----------

void setupTimeZone() {
  // POSIX timezone signs are reversed:
  // IST is UTC+5:30, so use IST-5:30.
  setenv("TZ", "Asia/Kolkata", 1);
  tzset();
}

bool timeIsValid() {
  return time(nullptr) > 1700000000;
}

// ---------- EEPROM FUNCTIONS ----------

void loadDeadline() {
  EEPROM.get(0, saved);

  if (saved.magic == SAVE_MAGIC && saved.deadline > 0) {
    deadlineEpoch = saved.deadline;

    strncpy(
      deadlineTitle,
      saved.title,
      sizeof(deadlineTitle) - 1
    );

    deadlineTitle[sizeof(deadlineTitle) - 1] = '\0';

    Serial.println("Saved deadline loaded.");
  } else {
    deadlineEpoch = 0;
    deadlineTitle[0] = '\0';

    Serial.println("No saved deadline.");
  }
}

void saveDeadline() {
  memset(&saved, 0, sizeof(saved));

  saved.magic = SAVE_MAGIC;
  saved.deadline = deadlineEpoch;

  strncpy(
    saved.title,
    deadlineTitle,
    sizeof(saved.title) - 1
  );

  EEPROM.put(0, saved);
  EEPROM.commit();

  Serial.println("Deadline saved.");
}

void clearDeadline() {
  deadlineEpoch = 0;
  deadlineTitle[0] = '\0';

  memset(&saved, 0, sizeof(saved));
  EEPROM.put(0, saved);
  EEPROM.commit();

  Serial.println("Deadline cleared.");
}

// ---------- HTML PAGE ----------

String makeWebPage(const String& message = "") {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Deadline Clock</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      max-width: 440px;
      margin: 35px auto;
      padding: 0 18px;
      background: #111827;
      color: #f9fafb;
    }
    .card {
      background: #1f2937;
      padding: 22px;
      border-radius: 14px;
    }
    h2 { margin-top: 0; }
    label {
      display: block;
      margin-top: 16px;
      margin-bottom: 7px;
    }
    input {
      box-sizing: border-box;
      width: 100%;
      padding: 12px;
      border-radius: 8px;
      border: 1px solid #4b5563;
      font-size: 16px;
    }
    button {
      width: 100%;
      margin-top: 18px;
      padding: 13px;
      border: 0;
      border-radius: 8px;
      font-size: 16px;
      cursor: pointer;
    }
    .save { background: #34d399; color: #111827; }
    .clear { background: #f87171; color: #111827; }
    .message { color: #6ee7b7; }
    .note { color: #d1d5db; font-size: 14px; }
  </style>
</head>
<body>
  <div class="card">
    <h2>Desk Deadline Clock</h2>
    <p class="note">Set a deadline using Indian Standard Time (IST).</p>
)rawliteral";

  if (message.length() > 0) {
    page += "<p class='message'>";
    page += message;
    page += "</p>";
  }

  page += R"rawliteral(
    <form action="/set" method="POST">
      <label for="title">Deadline name</label>
      <input id="title" name="title" maxlength="31"
             placeholder="e.g. Project submission">

      <label for="due">Deadline date and time</label>
      <input id="due" name="due" type="datetime-local" required>

      <button class="save" type="submit">Save deadline</button>
    </form>

    <form action="/clear" method="POST"
          onsubmit="return confirm('Clear the saved deadline?')">
      <button class="clear" type="submit">Clear deadline</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

  return page;
}

// ---------- WEB HANDLERS ----------

void handleHome() {
  server.send(200, "text/html", makeWebPage());
}

void handleSetDeadline() {
  if (!server.hasArg("due")) {
    server.send(400, "text/plain", "Missing deadline date/time.");
    return;
  }

  if (!timeIsValid()) {
    server.send(
      503,
      "text/plain",
      "Clock has not synchronized with NTP yet. Try again shortly."
    );
    return;
  }

  String input = server.arg("due");

  int year, month, day, hour, minute;

  // Expected format: YYYY-MM-DDTHH:MM
  if (sscanf(
        input.c_str(),
        "%4d-%2d-%2dT%2d:%2d",
        &year, &month, &day, &hour, &minute
      ) != 5) {
    server.send(400, "text/plain", "Invalid date/time format.");
    return;
  }

  if (month < 1 || month > 12 ||
      day < 1 || day > 31 ||
      hour < 0 || hour > 23 ||
      minute < 0 || minute > 59) {
    server.send(400, "text/plain", "Invalid date/time values.");
    return;
  }

  struct tm due = {};
  due.tm_year = year - 1900;
  due.tm_mon = month - 1;
  due.tm_mday = day;
  due.tm_hour = hour;
  due.tm_min = minute;
  due.tm_sec = 0;
  due.tm_isdst = -1;

  // mktime interprets the entered date as local IST
  time_t converted = mktime(&due);

  if (converted <= 0) {
    server.send(400, "text/plain", "Could not convert deadline.");
    return;
  }

  if ((int64_t)converted <= (int64_t)time(nullptr)) {
    server.send(
      400,
      "text/plain",
      "Please choose a future deadline."
    );
    return;
  }

  deadlineEpoch = (int64_t)converted;

  String title = server.arg("title");
  title.trim();

  if (title.length() == 0) {
    title = "Deadline";
  }

  strncpy(
    deadlineTitle,
    title.c_str(),
    sizeof(deadlineTitle) - 1
  );
  deadlineTitle[sizeof(deadlineTitle) - 1] = '\0';

  saveDeadline();

  // Show countdown immediately for the next 5 seconds.
  bootMillis = millis();

  Serial.print("Deadline epoch: ");
  Serial.println((long)deadlineEpoch);

  struct tm localDue;
  localtime_r(&converted, &localDue);

  char dueText[32];
  strftime(
    dueText,
    sizeof(dueText),
    "%d %b %Y %H:%M",
    &localDue
  );

  Serial.print("Deadline in IST: ");
  Serial.println(dueText);

  server.send(
    200,
    "text/html",
    makeWebPage("Deadline saved successfully!")
  );
}

void handleClearDeadline() {
  clearDeadline();
  bootMillis = millis();

  server.send(
    200,
    "text/html",
    makeWebPage("Deadline cleared. Showing the normal clock.")
  );
}

// ---------- OLED DISPLAY ----------

void drawClock() {
  time_t now = time(nullptr);
  struct tm local;

  if (!timeIsValid()) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 28);
    display.println("Syncing time...");
    display.display();
    return;
  }

  if (localtime_r(&now, &local) == nullptr) {
    return;
  }

  char timeBuffer[16];
  char dateBuffer[24];

  strftime(
    timeBuffer, sizeof(timeBuffer),
    "%H:%M:%S", &local
  );

  strftime(
    dateBuffer, sizeof(dateBuffer),
    "%a, %d %b %Y", &local
  );

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Large time, centered horizontally and vertically
  display.setTextSize(2.5);

  int16_t x1, y1;
  uint16_t textWidth, textHeight;

  display.getTextBounds(
    timeBuffer, 0, 0,
    &x1, &y1, &textWidth, &textHeight
  );

  int timeX = (SCREEN_WIDTH - textWidth) / 2;
  display.setCursor(timeX, 19);
  display.print(timeBuffer);

  // Date centered along the bottom
  display.setTextSize(1);

  display.getTextBounds(
    dateBuffer, 0, 0,
    &x1, &y1, &textWidth, &textHeight
  );

  int dateX = (SCREEN_WIDTH - textWidth) / 2;
  display.setCursor(dateX, 54);
  display.print(dateBuffer);

  display.display();
}

void drawCountdown() {
  if (!timeIsValid()) {
    drawClock();
    return;
  }

  int64_t now = (int64_t)time(nullptr);
  int64_t remaining = deadlineEpoch - now;

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("COUNTDOWN");

  display.setCursor(0, 13);
  display.println(deadlineTitle);

  if (remaining <= 0) {
    display.setTextSize(2);
    display.setCursor(12, 31);
    display.println("TIME'S UP!");

    display.setTextSize(1);
    display.setCursor(0, 55);
    display.println("Deadline reached");
  } else {
    int64_t days = remaining / 86400;
    int hours = (remaining % 86400) / 3600;
    int minutes = (remaining % 3600) / 60;
    int seconds = remaining % 60;

    char countdown[24];

    if (days > 0) {
      snprintf(
        countdown, sizeof(countdown),
        "%lldd %02dh %02dm",
        (long long)days, hours, minutes
      );

      display.setTextSize(2);
      display.setCursor(0, 30);
      display.println(countdown);

      char secText[20];
      snprintf(secText, sizeof(secText), "%02d seconds", seconds);
      display.setTextSize(1);
      display.setCursor(0, 53);
      display.println(secText);
    } else {
      snprintf(
        countdown, sizeof(countdown),
        "%02d:%02d:%02d",
        hours, minutes, seconds
      );

      display.setTextSize(2);
      display.setCursor(5, 32);
      display.println(countdown);

      // display.setTextSize(1);
      // display.setCursor(0, 55);
      // display.println("Hours : Minutes : Seconds");
    }
  }

  display.display();
}

void updateDisplay() {
  if (!oledReady) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Show countdown for 5 seconds in each 30-second cycle,
  // but only when a deadline has been saved.
  unsigned long phase = (millis() - bootMillis) % 30000UL;

  bool showCountdown =
    (deadlineEpoch > 0) && (phase < 5000UL);

  if (showCountdown) {
    drawCountdown();
  } else {
    drawClock();
  }
}

// ---------- SETUP ----------

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("Starting Desk Deadline Clock...");

  setupTimeZone();

  Wire.begin(OLED_SDA, OLED_SCL);

  oledReady = display.begin(
    SSD1306_SWITCHCAPVCC,
    OLED_ADDR
  );

  if (oledReady) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Starting clock...");
    display.display();
  } else {
    Serial.println("OLED not found at address 0x3C.");
  }

  EEPROM.begin(EEPROM_SIZE);
  loadDeadline();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  unsigned long wifiStart = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - wifiStart < 30000UL) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected.");
    Serial.print("Open this address on your phone: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi connection failed.");
    Serial.println("Check SSID/password and restart.");
  }

  // Keep the system clock in UTC; TZ converts local time to IST.
  configTime(
  "IST-5:30",
  "pool.ntp.org",
  "time.google.com"
  );

  server.on("/", HTTP_GET, handleHome);
  server.on("/set", HTTP_POST, handleSetDeadline);
  server.on("/clear", HTTP_POST, handleClearDeadline);

  server.onNotFound([]() {
    server.send(404, "text/plain", "Page not found.");
  });

  server.begin();
  Serial.println("Web server started.");

  bootMillis = millis();
}

// ---------- MAIN LOOP ----------

void loop() {
  server.handleClient();

  // Retry Wi-Fi if the connection drops.
  static unsigned long lastWiFiAttempt = 0;

  if (WiFi.status() != WL_CONNECTED &&
      millis() - lastWiFiAttempt > 15000UL) {
    lastWiFiAttempt = millis();
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  // Refresh the OLED four times per second.
  if (millis() - lastDisplayUpdate >= 250UL) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }

  // Print local time periodically for troubleshooting.
  static unsigned long lastTimeLog = 0;

  if (millis() - lastTimeLog >= 10000UL) {
    lastTimeLog = millis();

    if (timeIsValid()) {
      time_t now = time(nullptr);
      struct tm local;

      if (localtime_r(&now, &local) != nullptr) {
        char buffer[32];

        strftime(
          buffer, sizeof(buffer),
          "%Y-%m-%d %H:%M:%S",
          &local
        );

        Serial.print("Local IST: ");
        Serial.println(buffer);
      }
    } else {
      Serial.println("Waiting for NTP synchronization...");
    }
  }
}