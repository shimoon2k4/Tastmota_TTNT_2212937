#ifdef USE_RS485
#ifdef USE_ULTRA_SONIC_SENSOR

#define XSNS_125 125
#define XRS485_16 16

struct ULTRA_SONIC_t {
    bool valid = false;
    float temperature = 0.0;
    float humidity = 0.0;
    float pressure = 0.0;
    float noise = 0.0;
    float pm25 = 0.0;
    float pm10 = 0.0;
    float lux = 0.0;
    char name[19] = "Ultra Sonic Sensor";
} ULTRA_SONIC;

#define ULTRA_SONIC_ADDRESS_ID           0x15
#define ULTRA_SONIC_FUNCTION_CODE        0x03
#define ULTRA_SONIC_ADDRESS_CHECK    0x0100  // 256
#define ULTRA_SONIC_TIMEOUT              200  // nếu có delay giữa 2 lần đọc

#define ULTRA_SONIC_ADDRESS_HUMIDITY     0x01F4  // 500
#define ULTRA_SONIC_ADDRESS_TEMPERATURE  0x01F5  // 501
#define ULTRA_SONIC_ADDRESS_NOISE        0x01F6  // 502
#define ULTRA_SONIC_ADDRESS_PM25         0x01F7  // 503
#define ULTRA_SONIC_ADDRESS_PM10         0x01F8  // 504
#define ULTRA_SONIC_ADDRESS_PRESSURE     0x01F9  // 505
#define ULTRA_SONIC_ADDRESS_LUX_HI       0x01FA  // 506
#define ULTRA_SONIC_ADDRESS_LUX_LO       0x01FB  // 507


bool ULTRASONICisConnected()
{
    if (!RS485.active) return false;

    // Gửi lệnh đọc 2 thanh ghi từ địa chỉ 0x01F4 (500) – Humidity + Temperature
    RS485.Rs485Modbus->Send(ULTRA_SONIC_ADDRESS_ID, ULTRA_SONIC_FUNCTION_CODE, 0x01F4, 0x02);
    delay(200);
    RS485.Rs485Modbus->ReceiveReady();

    uint8_t buffer[9];
    uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, sizeof(buffer));

    if (error) {
        AddLog(LOG_LEVEL_INFO, PSTR("ULTRA SONIC error %d"), error);
        return false;
    }

    // Kiểm tra ID và Function Code
    if (buffer[0] != ULTRA_SONIC_ADDRESS_ID || buffer[1] != ULTRA_SONIC_FUNCTION_CODE)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("ULTRA SONIC invalid ID or FunctionCode"));
        return false;
    }
    // Nếu đến đây thì coi như cảm biến hoạt động tốt
    return true;
}



void ULTRASONICInit(void)
{
    if (!RS485.active) return;
    ULTRA_SONIC.valid = ULTRASONICisConnected();
    if (ULTRA_SONIC.valid) Rs485SetActiveFound(ULTRA_SONIC_ADDRESS_ID, ULTRA_SONIC.name);
    AddLog(LOG_LEVEL_INFO, ULTRA_SONIC.valid ? PSTR("ULTRA_SONIC sensor connected") : PSTR("ULTRA_SONIC not detected"));
}

