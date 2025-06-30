#ifdef USE_RS485
#ifdef USE_EPCO01

#define XSNS_122 122
#define XRS485_40 40

struct EPCO01t
{
    bool valid = false;
    uint16_t temperature = 0.0;
    char name[9] = "EP-CO-01";
} EPCO01;

#define EPCO01_ADDRESS_ID 0x25
#define EPCO01_ADDRESS_VALUE 0x0006
#define EPCO01_FUNCTION_CODE 0x03
#define EPCO01_ADDRESS_CHECK 0x0100

bool EPCO01isConnected()
{
    if (!RS485.active)
        return false;

    RS485.Rs485Modbus->Send(EPCO01_ADDRESS_ID, EPCO01_FUNCTION_CODE, EPCO01_ADDRESS_CHECK, 0x01);

    delay(200);

    RS485.Rs485Modbus->ReceiveReady();

    uint8_t buffer[8];
    uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);

    if (error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("EPCO01 has error %d"), error);
        return false;
    }
    else
    {
        uint16_t check_EPCO01 = (buffer[3] << 8) | buffer[4];
        if (check_EPCO01 == EPCO01_ADDRESS_ID)
            return true;
    }
    return false;
}

void EPCO01Init()
{
    if (!RS485.active)
        return;
    EPCO01.valid = EPCO01isConnected();
    if (EPCO01.valid)
        Rs485SetActiveFound(EPCO01_ADDRESS_ID, EPCO01.name);
    AddLog(LOG_LEVEL_INFO, EPCO01.valid ? PSTR("EPCO01 is connected") : PSTR("EPCO01 is not detected"));
}

const char HTTP_SNS_EPCO01[] PROGMEM = "{s} Temperature {m} %.1f";
#define D_JSON_EPCO01 "EPCO01"

void EPCO01ReadData()
{
    if (!EPCO01.valid)
        return;

    if (isWaitingResponse(EPCO01_ADDRESS_ID))
        return;

    if ((RS485.requestSent[EPCO01_ADDRESS_ID] == 0) && (RS485.lastRequestTime == 0))
    {
        RS485.Rs485Modbus->Send(EPCO01_ADDRESS_ID, EPCO01_FUNCTION_CODE, EPCO01_ADDRESS_VALUE, 1);
        RS485.requestSent[EPCO01_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }

    if ((RS485.requestSent[EPCO01_ADDRESS_ID] == 1) && (millis() - RS485.lastRequestTime >= 200))
    {
        if (!RS485.Rs485Modbus->ReceiveReady())
            return;
        uint8_t buffer[8];
        uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);

        if (error)
        {
            AddLog(LOG_LEVEL_INFO, PSTR("Modbus EPCO01 error: %d"), error);
        }
        else
        {
            uint16_t temperatureRaw = (buffer[3] << 8) | buffer[4];
            EPCO01.temperature = temperatureRaw;
        }
        RS485.requestSent[EPCO01_ADDRESS_ID] = 0;
        RS485.lastRequestTime = 0;
    }
}

void EPCO01Show(bool json)
{
    if (json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), EPCO01.name);
        ResponseAppend_P(PSTR("\"" D_JSON_EPCO01 "\":%.1f"), EPCO01.temperature);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_EPCO01, EPCO01.temperature);
    }
#endif
}

bool Xsns122(uint32_t function)
{
    if (!Rs485Enabled(XRS485_40))
    {
        return false;
    }
    bool result = false;
    if (FUNC_INIT == function)
    {
        EPCO01Init();
    }
    else if (EPCO01.valid)
    {
        switch (function)
        {
        case FUNC_EVERY_250_MSECOND:
            EPCO01ReadData();
            break;
        case FUNC_JSON_APPEND:
            EPCO01Show(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
            EPCO01Show(0);
            break;
#endif
        }
    }
    return result;
}
#endif
#endif