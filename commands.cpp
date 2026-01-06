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

#include "commands.h"

#include <Arduino.h>

#define COMMAND_RESULT_SIZE 64

// =======================
// Utils
// =======================

static bool isValidPin(int pin) {
  return (pin == 0 || pin == 2);  // ESP-01
}

static int parsePin(const char* param) {
  if (!param || !*param) return -1;
  int pin = atoi(param);
  return isValidPin(pin) ? pin : -1;
}

// =======================
// Commands
// =======================

void cmdOn(char* result, size_t resultSize, const char* param) {
  int pin = parsePin(param);

  if (pin < 0) {
    snprintf(result, resultSize, "Invalid pin");
    return;
  }

  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);

  snprintf(result, resultSize, "PIN %d -> HIGH", pin);
}

void cmdOff(char* result, size_t resultSize, const char* param) {
  int pin = parsePin(param);

  if (pin < 0) {
    snprintf(result, resultSize, "Invalid pin");
    return;
  }

  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);

  snprintf(result, resultSize, "PIN %d -> LOW", pin);
}

void cmdStatus(char* result, size_t resultSize, const char* param) {
  int pin = parsePin(param);

  if (pin < 0) {
    snprintf(result, resultSize, "Invalid pin");
    return;
  }

  pinMode(pin, INPUT);
  int state = digitalRead(pin);

  snprintf(result, resultSize, "PIN %d state: %d", pin, state);
}

void cmdReboot(char* result, size_t resultSize, const char* param, FunctionCallback callback) {
  if (callback) {
    callback(COMMAND_REBOOT, "Device: rebooted!");
  }

  delay(300);

  ESP.restart();
}

void cmdHardReset(char* result, size_t resultSize, const char* param, FunctionCallback callback) {
  if (callback) {
    callback(COMMAND_HARDRESET, "Device: rebooted!");
  }
  delay(300);

  WiFi.disconnect(true);
  delay(200);
  ESP.eraseConfig();
  delay(300);

  ESP.restart();
}