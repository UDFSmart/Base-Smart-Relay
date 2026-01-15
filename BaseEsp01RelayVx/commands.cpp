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

#define COMMAND_RESULT_SIZE 128

#define RELAY_PIN_PARAM "0"

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
// Private Commands
// =======================

static void cmdOn(char* result, size_t resultSize, const char* param) {
  commands_setPinState(result, resultSize, param, HIGH);
}

static void cmdOff(char* result, size_t resultSize, const char* param) {
  commands_setPinState(result, resultSize, param, LOW);
}

static void cmdStatus(char* result, size_t resultSize, const char* param) {
  int pin = parsePin(param);

  if (pin < 0) {
    snprintf(result, resultSize, "Invalid pin");
    return;
  }

  pinMode(pin, OUTPUT);  // WARNING: relay output only, not for GPIO input usage
  int state = digitalRead(pin);

  snprintf(result, resultSize, "PIN %d state: %d", pin, state);
}

static void cmdReboot(char* result, size_t resultSize, const char* param, CommandFunctionCallback callback) {
  if (callback) {
    callback(COMMAND_REBOOT, param, "Device: rebooted!");
  }

  delay(300);

  ESP.restart();
}

static void cmdHardReset(char* result, size_t resultSize, const char* param, CommandFunctionCallback callback) {
  if (callback) {
    callback(COMMAND_HARDRESET, param, "Device: rebooted!");
  }

  yield();
  delay(500);

  WiFi.disconnect(true);
  delay(200);
  ESP.eraseConfig();
  delay(300);

  ESP.restart();
}

static void execAndCallback(
  const char* cmd,
  const char* param,
  void (*exec)(char*, size_t, const char*),
  CommandFunctionCallback cb) {
  char result[COMMAND_RESULT_SIZE] = { 0 };
  exec(result, COMMAND_RESULT_SIZE, param);
  if (cb) cb(cmd, param, result);
}

// =======================
// Public Commands
// =======================

void commands_setPinState(char* result, size_t resultSize, const char* param, int state) {
  int pin = parsePin(param);
  if (pin < 0) {
    snprintf(result, resultSize, "Invalid pin");
    return;
  }
  pinMode(pin, OUTPUT);
  digitalWrite(pin, state);
  snprintf(result, resultSize, "PIN %d -> %s", pin, state ? "HIGH" : "LOW");
}

void commands_setPinOn(const char* param, CommandFunctionCallback callback) {
  execAndCallback(COMMAND_PIN_ON, param, cmdOn, callback);
}

void commands_setPinOff(const char* param, CommandFunctionCallback callback) {
  execAndCallback(COMMAND_PIN_OFF, param, cmdOff, callback);
}

void commands_setRelayOn(const char* param, CommandFunctionCallback callback) {
  execAndCallback(COMMAND_RELAY_ON, RELAY_PIN_PARAM, cmdOn, callback);
}

void commands_setRelayOff(const char* param, CommandFunctionCallback callback) {
  execAndCallback(COMMAND_RELAY_OFF, RELAY_PIN_PARAM, cmdOff, callback);
}

void commands_setStatus(const char* param, CommandFunctionCallback callback) {
  execAndCallback(COMMAND_PIN_WATCH, param, cmdStatus, callback);
}

void commands_setReboot(const char* param, CommandFunctionCallback callback) {
  char result[COMMAND_RESULT_SIZE] = { 0 };
  cmdReboot(result, COMMAND_RESULT_SIZE, param, callback);
}

void commands_setHardReset(const char* param, CommandFunctionCallback callback) {
  char result[COMMAND_RESULT_SIZE] = { 0 };
  cmdHardReset(result, COMMAND_RESULT_SIZE, param, callback);
}
