#ifdef USE_RS485
#ifdef USE_SOIL7IN1

#define XSNS_123 123
#define XRS485_35 35

struct SOIL7IN1t
{
    bool valid = false;
    float ph = 0.0;
    float moisture = 0.0;
    float temperature = 0.0;
    float conductivity = 0.0;
    float nitrogen = 0.0;
    float phosphorus = 0.0; 
    float potassium = 0.0;
    char name[19] = "7 in 1 Soil Sensor";
} SOIL7IN1;

#define SOIL7IN1_ADDRESS_ID 0x11
#define SOIL7IN1_ADDRESS_CHECK  0x0100
#define SOIL7IN1_ADDRESS_PH 0x0006
#define SOIL7IN1_ADDRESS_MOISTURE 0x0012
#define SOIL7IN1_ADDRESS_TEMPERATURE 0x0013
#define SOIL7IN1_ADDRESS_CONDUCTIVITY 0x0015
#define SOIL7IN1_ADDRESS_NITROGEN 0x001E
#define SOIL7IN1_ADDRESS_PHOSPHORUS 0x001F
#define SOIL7IN1_ADDRESS_POTASSIUM 0x0020
#define SOIL7IN1_FUNCTION_CODE 0x03
#define SOIL7IN1_TIMEOUT 150

bool SOIL7IN1isConnected()
{
    if(!RS485.active) return false;

    RS485.Rs485Modbus->Send(SOIL7IN1_ADDRESS_ID, SOIL7IN1_FUNCTION_CODE, SOIL7IN1_ADDRESS_CHECK, 0x01);

    delay(200);

    RS485.Rs485Modbus->ReceiveReady();

    uint8_t buffer[8];
    uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, 8);

    if (error)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("SOIL7IN1 has error %d"), error);
        return false;
    }
    else
    {   
        uint16_t check_SOIL7IN1 = (buffer[3] << 8) | buffer[4];
        if(check_SOIL7IN1 == SOIL7IN1_ADDRESS_ID) return true;
    }
    return false;
}

void SOIL7IN1Init(void)
{
    if(!RS485.active) return;
    SOIL7IN1.valid = SOIL7IN1isConnected();
    //if(!SOIL7IN1.valid) TasmotaGlobal.restart_flag = 2;
    if(SOIL7IN1.valid) Rs485SetActiveFound(SOIL7IN1_ADDRESS_ID, SOIL7IN1.name);
    AddLog(LOG_LEVEL_INFO, SOIL7IN1.valid ? PSTR("SOIL7IN1 is connected") : PSTR("SOIL7IN1 is not detected"));
}

void SOIL7IN1ReadData(void)
{
    if (!SOIL7IN1.valid) return;
    if (isWaitingResponse(SOIL7IN1_ADDRESS_ID)) return;

    // Queue các request cần đọc
    static const struct
    {
        uint16_t regAddr;
        uint8_t regCount;
        void (*assign)(float); // Hàm gán giá trị
    } SOIL7IN1Queue[] = {
        {SOIL7IN1_ADDRESS_PH, 1, [](float v){ SOIL7IN1.ph = v / 10.0f; }},
        {SOIL7IN1_ADDRESS_MOISTURE, 1, [](float v){ SOIL7IN1.moisture = v / 10.0f; }},
        {SOIL7IN1_ADDRESS_TEMPERATURE, 1, [](float v){ SOIL7IN1.temperature = v / 10.0f; }},
        {SOIL7IN1_ADDRESS_CONDUCTIVITY, 1, [](float v){ SOIL7IN1.conductivity = v; }},
        {SOIL7IN1_ADDRESS_NITROGEN, 1, [](float v){ SOIL7IN1.nitrogen = v; }},
        {SOIL7IN1_ADDRESS_PHOSPHORUS, 1, [](float v){ SOIL7IN1.phosphorus = v; }},
        {SOIL7IN1_ADDRESS_POTASSIUM, 1, [](float v){ SOIL7IN1.potassium = v; }},
    };
    static uint8_t queueIndex = 0;
    static bool waitingResponse = false;
    static uint32_t lastRequestTime = 0;

    if (!waitingResponse) {
        // Gửi request mới
        RS485.Rs485Modbus->Send(
            SOIL7IN1_ADDRESS_ID,
            SOIL7IN1_FUNCTION_CODE,
            SOIL7IN1Queue[queueIndex].regAddr,
            SOIL7IN1Queue[queueIndex].regCount
        );
        waitingResponse = true;
        lastRequestTime = millis();
    } else if (millis() - lastRequestTime >= 200) {
        // Đợi đủ thời gian, kiểm tra phản hồi
        if (RS485.Rs485Modbus->ReceiveReady()) {
            uint8_t buffer[8];
            uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, sizeof(buffer));
            if (error) {
                AddLog(LOG_LEVEL_INFO, PSTR("Modbus SOIL7IN1 error %d"), error);
            } else if (buffer[0] == SOIL7IN1_ADDRESS_ID) {
                float value = (float)((buffer[3] << 8) | buffer[4]);
                SOIL7IN1Queue[queueIndex].assign(value);
            }
            // Chuyển sang request tiếp theo trong queue
            queueIndex = (queueIndex + 1) % (sizeof(SOIL7IN1Queue) / sizeof(SOIL7IN1Queue[0]));
            waitingResponse = false;
        }
    }
}

