#include "HardwareSerial.h"
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

#include "network_utils.h"

WiFiClientSecure client;
HTTPClient http;

void copyHeaders(HttpHeader* resHeaders, int count, HTTPClient& http);

void initHttpRequest() {
  client.setInsecure();
}

int processHttpRequest(
  const char* url,
  const char* method,
  String* body,
  HttpHeader* extraHeaders,
  size_t headersCount,
  const char* collectHeaders[],
  int collectHeadersCount,
  int timeout,
  HttpResponseCallback callback) {

  http.begin(client, url);
  http.setTimeout(timeout);
  http.setReuse(true);

  setBaseHeaders(http);

  for (size_t i = 0; i < headersCount; i++) {
    http.addHeader(extraHeaders[i].name, extraHeaders[i].value);
  }

  if (collectHeadersCount > 0)
    http.collectHeaders(collectHeaders, collectHeadersCount);

  int code = -1;
  Serial.println("===== Start Request ====");
  Serial.print(method);
  Serial.print(" ");
  Serial.println(url);
  if (strcmp(method, "POST") == 0) {
    code = http.POST(body ? *body : "");
  } else {
    code = http.GET();
  }
  Serial.println("===== End Request ====");
  Serial.println();

  int responseHeadersCount = http.headers();

  HttpHeader responseHeaders[responseHeadersCount > 0 ? responseHeadersCount : 1];

  if (responseHeadersCount > 0)
    copyHeaders(responseHeaders, responseHeadersCount, http);

  http.end();

  if (callback) {
    callback(code, responseHeaders, responseHeadersCount);
  }

  return code;
}

void copyHeaders(HttpHeader* resHeaders, int count, HTTPClient& http) {
  for (int i = 0; i < count; i++) {
    String name = http.headerName(i);
    String value = http.header(i);

    strlcpy(resHeaders[i].name, name.c_str(), HEADER_NAME_LEN);
    strlcpy(resHeaders[i].value, value.c_str(), HEADER_VALUE_LEN);
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
