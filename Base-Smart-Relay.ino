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

#include "config.h"

#include "network_utils.h"
#include "string_utils.h"

#include "commands.h"

#define BASE_URL "https://smart.udfsoft.com/api/v1/devices/commands"
#define GET_COMMAND_URL BASE_URL

// HEADERS NAMES
#define X_POLL_INTERVAL "X-POLL-INTERVAL"
#define X_CMD "X-CMD"
#define X_CMD_PARAM "X-CMD-PARAM"
#define X_CMD_STATUS "X-CMD-STATUS"

const unsigned long DEFAULT_POLL_INTERVAL = 15000;

unsigned long lastPoll = 0;

unsigned long pollInterval = DEFAULT_POLL_INTERVAL;

void setup() {
  Serial.begin(115200);

  setupWifi();

  initHttpRequest();
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

  const char* collectHeaders[] = {
    X_CMD,
    X_CMD_PARAM,
    X_POLL_INTERVAL
  };

  processHttpRequest(GET_COMMAND_URL, "GET", nullptr, nullptr, 0, collectHeaders, 3, 15000, [](int code, const HttpHeader* headers, size_t count) {
    Serial.print("HTTPS Response code: ");
    Serial.println(code);

    switch (code) {
      case HTTP_CODE_NO_CONTENT:
        handleCommand(headers, count);
        break;
      case HTTP_CODE_NOT_FOUND:
        Serial.println("No command");
        break;
      case HTTP_CODE_FORBIDDEN:
        Serial.println("Access Forbidden! DEVICE_ID not found or API_KEY not valid");
        break;
      default:
        Serial.print("Unexpected code: ");
        Serial.println(code);
    }
  });
}

void handleCommand(const HttpHeader* headers, size_t headersCount) {
  char cmd[32] = { 0 };
  char param[32] = { 0 };

  for (size_t i = 0; i < headersCount; i++) {
    const char* name = headers[i].name;

    if (strcmp(name, X_CMD) == 0) {  // if name == X_CMD
      const char* value = headers[i].value;
      strlcpy(cmd, value, sizeof(cmd));
    } else if (strcmp(name, X_CMD_PARAM) == 0) {  // if name == X_CMD_PARAM
      strlcpy(param, headers[i].value, sizeof(param));
    } else if (strcmp(name, X_POLL_INTERVAL) == 0) {  // if name == X_POLL_INTERVAL
      pollInterval = atoi(headers[i].value);
      if (pollInterval <= 1000) pollInterval = DEFAULT_POLL_INTERVAL;
    }
  }

  Serial.print("cmd: ");
  Serial.println(cmd);
  Serial.print("param: ");
  Serial.println(param);
  Serial.print("pollInterval: ");
  Serial.println(pollInterval);

  executeCommand(cmd, param);
}

void executeCommand(const char* cmd, const char* param) {
  if (!cmd || strlen(cmd) == 0) {
    Serial.println("No command received");
    return;
  }

  char status[128] = { 0 };

  if (strcmp(cmd, COMMAND_PIN_ON) == 0) {
    cmdOn(status, sizeof(status), param);
  } else if (strcmp(cmd, COMMAND_PIN_OFF) == 0) {
    cmdOff(status, sizeof(status), param);
  } else if (strcmp(cmd, COMMAND_PIN_WATCH) == 0) {
    cmdStatus(status, sizeof(status), param);
  } else if (strcmp(cmd, COMMAND_HARDRESET) == 0) {
    cmdHardReset(status, sizeof(status), param, [](const char* cmd, char* status) {
      Serial.println("Smart device: RESET!");
      Serial.flush();
      delay(200);
      sendResult(cmd, status);
    });

    return;
  } else if (strcmp(cmd, COMMAND_REBOOT) == 0) {
    cmdReboot(status, sizeof(status), param, [](const char* cmd, char* status) {
      sendResult(cmd, status);
    });
    return;
  } else {
    Serial.print("Unknown command: ");
    Serial.println(cmd);
    strncpy(status, "Unknown command", sizeof(status) - 1);
  }

  // delay(100);
  yield();

  sendResult(cmd, status);
}

void sendResult(const char* cmd, const char* status) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("sendResult: WiFi.status() != WL_CONNECTED");
    return;
  }

  HttpHeader headers[1];
  strlcpy(headers[0].name, X_CMD_STATUS, sizeof(headers[0].name));
  strlcpy(headers[0].value, status, sizeof(headers[0].value));

  char postCommandUrl[256] = { 0 };

  snprintf(
    postCommandUrl,
    sizeof(postCommandUrl),
    "%s/%s",
    BASE_URL,
    cmd);

  sanitizePath(postCommandUrl);

  processHttpRequest(postCommandUrl, "POST", nullptr, headers, 1);
}