const char HTTP_SNS_SOIL7IN1_PH[] PROGMEM = "{s} SOIL7IN1 PH {m} %.2fpH";
const char HTTP_SNS_SOIL7IN1_MOISTURE[] PROGMEM = "{s} SOIL7IN1 MOISTURE {m} %.1f%%";
const char HTTP_SNS_SOIL7IN1_TEMPERATURE[] PROGMEM = "{s} SOIL7IN1 TEMPERATURE {m} %.1f°C";
const char HTTP_SNS_SOIL7IN1_CONDUCTIVITY[] PROGMEM = "{s} SOIL7IN1 CONDUCTIVITY {m} %.1f µS/cm";
const char HTTP_SNS_SOIL7IN1_NITROGEN[] PROGMEM = "{s} SOIL7IN1 NITROGEN {m} %.1f mg/kg";
const char HTTP_SNS_SOIL7IN1_PHOSPHORUS[] PROGMEM = "{s} SOIL7IN1 PHOSPHORUS {m} %.1f mg/kg";
const char HTTP_SNS_SOIL7IN1_POTASSIUM[] PROGMEM = "{s} SOIL7IN1 POTASSIUM {m} %.1f mg/kg";

#define D_JSON_SOIL7IN1_PH "SOIL7IN1_PH"
#define D_JSON_SOIL7IN1_MOISTURE "SOIL7IN1_MOISTURE"
#define D_JSON_SOIL7IN1_TEMPERATURE "SOIL7IN1_TEMPERATURE"
#define D_JSON_SOIL7IN1_CONDUCTIVITY "SOIL7IN1_CONDUCTIVITY"
#define D_JSON_SOIL7IN1_NITROGEN "SOIL7IN1_NITROGEN"
#define D_JSON_SOIL7IN1_PHOSPHORUS "SOIL7IN1_PHOSPHORUS"
#define D_JSON_SOIL7IN1_POTASSIUM "SOIL7IN1_POTASSIUM"

void SOIL7IN1Show(bool json)
{
    if(json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), SOIL7IN1.name);
        ResponseAppend_P(PSTR("\"" D_JSON_SOIL7IN1_PH "\":%.2f,"), SOIL7IN1.ph);
        ResponseAppend_P(PSTR("\"" D_JSON_SOIL7IN1_MOISTURE "\":%.1f,"), SOIL7IN1.moisture);
        ResponseAppend_P(PSTR("\"" D_JSON_SOIL7IN1_TEMPERATURE "\":%.1f,"), SOIL7IN1.temperature);
        ResponseAppend_P(PSTR("\"" D_JSON_SOIL7IN1_CONDUCTIVITY "\":%.1f,"), SOIL7IN1.conductivity);
        ResponseAppend_P(PSTR("\"" D_JSON_SOIL7IN1_NITROGEN "\":%.1f,"), SOIL7IN1.nitrogen);
        ResponseAppend_P(PSTR("\"" D_JSON_SOIL7IN1_PHOSPHORUS "\":%.1f,"), SOIL7IN1.phosphorus);
        ResponseAppend_P(PSTR("\"" D_JSON_SOIL7IN1_POTASSIUM "\":%.1f"), SOIL7IN1.potassium);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_SOIL7IN1_PH, SOIL7IN1.ph);
        WSContentSend_PD(HTTP_SNS_SOIL7IN1_MOISTURE, SOIL7IN1.moisture);
        WSContentSend_PD(HTTP_SNS_SOIL7IN1_TEMPERATURE, SOIL7IN1.temperature);
        WSContentSend_PD(HTTP_SNS_SOIL7IN1_CONDUCTIVITY, SOIL7IN1.conductivity);
        WSContentSend_PD(HTTP_SNS_SOIL7IN1_NITROGEN, SOIL7IN1.nitrogen);
        WSContentSend_PD(HTTP_SNS_SOIL7IN1_PHOSPHORUS, SOIL7IN1.phosphorus);
        WSContentSend_PD(HTTP_SNS_SOIL7IN1_POTASSIUM, SOIL7IN1.potassium);
    }
#endif
}

bool Xsns123(uint32_t function)
{
    if(!Rs485Enabled(XRS485_35)) return false;

    bool result = false;
    if(FUNC_INIT == function)
    {
        SOIL7IN1Init();
    }
    else if(SOIL7IN1.valid)
    {
        switch (function)
        {
            case FUNC_EVERY_250_MSECOND:
                SOIL7IN1ReadData();
                break;
            case FUNC_JSON_APPEND:
                SOIL7IN1Show(1);
                break;
#ifdef USE_WEBSERVER
            case FUNC_WEB_SENSOR:
                SOIL7IN1Show(0);
                break;
#endif
        }
    }
    return result;
}

#endif
#endif