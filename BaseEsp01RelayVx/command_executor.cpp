/*
 *    Copyright 2026 UDFOwner
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

#include <Arduino.h>
#include <cstddef>
#include "command_executor.h"

static Command commands[] = {
  { COMMAND_PIN_ON, commands_setPinOn },
  { COMMAND_PIN_OFF, commands_setPinOff },
  { COMMAND_RELAY_ON, commands_setRelayOn },
  { COMMAND_RELAY_OFF, commands_setRelayOff },
  { COMMAND_PIN_WATCH, commands_setStatus },
  { COMMAND_REBOOT, commands_setReboot },
  { COMMAND_HARDRESET, commands_setHardReset },
};


void command_executor_execute(const char *cmd, const char *param, CommandFunctionCallback function) {
  if (!cmd || strlen(cmd) == 0) {
    Serial.println("No command received");
    return;
  }

  size_t commandsCount = sizeof(commands) / sizeof(commands[0]);

  for (size_t i = 0; i < commandsCount; ++i) {
    if (strcmp(commands[i].name, cmd) == 0) {
      commands[i].function(param, function);

      return;
    }
  }
  
  if (function) function(cmd, param, "Unknown command");
}