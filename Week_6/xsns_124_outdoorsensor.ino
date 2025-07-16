#ifdef USE_RS485
#ifdef USE_OUTDOOR_ENV_SENSOR

#define XSNS_124 124
#define XRS485_15 15

struct OUTDOOR_ENV_t {
    bool valid = false;
    float temperature = 0.0;
    float humidity = 0.0;
    float pressure = 0.0;
    float noise = 0.0;
    float pm25 = 0.0;
    float pm10 = 0.0;
    float lux = 0.0;
    char name[19] = "Outdoor Env Sensor";
} OUTDOOR_ENV;

#define OUTDOOR_ENV_ADDRESS_ID           0x15
#define OUTDOOR_ENV_FUNCTION_CODE        0x03
#define OUTDOOR_ENV_ADDRESS_CHECK    0x0100  // 256
#define OUTDOOR_ENV_TIMEOUT              200  // nếu có delay giữa 2 lần đọc

#define OUTDOOR_ENV_ADDRESS_HUMIDITY     0x01F4  // 500
#define OUTDOOR_ENV_ADDRESS_TEMPERATURE  0x01F5  // 501
#define OUTDOOR_ENV_ADDRESS_NOISE        0x01F6  // 502
#define OUTDOOR_ENV_ADDRESS_PM25         0x01F7  // 503
#define OUTDOOR_ENV_ADDRESS_PM10         0x01F8  // 504
#define OUTDOOR_ENV_ADDRESS_PRESSURE     0x01F9  // 505
#define OUTDOOR_ENV_ADDRESS_LUX_HI       0x01FA  // 506
#define OUTDOOR_ENV_ADDRESS_LUX_LO       0x01FB  // 507


bool OUTDOORENVisConnected()
{
    if (!RS485.active) return false;

    // Gửi lệnh đọc 2 thanh ghi từ địa chỉ 0x01F4 (500) – Humidity + Temperature
    RS485.Rs485Modbus->Send(OUTDOOR_ENV_ADDRESS_ID, OUTDOOR_ENV_FUNCTION_CODE, 0x01F4, 0x02);
    delay(200);
    RS485.Rs485Modbus->ReceiveReady();

    uint8_t buffer[9];
    uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, sizeof(buffer));

    if (error) {
        AddLog(LOG_LEVEL_INFO, PSTR("OUTDOOR ENV error %d"), error);
        return false;
    }

    // Kiểm tra ID và Function Code
    if (buffer[0] != OUTDOOR_ENV_ADDRESS_ID || buffer[1] != OUTDOOR_ENV_FUNCTION_CODE)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("OUTDOOR ENV invalid ID or FunctionCode"));
        return false;
    }
    // Nếu đến đây thì coi như cảm biến hoạt động tốt
    return true;
}



void OUTDOORENVInit(void)
{
    if (!RS485.active) return;
    OUTDOOR_ENV.valid = OUTDOORENVisConnected();
    if (OUTDOOR_ENV.valid) Rs485SetActiveFound(OUTDOOR_ENV_ADDRESS_ID, OUTDOOR_ENV.name);
    AddLog(LOG_LEVEL_INFO, OUTDOOR_ENV.valid ? PSTR("OUTDOOR_ENV sensor connected") : PSTR("OUTDOOR_ENV not detected"));
}

void OUTDOORENVReadData(void)
{
    if (!OUTDOOR_ENV.valid) return;
    if (isWaitingResponse(OUTDOOR_ENV_ADDRESS_ID)) return;

    static const struct {
        uint16_t regAddr;
        uint8_t regCount;
        void (*assign)(uint16_t* data);
    } OUTDOORENVQueue[] = {
        {500, 1, [](uint16_t* d) { OUTDOOR_ENV.humidity = d[0] / 10.0f; }},
        {501, 1, [](uint16_t* d) { OUTDOOR_ENV.temperature = ((int16_t)d[0]) / 10.0f; }},
        {502, 1, [](uint16_t* d) { OUTDOOR_ENV.noise = d[0] / 10.0f; }},
        {503, 1, [](uint16_t* d) { OUTDOOR_ENV.pm25 = d[0] * 1.0f; }},
        {504, 1, [](uint16_t* d) { OUTDOOR_ENV.pm10 = d[0] * 1.0f; }},
        {505, 1, [](uint16_t* d) { OUTDOOR_ENV.pressure = d[0] / 10.0f; }},
        {506, 2, [](uint16_t* d) {
            OUTDOOR_ENV.lux = ((uint32_t)d[0] << 16) | d[1];
        }},
    };

    static uint8_t queueIndex = 0;
    static bool waitingResponse = false;
    static uint32_t lastRequestTime = 0;

    if (!waitingResponse) {
        RS485.Rs485Modbus->Send(
            OUTDOOR_ENV_ADDRESS_ID,
            OUTDOOR_ENV_FUNCTION_CODE,
            OUTDOORENVQueue[queueIndex].regAddr,
            OUTDOORENVQueue[queueIndex].regCount
        );
        waitingResponse = true;
        lastRequestTime = millis();
    }
    else if (millis() - lastRequestTime >= OUTDOOR_ENV_TIMEOUT) {
        if (RS485.Rs485Modbus->ReceiveReady()) {
            uint8_t buffer[9];  // đủ cho 2 thanh ghi
            uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, sizeof(buffer));

            if (!error && buffer[0] == OUTDOOR_ENV_ADDRESS_ID) {
                uint8_t byteCount = buffer[2];
                uint16_t data[2] = {0};

                for (uint8_t i = 0; i < byteCount / 2; ++i) {
                    data[i] = (buffer[3 + i*2] << 8) | buffer[4 + i*2];
                }

                OUTDOORENVQueue[queueIndex].assign(data);
                AddLog(LOG_LEVEL_INFO, PSTR(
                    "OUTDOOR ENV: Humi=%.1f%% Temp=%.1fC Noise=%.1fdB "
                    "PM2.5=%.0f PM10=%.0f Pressure=%.1fkPa Lux=%lu"),
                    OUTDOOR_ENV.humidity,
                    OUTDOOR_ENV.temperature,
                    OUTDOOR_ENV.noise,
                    OUTDOOR_ENV.pm25,
                    OUTDOOR_ENV.pm10,
                    OUTDOOR_ENV.pressure,
                    OUTDOOR_ENV.lux
                );
                
            } else {
                AddLog(LOG_LEVEL_INFO, PSTR("OUTDOOR_ENV modbus error %d"), error);
            }

            queueIndex = (queueIndex + 1) % (sizeof(OUTDOORENVQueue) / sizeof(OUTDOORENVQueue[0]));
            waitingResponse = false;
        }
    }

    
}



