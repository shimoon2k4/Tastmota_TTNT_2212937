#ifdef USE_RS485
#ifdef USE_EPSO2

#define XSNS_119    119
#define XRS485_30   30

struct EP_SO2t
{
    bool valid = false;
    uint16_t temperature = 0.0;
    char name[7] = "EP_SO2";
} EP_SO2;

#define EP_SO2_ADDRESS_ID       0x21
#define EP_SO2_ADDRESS_VALUE    0x0006
#define EP_SO2_FUNCTION_CODE    0x03
#define EP_SO2_ADDRESS_CHECK    0x0100

bool EP_SO2isConnected()
{
    if (!RS485.active)
        return false;

    RS485.Rs485Modbus->Send(EP_SO2_ADDRESS_ID, EP_SO2_FUNCTION_CODE, EP_SO2_ADDRESS_CHECK, 0x01);

    delay(200);

    RS485.Rs485Modbus->ReceiveReady();

    uint8_t buffer[8];
    uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);

    if (error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("EP_SO2 has error %d"), error);
        return false;
    }
    else
    {
        uint16_t check_EP_SO2 = (buffer[3] << 8) | buffer[4];
        if (check_EP_SO2 == EP_SO2_ADDRESS_ID)
            return true;
    }
    return false;
}

void EP_SO2Init()
{
    if (!RS485.active)
        return;
    EP_SO2.valid = EP_SO2isConnected();
    if (EP_SO2.valid)
        Rs485SetActiveFound(EP_SO2_ADDRESS_ID, EP_SO2.name);
    if (EP_SO2.valid)
        AddLog(LOG_LEVEL_INFO, PSTR("EP_SO2 is connected"));
    else
        AddLog(LOG_LEVEL_INFO, PSTR("EP_SO2 is not detected"));
}

const char HTTP_SNS_EP_SO2[] PROGMEM = "{s} SO2 (ppm) {m} %.1f";
#define D_JSON_EP_SO2 "EP_SO2"

void EP_SO2ReadData()
{
    if (!EP_SO2.valid)
        return;

    if (isWaitingResponse(EP_SO2_ADDRESS_ID))
        return;

    if ((RS485.requestSent[EP_SO2_ADDRESS_ID] == 0) && (RS485.lastRequestTime == 0))
    {
        RS485.Rs485Modbus->Send(EP_SO2_ADDRESS_ID, EP_SO2_FUNCTION_CODE, EP_SO2_ADDRESS_VALUE, 1);
        RS485.requestSent[EP_SO2_ADDRESS_ID] = 1;
        RS485.lastRequestTime = millis();
    }

    if ((RS485.requestSent[EP_SO2_ADDRESS_ID] == 1) && (millis() - RS485.lastRequestTime >= 200))
    {
        if (!RS485.Rs485Modbus->ReceiveReady())
            return;
        uint8_t buffer[8];
        uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);

        if (error)
        {
            AddLog(LOG_LEVEL_INFO, PSTR("Modbus EP_SO2 error: %d"), error);
        }
        else
        {
            uint16_t temperatureRaw = (buffer[3] << 8) | buffer[4];
            EP_SO2.temperature = temperatureRaw;
        }
        RS485.requestSent[EP_SO2_ADDRESS_ID] = 0;
        RS485.lastRequestTime = 0;
    }
}

void EP_SO2Show(bool json)
{
    if (json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), EP_SO2.name);
        ResponseAppend_P(PSTR("\"" D_JSON_EP_SO2 "\":%.1f"), EP_SO2.temperature);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_EP_SO2, EP_SO2.temperature);
    }
#endif
}

bool Xsns119(uint32_t function)
{
    if (!Rs485Enabled(XRS485_30))
    {
        return false;
    }
    bool result = false;
    if (FUNC_INIT == function)
    {
        EP_SO2Init();
    }
    else if (EP_SO2.valid)
    {
        switch (function)
        {
        case FUNC_EVERY_250_MSECOND:
            EP_SO2ReadData();
            break;
        case FUNC_JSON_APPEND:
            EP_SO2Show(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
            EP_SO2Show(0);
            break;
#endif
        }
    }
    return result;
}
#endif
#endif