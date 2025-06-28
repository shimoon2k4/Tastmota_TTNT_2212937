/*
  xdrv_123_rs485.ino - RS485 Modbus support for Tasmota

  Copyright (C) 2024

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_RS485

#define XDRV_123 123

#include <stdint.h>
#include <TasmotaModbus.h>

// ======================= RS485 Config & State =========================

#ifndef RS485_MODBUS_SPEED
#define RS485_MODBUS_SPEED 9600
#endif

struct RS485State {
  bool active = false;
  TasmotaModbus* modbus = nullptr;
} RS485;

// ======================= RS485 Core Functions =========================

void RS485SettingsLoad(bool save) {
  // Placeholder for loading/saving settings if needed
}

bool RS485InitData(void) {
  // Placeholder for data initialization
  return true;
}
// ======================= RS485 Initialization =========================

void RS485Init(void) {
  RS485SettingsLoad(false);
  RS485.active = false;
  if (PinUsed(GPIO_RS485_RX) && PinUsed(GPIO_RS485_TX)) {
    RS485.modbus = new TasmotaModbus(Pin(GPIO_RS485_RX), Pin(GPIO_RS485_TX));
    uint8_t result = RS485.modbus->Begin(RS485_MODBUS_SPEED);
    if (result) {
      if (2 == result) {
        ClaimSerial();
      }
      if (RS485InitData()) {
        RS485.active = true;
      }
    }
#ifdef DEBUG_TASMOTA_SENSOR
    else {
      AddLog(LOG_LEVEL_DEBUG, PSTR("RS485: RS485Init Modbus init failed %d"), result);
    }
#endif
  }
#ifdef DEBUG_TASMOTA_SENSOR
  else {
    AddLog(LOG_LEVEL_DEBUG, PSTR("RS485: RS485Init no GPIOs assigned"));
  }
#endif
}

// ======================= RS485 Presentation ===========================

void RS485Show(bool json) {
  if (json) {
    ResponseAppend_P(PSTR(",\"RS485\":\"%s\""), RS485.active ? "Active" : "Inactive");
#ifdef USE_WEBSERVER
  } else {
    WSContentSend_PD(PSTR("{s}RS485 Status{m}%s{e}"), RS485.active ? "Active" : "Inactive");
#endif
  }
}

// ======================= RS485 Interface ==============================

bool Xdrv123(uint32_t function) {
  bool result = true;

  if (RS485.modbus || function == FUNC_INIT) {
    switch (function) {
      case FUNC_INIT:
        RS485Init();
        break;
      case FUNC_LOOP:
      case FUNC_SLEEP_LOOP:
        // Place for polling or background tasks if needed
        break;
      case FUNC_EVERY_50_MSECOND:
        // Place for fast polling if needed
        break;
      case FUNC_EVERY_SECOND:
        // Place for 1s polling if needed
        break;
      case FUNC_COMMAND:
        // Handle RS485 commands here if needed
        break;
      case FUNC_JSON_APPEND:
        RS485Show(true);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_SENSOR:
        RS485Show(false);
        break;
#endif
      default:
        result = false;
        break;
    }
  }
  return result;
}

#endif  // USE_RS485