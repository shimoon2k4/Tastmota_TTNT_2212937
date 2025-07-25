#ifdef USE_RS485
#ifdef USE_ULTRA_SONIC_SENSOR

#define XSNS_125 125
#define XRS485_16 16

struct SZY102_t {
    bool valid = false;
    float level = 0.0;
    float temperature = 0.0;
    char name[19] = "SZY-102 Sensor";
} SZY102;

#define SZY102_ADDRESS_ID        0x01    // <-- sửa nếu thiết bị có địa chỉ khác (ví dụ: 12)
#define SZY102_FUNCTION_CODE     0x03
#define SZY102_TIMEOUT           200  // milliseconds

#define REG_LEVEL_ADDR           0x0000  // 0000-0001: mức đo
#define REG_TEMP_ADDR            0x0002  // 0002-0003: nhiệt độ

bool SZY102isConnected()
{
    if (!RS485.active) return false;

    RS485.Rs485Modbus->Send(SZY102_ADDRESS_ID, SZY102_FUNCTION_CODE, REG_LEVEL_ADDR, 2);
    delay(200);
    RS485.Rs485Modbus->ReceiveReady();

    uint8_t buffer[9];
    uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, sizeof(buffer));

    if (error) {
        AddLog(LOG_LEVEL_INFO, PSTR("SZY102 error %d"), error);
        return false;
    }

    if (buffer[0] != SZY102_ADDRESS_ID || buffer[1] != SZY102_FUNCTION_CODE) {
        AddLog(LOG_LEVEL_INFO, PSTR("SZY102 invalid response format (b0=%02X b1=%02X b2=%02X)"),
               buffer[0], buffer[1], buffer[2]);
        return false;
    }

    return true;
}

void SZY102Init(void) {
    if (!RS485.active) return;
    SZY102.valid = SZY102isConnected();
    if (SZY102.valid) Rs485SetActiveFound(SZY102_ADDRESS_ID, SZY102.name);
    AddLog(LOG_LEVEL_INFO, SZY102.valid ? PSTR("SZY102 connected") : PSTR("SZY102 not detected"));
}

void SZY102ReadData(void) {
    if (!SZY102.valid || isWaitingResponse(SZY102_ADDRESS_ID)) return;

    static uint8_t step = 0;
    static bool waiting = false;
    static uint32_t lastReqTime = 0;

    if (!waiting) {
        uint16_t reg = (step == 0) ? REG_LEVEL_ADDR : REG_TEMP_ADDR;
        RS485.Rs485Modbus->Send(SZY102_ADDRESS_ID, SZY102_FUNCTION_CODE, reg, 2);
        waiting = true;
        lastReqTime = millis();
    } else if (millis() - lastReqTime >= SZY102_TIMEOUT) {
        if (RS485.Rs485Modbus->ReceiveReady()) {
            uint8_t buffer[9];
            uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, sizeof(buffer));

            if (error) {
                AddLog(LOG_LEVEL_INFO, PSTR("SZY102 receive error: %d"), error);
            } else if (buffer[0] == SZY102_ADDRESS_ID && buffer[1] == 0x03 && buffer[2] == 4) {
                uint32_t raw = ((uint32_t)buffer[3] << 24) |
                   ((uint32_t)buffer[4] << 16) |
                   ((uint32_t)buffer[5] << 8)  |
                   ((uint32_t)buffer[6]);

    float val;
    memcpy(&val, &raw, sizeof(val));

    if (step == 0) {
        SZY102.level = val;
        AddLog(LOG_LEVEL_INFO, PSTR("SZY102: Level = %.2f m"), val);
    } else {
        SZY102.temperature = val;
        AddLog(LOG_LEVEL_INFO, PSTR("SZY102: Temperature = %.2f °C"), val);
    }
            } else {
                AddLog(LOG_LEVEL_INFO, PSTR("SZY102 read error: invalid response (b0=%02X b1=%02X b2=%02X)"),
                       buffer[0], buffer[1], buffer[2]);
            }

            step = (step + 1) % 2;
            waiting = false;
        }
    }
}

#define D_JSON_SZY_LEVEL "SZY_LEVEL"
#define D_JSON_SZY_TEMP  "SZY_TEMP"

const char HTTP_SNS_SZY_LEVEL[] PROGMEM = "{s}Level {m}%.2f m";
const char HTTP_SNS_SZY_TEMP[]  PROGMEM = "{s}Temperature {m}%.1f°C";

void SZY102Show(bool json) {
    if (json) {
        ResponseAppend_P(PSTR(",\"%s\":{"), SZY102.name);
        ResponseAppend_P(PSTR("\"" D_JSON_SZY_LEVEL "\":%.2f,"), SZY102.level);
        ResponseAppend_P(PSTR("\"" D_JSON_SZY_TEMP "\":%.2f"), SZY102.temperature);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else {
        WSContentSend_PD(HTTP_SNS_SZY_LEVEL, SZY102.level);
        WSContentSend_PD(HTTP_SNS_SZY_TEMP, SZY102.temperature);
    }
#endif
}

bool Xsns125(uint32_t function) {
    if (!Rs485Enabled(XRS485_16)) return false;

    if (function == FUNC_INIT) {
        SZY102Init();
    } else if (SZY102.valid) {
        switch (function) {
            case FUNC_EVERY_250_MSECOND:
                SZY102ReadData();
                break;
            case FUNC_JSON_APPEND:
                SZY102Show(true);
                break;
#ifdef USE_WEBSERVER
            case FUNC_WEB_SENSOR:
                SZY102Show(false);
                break;
#endif
        }
    }
    return true;
}

#endif
#endif
