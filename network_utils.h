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
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "config.h"

#define HEADER_NAME_LEN 32
#define HEADER_VALUE_LEN 64

struct HttpHeader {
  char name[HEADER_NAME_LEN];
  char value[HEADER_VALUE_LEN];
};

using HttpResponseCallback = void (*)(int code, const HttpHeader* headers, size_t count);

void initHttpRequest();

int processHttpRequest(
  const char* url,
  const char* method,
  String* body = nullptr,
  HttpHeader* extraHeaders = nullptr,
  size_t headersCount = 0,
  const char* collectHeaders[] = nullptr,
  int collectHeadersCount = 0,
  int timeout = 15000,
  HttpResponseCallback callback = nullptr);

void setBaseHeaders(HTTPClient& http);

void printResponseHeaders(HTTPClient& http);

void network_SetHeader(HttpHeader& header, const char* name, const char* value);
