#ifdef USE_RS485
#ifdef USE_WIND_RAIN_SENSOR

#define XSNS_126 126
#define XRS485_18 18

struct WeatherSensor_t {
    bool valid = false;
    uint16_t wind_dir = 0;
    float wind_speed = 0.0;
    float rainfall = 0.0;
    char name[16] = "Weather Sensor";
} WeatherSensor;

#define SENSOR_ADDRESS_ID     0x01
#define SENSOR_FUNCTION_CODE  0x03
#define SENSOR_REG_START      0x0000
#define SENSOR_REG_COUNT      3
#define SENSOR_TIMEOUT        300

bool WINDRAINisConnected()
{
    if (!RS485.active) return false;

    RS485.Rs485Modbus->Send(SENSOR_ADDRESS_ID, SENSOR_FUNCTION_CODE, SENSOR_REG_START, SENSOR_REG_COUNT);
    delay(SENSOR_TIMEOUT);
    RS485.Rs485Modbus->ReceiveReady();

    uint8_t buffer[11];
    uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, sizeof(buffer));

    if (error) {
        AddLog(LOG_LEVEL_INFO, PSTR("WTHR SENSOR error %d"), error);
        return false;
    }

    if (buffer[0] != SENSOR_ADDRESS_ID || buffer[1] != SENSOR_FUNCTION_CODE || buffer[2] != 6) {
        AddLog(LOG_LEVEL_INFO, PSTR("WTHR invalid response: ID=%02X FUNC=%02X BYTE_COUNT=%02X"), buffer[0], buffer[1], buffer[2]);
        return false;
    }

    return true;
}

void WINDRAINInit(void)
{
    if (!RS485.active) return;
    WeatherSensor.valid = WINDRAINisConnected();
    if (WeatherSensor.valid) Rs485SetActiveFound(SENSOR_ADDRESS_ID, WeatherSensor.name);
    AddLog(LOG_LEVEL_INFO, WeatherSensor.valid ? PSTR("WTHR sensor connected") : PSTR("WTHR sensor not detected"));
}

void WINDRAINReadData()
{
    if (!WeatherSensor.valid) return;
    if (isWaitingResponse(SENSOR_ADDRESS_ID)) return;

    RS485.Rs485Modbus->Send(SENSOR_ADDRESS_ID, SENSOR_FUNCTION_CODE, SENSOR_REG_START, SENSOR_REG_COUNT);
    delay(SENSOR_TIMEOUT);
    if (!RS485.Rs485Modbus->ReceiveReady()) return;

    uint8_t buffer[11];
    uint8_t error = RS485.Rs485Modbus->ReceiveBuffer(buffer, sizeof(buffer));

    if (!error && buffer[0] == SENSOR_ADDRESS_ID && buffer[1] == SENSOR_FUNCTION_CODE && buffer[2] == 6) {
        uint16_t raw_dir = (buffer[3] << 8) | buffer[4];
        uint16_t raw_speed = (buffer[5] << 8) | buffer[6];
        uint16_t raw_rain = (buffer[7] << 8) | buffer[8];

        WeatherSensor.wind_dir = raw_dir;
        WeatherSensor.wind_speed = raw_speed / 10.0f;
        WeatherSensor.rainfall = raw_rain / 10.0f;

        AddLog(LOG_LEVEL_INFO, PSTR("WTHR RAW: Dir=%d°, Speed=%.1f m/s, Rain=%.1f mm"),
               WeatherSensor.wind_dir, WeatherSensor.wind_speed, WeatherSensor.rainfall);
    } else {
        AddLog(LOG_LEVEL_INFO, PSTR("WTHR read error %d"), error);
    }
}

#define D_JSON_WINDDIR     "WindDir"
#define D_JSON_WINDSPD     "WindSpeed"
#define D_JSON_RAINFALL    "Rain"

const char HTTP_SNS_WTHR[] PROGMEM = "{s}Wind Direction: %d° | Wind Speed %.1f m/s | Rain fall: %.1f mm";

void WINDRAINShow(bool json)
{
    if (!WeatherSensor.valid) return;

    if (json)
    {
        ResponseAppend_P(PSTR(",\"%s\":{"), WeatherSensor.name);
        ResponseAppend_P(PSTR("\"" D_JSON_WINDDIR "\":%d,"), WeatherSensor.wind_dir);
        ResponseAppend_P(PSTR("\"" D_JSON_WINDSPD "\":%.1f,"), WeatherSensor.wind_speed);
        ResponseAppend_P(PSTR("\"" D_JSON_RAINFALL "\":%.1f"), WeatherSensor.rainfall);
        ResponseJsonEnd();
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_WTHR, WeatherSensor.wind_dir, WeatherSensor.wind_speed, WeatherSensor.rainfall);
    }
#endif
}

bool Xsns126(uint32_t function)
{
    if (!Rs485Enabled(XRS485_18)) return false;

    bool result = false;
    if (FUNC_INIT == function)
    {
        WINDRAINInit();
    }
    else if (WeatherSensor.valid)
    {
        switch (function)
        {
            case FUNC_EVERY_250_MSECOND:
                WINDRAINReadData();
                break;
            case FUNC_JSON_APPEND:
                WINDRAINShow(true);
                break;
#ifdef USE_WEBSERVER
            case FUNC_WEB_SENSOR:
                WINDRAINShow(false);
                break;
#endif
        }
    }
    return result;
}

#endif
#endif
