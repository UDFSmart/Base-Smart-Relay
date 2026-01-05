/*
 *    Copyright 2025 UDFOwner
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 *    More details: https://udfsoft.com/
 */

#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define APP_VERSION "2"
#define DEVICE_ID "xxxx-xxxx-xxxx-xxxx" // YOUR DEVICE ID, to get it write to us: support@udfsoft.com

#define BASE_URL "https://smart.udfsoft.com/api/v1/devices/commands"
#define GET_COMMAND_URL BASE_URL

#define API_KEY "XXXXXX"  // YOUR API Key, to get it write to us: support@udfsoft.com

// HEADERS NAMES
#define X_POLL_INTERVAL "X-POLL-INTERVAL"
#define X_CMD "X-CMD"
#define X_CMD_PARAM "X-CMD-PARAM"
#define X_CMD_STATUS "X-CMD-STATUS"

// COMMAND LIST
#define COMMAND_NO_COMMAND "NO_COMMAND"

#define COMMAND_PIN_ON "ON"
#define COMMAND_PIN_OFF "OFF"

#define COMMAND_PIN_WATCH "STATUS"

#define COMMAND_HARDRESET "HARDRESET"
#define COMMAND_REBOOT "REBOOT"


const unsigned long DEFAULT_POLL_INTERVAL = 15000;

using HttpCallback = void (*)(HTTPClient& http, int code);

struct HttpHeader {
  const char* name;
  const char* value;
};

int processHttpRequest(
  const char* url,
  const char* method,
  String* body = nullptr,
  HttpHeader* extraHeaders = nullptr,
  size_t headersCount = 0,
  int timeout = 15000,
  HttpCallback callback = nullptr);

WiFiClientSecure client;  // WiFiClient client;
HTTPClient http;

unsigned long lastPoll = 0;

unsigned long pollInterval = DEFAULT_POLL_INTERVAL;

String lastCmdId = "";

void setup() {
  Serial.begin(115200);

  setupWifi();

  client.setInsecure();
}

void setupWifi() {
  WiFiManager wm;

  wm.setConnectTimeout(120);  // 2 mins
  wm.setConfigPortalTimeout(300);

  // If the connection fails, the configurator will start
  if (!wm.autoConnect("SMART_ESP_AP", "12345678")) {
    Serial.println("Failed to connect, rebooting...");
    ESP.restart();
  }

  Serial.println("Connected to WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (millis() - lastPoll >= pollInterval) {
    lastPoll = millis();
    pollServer();
  }
}

void pollServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting WiFi...");
    WiFi.reconnect();
    return;  // не делаем HTTPS пока нет WiFi
  }

  processHttpRequest(GET_COMMAND_URL, "GET", nullptr, nullptr, 0, 15000, [](HTTPClient& http, int code) {
    // Callback: обработка команд сразу после GET

    Serial.print("HTTPS Response code: ");
    Serial.println(code);

    if (code == HTTP_CODE_NO_CONTENT) {
      handleCommand(http);
    } else {
      Serial.print("Unexpected code: ");
      Serial.println(code);
    }
  });
}

void handleCommand(HTTPClient& http) {
  int headersCount = http.headers();
  char cmd[32] = { 0 };
  char param[32] = { 0 };

  for (int i = 0; i < headersCount; i++) {
    String headerName = http.headerName(i);
    String headerValue = http.header(i);

    Serial.print(headerName);
    Serial.print(": ");

    Serial.println(headerValue);

    if (headerName.equalsIgnoreCase(X_CMD)) {
      headerValue.toCharArray(cmd, sizeof(cmd));
    } else if (headerName.equalsIgnoreCase(X_CMD_PARAM)) {
      headerValue.toCharArray(param, sizeof(param));
    } else if (headerName.equalsIgnoreCase(X_POLL_INTERVAL)) {

      pollInterval = headerValue.toInt();
      if (pollInterval <= 1000) pollInterval = DEFAULT_POLL_INTERVAL;
    }
  }

  http.end();

  executeCommand(cmd, param);
}