#define D_JSON_OUT_TEMP    "OUT_TEMP"
#define D_JSON_OUT_HUMI    "OUT_HUMI"
#define D_JSON_OUT_PRESS   "OUT_PRESSURE"
#define D_JSON_OUT_NOISE   "OUT_NOISE"
#define D_JSON_OUT_PM25    "OUT_PM25"
#define D_JSON_OUT_PM10    "OUT_PM10"
#define D_JSON_OUT_LUX     "OUT_LUX"

const char HTTP_SNS_OUT_TEMP[]  PROGMEM = "{s}Temperature {m}%.1f°C";
const char HTTP_SNS_OUT_HUMI[]  PROGMEM = "{s}Humidity {m}%.1f%%";
const char HTTP_SNS_OUT_PRESS[] PROGMEM = "{s}Pressure {m}%.1f kPa";
const char HTTP_SNS_OUT_NOISE[] PROGMEM = "{s}Noise {m}%.1f dB";
const char HTTP_SNS_OUT_PM25[]  PROGMEM = "{s}PM2.5 {m}%.0f µg/m³";
const char HTTP_SNS_OUT_PM10[]  PROGMEM = "{s}PM10 {m}%.0f µg/m³";
const char HTTP_SNS_OUT_LUX[]   PROGMEM = "{s}Lux {m}%.0f lx";

void OUTDOORENVShow(bool json)
{
    if (json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), OUTDOOR_ENV.name);
        ResponseAppend_P(PSTR("\"" D_JSON_OUT_TEMP "\":%.1f,"), OUTDOOR_ENV.temperature);
        ResponseAppend_P(PSTR("\"" D_JSON_OUT_HUMI "\":%.1f,"), OUTDOOR_ENV.humidity);
        ResponseAppend_P(PSTR("\"" D_JSON_OUT_PRESS "\":%.1f,"), OUTDOOR_ENV.pressure);
        ResponseAppend_P(PSTR("\"" D_JSON_OUT_NOISE "\":%.1f,"), OUTDOOR_ENV.noise);
        ResponseAppend_P(PSTR("\"" D_JSON_OUT_PM25 "\":%.0f,"), OUTDOOR_ENV.pm25);
        ResponseAppend_P(PSTR("\"" D_JSON_OUT_PM10 "\":%.0f,"), OUTDOOR_ENV.pm10);
        ResponseAppend_P(PSTR("\"" D_JSON_OUT_LUX "\":%.0f"), OUTDOOR_ENV.lux);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_OUT_TEMP, OUTDOOR_ENV.temperature);
        WSContentSend_PD(HTTP_SNS_OUT_HUMI, OUTDOOR_ENV.humidity);
        WSContentSend_PD(HTTP_SNS_OUT_PRESS, OUTDOOR_ENV.pressure);
        WSContentSend_PD(HTTP_SNS_OUT_NOISE, OUTDOOR_ENV.noise);
        WSContentSend_PD(HTTP_SNS_OUT_PM25, OUTDOOR_ENV.pm25);
        WSContentSend_PD(HTTP_SNS_OUT_PM10, OUTDOOR_ENV.pm10);
        WSContentSend_PD(HTTP_SNS_OUT_LUX, OUTDOOR_ENV.lux);
    }
#endif
}

bool Xsns124(uint32_t function)
{
    if (!Rs485Enabled(XRS485_15)) return false;

    bool result = false;
    if (FUNC_INIT == function)
    {
        OUTDOORENVInit();
    }
    else if (OUTDOOR_ENV.valid)
    {
        switch (function)
        {
            case FUNC_EVERY_250_MSECOND:
                OUTDOORENVReadData();
                break;
            case FUNC_JSON_APPEND:
                OUTDOORENVShow(1);
                break;
#ifdef USE_WEBSERVER
            case FUNC_WEB_SENSOR:
                OUTDOORENVShow(0);
                break;
#endif
        }
    }
    return result;
}

#endif
#endif
