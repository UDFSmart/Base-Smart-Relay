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

#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>

// COMMAND LIST
#define COMMAND_RELAY_ON "RELAY_ON"
#define COMMAND_RELAY_OFF "RELAY_OFF"

#define COMMAND_PIN_ON "ON"
#define COMMAND_PIN_OFF "OFF"

#define COMMAND_PIN_WATCH "STATUS"

#define COMMAND_HARDRESET "HARDRESET"
#define COMMAND_REBOOT "REBOOT"
// End COMMAND LIST

using CommandFunctionCallback = void (*)(const char* cmd, const char* param, const char* status);

// ======================== PUBLIC COMMANDS ======================== \\

void commands_setPinState(char* result, size_t resultSize, const char* param, int state);

void commands_setPinOn(const char* param, CommandFunctionCallback callback);
void commands_setPinOff(const char* param, CommandFunctionCallback callback);

void commands_setRelayOn(const char* param, CommandFunctionCallback callback);
void commands_setRelayOff(const char* param, CommandFunctionCallback callback);

void commands_setStatus(const char* param, CommandFunctionCallback callback);

void commands_setReboot(const char* param, CommandFunctionCallback callback);

void commands_setHardReset(const char* param, CommandFunctionCallback callback);
