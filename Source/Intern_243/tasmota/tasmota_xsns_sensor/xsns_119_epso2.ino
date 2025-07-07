#ifdef USE_RS485
#ifdef USE_EPSO2

#define XSNS_119 119
#define XRS485_36 36

struct EPSO2_t
{
    bool valid = false;
    float so2 = 0.0;
    char name[12] = "EP SO2";
} EP_SO2;

#define EP_SO2_ADDRESS_ID      0x21
#define EP_SO2_ADDRESS_CHECK   0x0100
#define EP_SO2_ADDRESS_SO2     0x0006
#define EP_SO2_FUNCTION_CODE   0x03
#define EP_SO2_TIMEOUT         150

bool EP_SO2isConnected()
{
    if (!RS485.active) return false;

    RS485.Rs485Modbus->Send(EP_SO2_ADDRESS_ID, EP_SO2_FUNCTION_CODE, EP_SO2_ADDRESS_CHECK, 0x01);
    AddLog(LOG_LEVEL_INFO, PSTR("Sending Modbus to EP_SO2..."));
    delay(500);
    AddLog(LOG_LEVEL_INFO, PSTR("Reading response from EP_SO2..."));

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
        if (check_EP_SO2 == EP_SO2_ADDRESS_ID) return true;
    }
    return false;
}

void EP_SO2Init(void)
{
    if (!RS485.active) return;
    EP_SO2.valid = EP_SO2isConnected();
    if (EP_SO2.valid) Rs485SetActiveFound(EP_SO2_ADDRESS_ID, EP_SO2.name);
    AddLog(LOG_LEVEL_INFO, EP_SO2.valid ? PSTR("EP_SO2 is connected") : PSTR("EP_SO2 is not detected"));
}

void EP_SO2ReadData(void)
{
    if (!EP_SO2.valid) return;
    if (isWaitingResponse(EP_SO2_ADDRESS_ID)) return;

    // Queue các request cần đọc (ở đây chỉ có 1 giá trị SO2)
    static const struct
    {
        uint16_t regAddr;
        uint8_t regCount;
        void (*assign)(float);
    } EP_SO2Queue[] = {
        {EP_SO2_ADDRESS_SO2, 1, [](float v){ EP_SO2.so2 = v / 10.0f; }},
    };
    static uint8_t queueIndex = 0;
    static bool waitingResponse = false;
    static uint32_t lastRequestTime = 0;

    if (!waitingResponse) {
        RS485.Rs485Modbus->Send(
            EP_SO2_ADDRESS_ID,
            EP_SO2_FUNCTION_CODE,
            EP_SO2Queue[queueIndex].regAddr,
            EP_SO2Queue[queueIndex].regCount
        );
        waitingResponse = true;
        lastRequestTime = millis();
    } else if (millis() - lastRequestTime >= 200) {
        if (RS485.Rs485Modbus->ReceiveReady()) {
            uint8_t buffer[8];
            uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, sizeof(buffer));
            if (error) {
                AddLog(LOG_LEVEL_INFO, PSTR("Modbus EP_SO2 error %d"), error);
            } else if (buffer[0] == EP_SO2_ADDRESS_ID) {
                float value = (float)((buffer[3] << 8) | buffer[4]);
                EP_SO2Queue[queueIndex].assign(value);
            }
            queueIndex = (queueIndex + 1) % (sizeof(EP_SO2Queue) / sizeof(EP_SO2Queue[0]));
            waitingResponse = false;
        }
    }
}

const char HTTP_SNS_EP_SO2[] PROGMEM = "{s} SO2 (ppm) {m} %.1f";
#define D_JSON_EP_SO2 "EP_SO2"

void EP_SO2Show(bool json)
{
    if (json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), EP_SO2.name);
        ResponseAppend_P(PSTR("\"" D_JSON_EP_SO2 "\":%.1f"), EP_SO2.so2);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_EP_SO2, EP_SO2.so2);
    }
#endif
}

bool Xsns119(uint32_t function)
{
    if (!Rs485Enabled(XRS485_36)) return false;

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