void executeCommand(const char* cmd, const char* param) {
  if (!cmd || strlen(cmd) == 0) {
    Serial.println("No command received");
    return;
  }

  char status[128] = { 0 };

  if (strcmp(cmd, COMMAND_NO_COMMAND) == 0) {
    Serial.println("No command");
    return;
  } else if ((strcmp(cmd, COMMAND_PIN_ON) == 0 || strcmp(cmd, COMMAND_PIN_OFF) == 0 || strcmp(cmd, COMMAND_PIN_WATCH) == 0) && strlen(param) > 0) {
    int pin = atoi(param);
    if (pin != 0 && pin != 2) {
      Serial.print("Invalid pin: ");
      Serial.println(pin);
      strncpy(status, "Invalid pin!", sizeof(status) - 1);
    } else {
      if (strcmp(cmd, COMMAND_PIN_ON) == 0) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
        snprintf(status, sizeof(status), "Pin %d set HIGH", pin);
      } else if (strcmp(cmd, COMMAND_PIN_OFF) == 0) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        snprintf(status, sizeof(status), "Pin %d set LOW", pin);
      } else if (strcmp(cmd, COMMAND_PIN_WATCH) == 0) {
        pinMode(pin, INPUT);
        int state = digitalRead(pin);
        snprintf(status, sizeof(status), "Pin %d state: %d", pin, state);
      }
    }
  } else if (strcmp(cmd, COMMAND_HARDRESET) == 0) {
    Serial.println("Smart device: RESET!");
    Serial.flush();
    delay(200);
    sendResult(cmd, "Device: hardreset!");
    WiFi.disconnect(true);
    delay(200);
    ESP.eraseConfig();
    delay(500);
    ESP.restart();
    return;
  } else if (strcmp(cmd, COMMAND_REBOOT) == 0) {
    delay(200);
    sendResult(cmd, "Device: rebooted!");
    delay(500);
    ESP.restart();
    return;
  } else {
    Serial.print("Unknown command: ");
    Serial.println(cmd);
    strncpy(status, "Unknown command", sizeof(status) - 1);
  }

  delay(100);
  yield();

  sendResult(cmd, status);
}

void sendResult(const char* cmd, const char* status) {
  if (WiFi.status() != WL_CONNECTED) return;

  HttpHeader headers[] = {
    { X_CMD_STATUS, status }
  };

  char postCommandUrl[256] = { 0 };

  snprintf(
    postCommandUrl,
    sizeof(postCommandUrl),
    "%s/%s",
    BASE_URL,
    cmd);

  sanitizePath(postCommandUrl);

  processHttpRequest(postCommandUrl, "POST", nullptr, headers, 1, 15000);
}

// Универсальный метод с дополнительными заголовками
int processHttpRequest(
  const char* url,
  const char* method,  // "GET" или "POST"
  String* body,        // тело запроса (для POST)
  // String* response,          // сюда вернётся ответ (опционально)
  HttpHeader* extraHeaders,  // дополнительные заголовки
  size_t headersCount,       // их количество
  int timeout,
  HttpCallback callback) {

  // Serial.println("===== Start Request ====");
  // Serial.print(method);
  // Serial.print(" ");
  // Serial.println(url);
  // Serial.println("===== End Request ====");

  http.begin(client, url);
  http.setTimeout(timeout);
  http.setReuse(true);  // keep-alive

  // Добавляем стандартные заголовки
  setBaseHeaders(http);

  // Добавляем дополнительные заголовки
  for (size_t i = 0; i < headersCount; i++) {
    http.addHeader(extraHeaders[i].name, extraHeaders[i].value);
  }

  const char* keys[] = {
    X_CMD,
    X_CMD_PARAM,
    X_POLL_INTERVAL
  };

  http.collectHeaders(keys, 3);

  int code = -1;

  if (strcmp(method, "POST") == 0) {
    String payload = (body != nullptr) ? *body : "";
    code = http.POST(payload);
  } else if (strcmp(method, "GET") == 0) {
    code = http.GET();
  }


  // Serial.println("===== Start Response ====");
  // Serial.print(method);
  // Serial.print(" ");
  // Serial.println(url);
  // Serial.print("Headers:");
  // Serial.println("===== End Response ====");

  if (callback) {
    callback(http, code);
  } else {
    http.end();
  }

  return code;
}

void printResponseHeaders(HTTPClient& http) {
  int count = http.headers();

  Serial.print("Headers count: ");
  Serial.println(count);

  for (int i = 0; i < count; i++) {
    Serial.print(http.headerName(i));
    Serial.print(": ");
    Serial.println(http.header(i));
  }
}

void setBaseHeaders(HTTPClient& http) {
  http.addHeader("Prefer", "return=minimal");
  http.addHeader("X-Api-Key", API_KEY);
  http.addHeader("X-DEVICE-ID", DEVICE_ID);
  http.addHeader("X-CHIP-ID", String(ESP.getChipId()));
  http.addHeader("X-MAC", WiFi.macAddress());
  http.addHeader("X-APP-VERSION", APP_VERSION);

  char buf[64];

  snprintf(buf, sizeof(buf), "%d", WiFi.RSSI());
  http.addHeader("X-WIFI-RSSI", buf);

  snprintf(buf, sizeof(buf), "%lu", millis() / 1000);
  http.addHeader("X-UPTIME", buf);

  snprintf(buf, sizeof(buf), "%u", ESP.getFreeHeap());
  http.addHeader("X-FREE-HEAP", buf);

  snprintf(buf, sizeof(buf), "%u", ESP.getFreeSketchSpace());
  http.addHeader("X-FREE-SKETCH", buf);

  snprintf(buf, sizeof(buf), "%u", ESP.getFlashChipSize());
  http.addHeader("X-FLASH-SIZE", buf);

  snprintf(buf, sizeof(buf), "%u", ESP.getFlashChipRealSize());
  http.addHeader("X-FLASH-REAL", buf);
}

void sanitizePath(char* s) {
  for (; *s; s++) {
    if (*s == ' ') *s = '_';
  }
}
