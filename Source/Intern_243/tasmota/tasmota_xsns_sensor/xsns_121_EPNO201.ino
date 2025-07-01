#ifdef USE_RS485
#ifdef USE_EPNO201

#define XSNS_EPNO201 121 
#define XRS485_39 39
// This code is for the EPNO201 temperature sensor using Modbus over RS485

struct EPNO201t
{
    bool valid = false;
    uint16_t temperature = 0.0;
    char name[8] = "EPNO201";
} EPNO201;

#define EPNO201_ADDRESS_ID 0x23
#define EPNO201_ADDRESS_VALUE 0x0006
#define EPNO201_FUNCTION_CODE 0x03
#define EPNO201_ADDRESS_CHECK 0x0100

bool EPNO201isConnected()
{
    if (!RS485.active)
        return false;

    RS485.Rs485Modbus->Send(EPNO201_ADDRESS_ID, EPNO201_FUNCTION_CODE, EPNO201_ADDRESS_CHECK, 0x01);

    delay(200);

    RS485.Rs485Modbus->ReceiveReady();

    uint8_t buffer[8];
    uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);

    if (error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("EPNO201 has error %d"), error);
        return false;
    }
    else
    {
        uint16_t check_EPNO201 = (buffer[3] << 8) | buffer[4];
        if (check_EPNO201 == EPNO201_ADDRESS_ID)
            return true;
    }
    return false;
}

void EPNO201Init()
{
    if (!RS485.active)
        return;
    EPNO201.valid = EPNO201isConnected();
    if (EPNO201.valid)
        Rs485SetActiveFound(EPNO201_ADDRESS_ID, EPNO201.name);
    const char* msg = EPNO201.valid ? "EPNO201 is connected" : "EPNO201 is not detected";
    AddLog(LOG_LEVEL_INFO, PSTR("EPNO201: %s"), msg);
}

const char HTTP_SNS_EPNO201[] PROGMEM = "{s} Temperature {m} %.1f";
#define D_JSON_EPNO201 "EPNO201"

void EPNO201ReadData()
{
    if (!EPNO201.valid)
        return;

    if (isWaitingResponse(EPNO201_ADDRESS_ID))
        return;

    if ((RS485.requestSent[EPNO201_ADDRESS_ID] == 0) && (RS485.lastRequestTime == 0))
    {
        RS485.Rs485Modbus->Send(EPNO201_ADDRESS_ID, EPNO201_FUNCTION_CODE, EPNO201_ADDRESS_VALUE, 1);
        RS485.requestSent[EPNO201_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }

    if ((RS485.requestSent[EPNO201_ADDRESS_ID] == 1) && (millis() - RS485.lastRequestTime >= 200))
    {
        if (!RS485.Rs485Modbus->ReceiveReady())
            return;
        uint8_t buffer[8];
        uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);

        if (error)
        {
            AddLog(LOG_LEVEL_INFO, PSTR("Modbus EPNO201 error: %d"), error);
        }
        else
        {
            uint16_t temperatureRaw = (buffer[3] << 8) | buffer[4];
            EPNO201.temperature = temperatureRaw;
        }
        RS485.requestSent[EPNO201_ADDRESS_ID] = 0;
        RS485.lastRequestTime = 0;
    }
}

void EPNO201Show(bool json)
{
    if (json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), EPNO201.name);
        ResponseAppend_P(PSTR("\"" D_JSON_EPNO201 "\":%.1f"), EPNO201.temperature);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_EPNO201, EPNO201.temperature);
    }
#endif
}

bool Xsns121(uint32_t function)
{
    if (!Rs485Enabled(XRS485_39))
    {
        return false;
    }
    bool result = false;
    if (FUNC_INIT == function)
    {
        EPNO201Init();
    }
    else if (EPNO201.valid)
    {
        switch (function)
        {
        case FUNC_EVERY_250_MSECOND:
            EPNO201ReadData();
            break;
        case FUNC_JSON_APPEND:
            EPNO201Show(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
            EPNO201Show(0);
            break;
#endif
        }
    }
    return result;
}
#endif
#endif