void ULTRASONICReadData(void)
{
    if (!ULTRA_SONIC.valid) return;
    if (isWaitingResponse(ULTRA_SONIC_ADDRESS_ID)) return;

    static const struct {
        uint16_t regAddr;
        uint8_t regCount;
        void (*assign)(uint16_t* data);
    } ULTRASONICQueue[] = {
        {500, 1, [](uint16_t* d) { ULTRA_SONIC.humidity = d[0] / 10.0f; }},
        {501, 1, [](uint16_t* d) { ULTRA_SONIC.temperature = ((int16_t)d[0]) / 10.0f; }},
        {502, 1, [](uint16_t* d) { ULTRA_SONIC.noise = d[0] / 10.0f; }},
        {503, 1, [](uint16_t* d) { ULTRA_SONIC.pm25 = d[0] * 1.0f; }},
        {504, 1, [](uint16_t* d) { ULTRA_SONIC.pm10 = d[0] * 1.0f; }},
        {505, 1, [](uint16_t* d) { ULTRA_SONIC.pressure = d[0] / 10.0f; }},
        {506, 2, [](uint16_t* d) {
            ULTRA_SONIC.lux = ((uint32_t)d[0] << 16) | d[1];
        }},
    };

    static uint8_t queueIndex = 0;
    static bool waitingResponse = false;
    static uint32_t lastRequestTime = 0;

    if (!waitingResponse) {
        RS485.Rs485Modbus->Send(
            ULTRA_SONIC_ADDRESS_ID,
            ULTRA_SONIC_FUNCTION_CODE,
            ULTRASONICQueue[queueIndex].regAddr,
            ULTRASONICQueue[queueIndex].regCount
        );
        waitingResponse = true;
        lastRequestTime = millis();
    }
    else if (millis() - lastRequestTime >= ULTRA_SONIC_TIMEOUT) {
        if (RS485.Rs485Modbus->ReceiveReady()) {
            uint8_t buffer[9];  // đủ cho 2 thanh ghi
            uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, sizeof(buffer));

            if (!error && buffer[0] == ULTRA_SONIC_ADDRESS_ID) {
                uint8_t byteCount = buffer[2];
                uint16_t data[2] = {0};

                for (uint8_t i = 0; i < byteCount / 2; ++i) {
                    data[i] = (buffer[3 + i*2] << 8) | buffer[4 + i*2];
                }

                ULTRASONICQueue[queueIndex].assign(data);
                AddLog(LOG_LEVEL_INFO, PSTR(
                    "ULTRA SONIC: Humi=%.1f%% Temp=%.1fC Noise=%.1fdB "
                    "PM2.5=%.0f PM10=%.0f Pressure=%.1fkPa Lux=%lu"),
                    ULTRA_SONIC.humidity,
                    ULTRA_SONIC.temperature,
                    ULTRA_SONIC.noise,
                    ULTRA_SONIC.pm25,
                    ULTRA_SONIC.pm10,
                    ULTRA_SONIC.pressure,
                    ULTRA_SONIC.lux
                );
                
            } else {
                AddLog(LOG_LEVEL_INFO, PSTR("ULTRA_SONIC modbus error %d"), error);
            }

            queueIndex = (queueIndex + 1) % (sizeof(ULTRASONICQueue) / sizeof(ULTRASONICQueue[0]));
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

void ULTRASONICShow(bool json)
{
    if (json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), ULTRA_SONIC.name);
        ResponseAppend_P(PSTR("\"" D_JSON_OUT_TEMP "\":%.1f,"), ULTRA_SONIC.temperature);
        ResponseAppend_P(PSTR("\"" D_JSON_OUT_HUMI "\":%.1f,"), ULTRA_SONIC.humidity);
        ResponseAppend_P(PSTR("\"" D_JSON_OUT_PRESS "\":%.1f,"), ULTRA_SONIC.pressure);
        ResponseAppend_P(PSTR("\"" D_JSON_OUT_NOISE "\":%.1f,"), ULTRA_SONIC.noise);
        ResponseAppend_P(PSTR("\"" D_JSON_OUT_PM25 "\":%.0f,"), ULTRA_SONIC.pm25);
        ResponseAppend_P(PSTR("\"" D_JSON_OUT_PM10 "\":%.0f,"), ULTRA_SONIC.pm10);
        ResponseAppend_P(PSTR("\"" D_JSON_OUT_LUX "\":%.0f"), ULTRA_SONIC.lux);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_OUT_TEMP, ULTRA_SONIC.temperature);
        WSContentSend_PD(HTTP_SNS_OUT_HUMI, ULTRA_SONIC.humidity);
        WSContentSend_PD(HTTP_SNS_OUT_PRESS, ULTRA_SONIC.pressure);
        WSContentSend_PD(HTTP_SNS_OUT_NOISE, ULTRA_SONIC.noise);
        WSContentSend_PD(HTTP_SNS_OUT_PM25, ULTRA_SONIC.pm25);
        WSContentSend_PD(HTTP_SNS_OUT_PM10, ULTRA_SONIC.pm10);
        WSContentSend_PD(HTTP_SNS_OUT_LUX, ULTRA_SONIC.lux);
    }
#endif
}

bool Xsns125(uint32_t function)
{
    if (!Rs485Enabled(XRS485_16)) return false;

    bool result = false;
    if (FUNC_INIT == function)
    {
        ULTRASONICInit();
    }
    else if (ULTRA_SONIC.valid)
    {
        switch (function)
        {
            case FUNC_EVERY_250_MSECOND:
                ULTRASONICReadData();
                break;
            case FUNC_JSON_APPEND:
                ULTRASONICShow(1);
                break;
#ifdef USE_WEBSERVER
            case FUNC_WEB_SENSOR:
                ULTRASONICShow(0);
                break;
#endif
        }
    }
    return result;
}

#endif
#endif
