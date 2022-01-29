#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <base64.h>

#include "config.h"
#include "utils.h"

enum LogLevel {
    DEBUG,
    INFO,
    ERROR,
};

float calculate_dew_point(float temperature, float humidity);
float calculate_heat_index(float temperature, float humidity);
float read_humidity_sensor();
float read_pressure_sensor();
float read_temperature_sensor();
void handle_http_metrics();
void handle_http_not_found();
void handle_http_root();
void log(char const *message, LogLevel level=LogLevel::INFO);
void setup_http_server();
void setup_wifi();

// Adafruit_BME280 bme; // I2C
// Adafruit_BME280 bme(BME_CS); // hardware SPI
Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

ESP8266WebServer http_server(HTTP_SERVER_PORT);
WiFiClient wifiClient;

bool request_handled;

static char const *base_labels = "room=\"" ROOM "\",hostname=\"" WIFI_HOSTNAME "\"";

void loop() {
#if USE_DEEP_SLEEP == 0
    log("Turning off WiFi");
    // Turn wifi off and delay
    WiFi.mode(WIFI_OFF);
    log("Set BME to sleep mode");
    bme.setSampling(bme.MODE_SLEEP);
    log("Waiting...");
    delay(SLEEP_INTERVAL * 1e3);

    // Resume
    bme.setSampling(bme.MODE_NORMAL);
    setup_wifi();

    log("Handling request...", LogLevel::DEBUG);
    while (!request_handled) http_server.handleClient();
    request_handled = false;

#elif USE_DEEP_SLEEP == -1
    http_server.handleClient();
#endif
}

void setup(void) {
    Serial.begin(9600);

    while(!Serial);

    request_handled = false;

    log("Setting up BME280 sensor", LogLevel::DEBUG);

    unsigned status = bme.begin();
    while (!status) {
        log("Waiting for BME280 sensor...", LogLevel::DEBUG);
        delay(100);
    }

    char message[128];
    snprintf(message, 128, "BME SensorID: 0x%x", bme.sensorID());
    log(message, LogLevel::DEBUG);

    snprintf(message, 128, "Setting %d temperature offset", TEMPERATURE_CORRECTION_OFFSET);
    log(message, LogLevel::DEBUG);
    bme.setTemperatureCompensation(float(TEMPERATURE_CORRECTION_OFFSET));
    snprintf(message, 128, "Temperature compensation: %.2f", bme.getTemperatureCompensation());
    log(message);

    setup_wifi();
    snprintf(message, 128, "Prometheus namespace: %s", PROM_NAMESPACE);
    log(message);

    setup_http_server();

    log("Handling request...", LogLevel::DEBUG);
    while (!request_handled) http_server.handleClient();

#if USE_DEEP_SLEEP == 1
    log("Begin deep sleep", LogLevel::DEBUG);

    uint64_t sleep_duration = SLEEP_INTERVAL * 1e6;

    // Don't attempt to sleep more than the max duration
    ESP.deepSleep(min(sleep_duration, ESP.deepSleepMax()));
#endif
}

void setup_http_server() {
    char message[128];
    log("Setting up HTTP server");
    http_server.on("/", HTTPMethod::HTTP_GET, handle_http_root);
    http_server.on(HTTP_METRICS_ENDPOINT, HTTPMethod::HTTP_GET, handle_http_metrics);
    http_server.onNotFound(handle_http_not_found);
    http_server.begin();
    log("HTTP server started", LogLevel::DEBUG);
    snprintf(message, 128, "Metrics endpoint: %s", HTTP_METRICS_ENDPOINT);
    log(message);
}

void handle_http_root() {
    static size_t const BUFSIZE = 512;
    static char const *response_template = "<html>\n"
        "<head><title>ESP8266 BME280 Exporter</title></head>\n"
        "<body>\n"
        "<h1>ESP8266 BME280 Exporter</h1>\n"
        "<p><a href=\"%s\">Metrics</a></p>\n"
        "<h2>More information:</h2>\n"
        "<p><a href=\"https://github.com/cosandr/prometheus-d1mini-bme280\">github.com/cosandr/prometheus-d1mini-bme280</a></p>\n"
        "</body>"
        "</html>";
    char response[BUFSIZE];
    snprintf(response, BUFSIZE, response_template, HTTP_METRICS_ENDPOINT);
    http_server.send(200, "text/html; charset=utf-8", response);
}

void handle_http_metrics() {
    static size_t const BUFSIZE = 1024;
    static char const *up_template =
        "# HELP " PROM_NAMESPACE "_up Metadata about the device.\n"
        "# TYPE " PROM_NAMESPACE "_up gauge\n"
        PROM_NAMESPACE "_up{%s,version=\"%s\",board=\"%s\",sensor=\"%s\",mac=\"%s\"} %d\n";
    static char const *response_template =
        "# HELP " PROM_NAMESPACE "_humidity Air humidity.\n"
        "# TYPE " PROM_NAMESPACE "_humidity gauge\n"
        "# UNIT " PROM_NAMESPACE "_humidity %%\n"
        PROM_NAMESPACE "_humidity{%s} %f\n"
        "# HELP " PROM_NAMESPACE "_temperature Air temperature.\n"
        "# TYPE " PROM_NAMESPACE "_temperature gauge\n"
        "# UNIT " PROM_NAMESPACE "_temperature \u00B0C\n"
        PROM_NAMESPACE "_temperature{%s} %f\n"
        "# HELP " PROM_NAMESPACE "_pressure Air pressure.\n"
        "# TYPE " PROM_NAMESPACE "_pressure gauge\n"
        "# UNIT " PROM_NAMESPACE "_pressure Pa\n"
        PROM_NAMESPACE "_pressure{%s} %f\n"
        "# HELP " PROM_NAMESPACE "_heat_index Apparent air temperature, based on temperature and humidity.\n"
        "# TYPE " PROM_NAMESPACE "_heat_index gauge\n"
        "# UNIT " PROM_NAMESPACE "_heat_index \u00B0C\n"
        PROM_NAMESPACE "_heat_index{%s} %f\n"
        "# HELP " PROM_NAMESPACE "_dew_point Dew point, based on temperature and humidity.\n"
        "# TYPE " PROM_NAMESPACE "_dew_point gauge\n"
        "# UNIT " PROM_NAMESPACE "_dew_point \u00B0C\n"
        PROM_NAMESPACE "_dew_point{%s} %f\n";

    char response[BUFSIZE];
    // Read sensors
    float temperature = read_temperature_sensor();
    float humidity = read_humidity_sensor();
    float pressure = read_pressure_sensor();
    float heat_index = calculate_heat_index(temperature, humidity);
    float dew_point = calculate_dew_point(temperature, humidity);
#if DEBUG_MODE == 1
    char message[128];
    snprintf(message, 128, "T: %f, H: %f, P: %f, HI: %f, DP: %f", temperature, humidity, pressure, heat_index, dew_point);
    log(message, LogLevel::DEBUG);
#endif
    if (isnan(humidity) || isnan(temperature) || isnan(pressure)) {
        snprintf(response, BUFSIZE, up_template, base_labels, VERSION, BOARD_NAME, SENSOR_NAME, WiFi.macAddress().c_str(), 0);
    } else {
        int cx = snprintf(response, BUFSIZE, up_template, base_labels, VERSION, BOARD_NAME, SENSOR_NAME, WiFi.macAddress().c_str(), 1);
        snprintf(response+cx, BUFSIZE-cx, response_template,
            base_labels, humidity,
            base_labels, temperature,
            base_labels, pressure,
            base_labels, heat_index,
            base_labels, dew_point);
    }
    http_server.send(200, "text/plain; charset=utf-8", response);
    request_handled = true;
}

void handle_http_not_found() {
    http_server.send(404, "text/plain; charset=utf-8", "Not found.");
}

float read_temperature_sensor() {
    log("Reading temperature sensor ...", LogLevel::DEBUG);
    float temperature = bme.readTemperature();
    if (isnan(temperature)) {
        log("Failed to read temperature sensor.", LogLevel::ERROR);
    }
    return temperature;
}

float read_humidity_sensor() {
    log("Reading humidity sensor ...", LogLevel::DEBUG);
    float humidity = bme.readHumidity();
    if (!isnan(humidity)) {
        humidity += HUMIDITY_CORRECTION_OFFSET;
    } else {
        log("Failed to read humidity sensor.", LogLevel::ERROR);
    }
    return humidity;
}

float read_pressure_sensor() {
    log("Reading pressure sensor ...", LogLevel::DEBUG);
    float pressure = bme.readPressure();
    if (isnan(pressure)) {
        log("Failed to read pressure sensor.", LogLevel::ERROR);
    }
    return pressure;
}

float calculate_heat_index(float temperature, float humidity) {
    float heat_index = NAN;
    if (COMPUTE_HEAT_INDEX && !isnan(humidity) && !isnan(temperature)) {
        heat_index = computeHeatIndex(temperature, humidity, false);
    }
    return heat_index;
}

float calculate_dew_point(float temperature, float humidity) {
    float dew_point = NAN;
    if (COMPUTE_DEW_POINT && !isnan(humidity) && !isnan(temperature)) {
        dew_point = computeDewPoint(temperature, humidity, false);
    }
    return dew_point;
}

void log(char const *message, LogLevel level) {
    if (DEBUG_MODE == 0 && level == LogLevel::DEBUG) {
        return;
    }
    // Will overflow after a while
    float seconds = millis() / 1000.0;
    char str_level[10];
    switch (level) {
        case DEBUG:
            strcpy(str_level, "DEBUG");
            break;
        case INFO:
            strcpy(str_level, "INFO");
            break;
        case ERROR:
            strcpy(str_level, "ERROR");
            break;
        default:
            break;
    }
    char record[150];
    snprintf(record, 150, "[%10.3f] [%-5s] %s", seconds, str_level, message);
    Serial.println(record);
}

void setup_wifi() {
    char message[128];
    log("Setting up Wi-Fi");
#if DEBUG_MODE == 1
    snprintf(message, 128, "Wi-Fi SSID: %s", WIFI_SSID);
    log(message, LogLevel::DEBUG);
    snprintf(message, 128, "MAC address: %s", WiFi.macAddress().c_str());
    log(message, LogLevel::DEBUG);
    snprintf(message, 128, "Initial hostname: %s", WiFi.hostname().c_str());
    log(message, LogLevel::DEBUG);
#endif

    WiFi.mode(WIFI_STA);

#if WIFI_IPV4_STATIC == true
        log("Using static IPv4 adressing", LogLevel::DEBUG);
        IPAddress static_address(WIFI_IPV4_ADDRESS);
        IPAddress static_subnet(WIFI_IPV4_SUBNET_MASK);
        IPAddress static_gateway(WIFI_IPV4_GATEWAY);
        IPAddress static_dns1(WIFI_IPV4_DNS_1);
        IPAddress static_dns2(WIFI_IPV4_DNS_2);
        if (!WiFi.config(static_address, static_gateway, static_subnet, static_dns1, static_dns2)) {
            log("Failed to configure static addressing", LogLevel::ERROR);
        }
#endif

#ifdef WIFI_HOSTNAME
        log("Requesting hostname: " WIFI_HOSTNAME, LogLevel::DEBUG);
        if (WiFi.hostname(WIFI_HOSTNAME)) {
            log("Hostname changed", LogLevel::DEBUG);
        } else {
            log("Failed to change hostname (too long?)", LogLevel::ERROR);
        }
#endif

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        log("Wi-Fi connection not ready, waiting", LogLevel::DEBUG);
        delay(500);
    }

    log("Wi-Fi connected.");
    snprintf(message, 128, "SSID: %s", WiFi.SSID().c_str());
    log(message);
    snprintf(message, 128, "BSSID: %s", WiFi.BSSIDstr().c_str());
    log(message, LogLevel::DEBUG);
    snprintf(message, 128, "Hostname: %s", WiFi.hostname().c_str());
    log(message, LogLevel::DEBUG);
    snprintf(message, 128, "MAC address: %s", WiFi.macAddress().c_str());
    log(message);
    snprintf(message, 128, "IPv4 address: %s", WiFi.localIP().toString().c_str());
    log(message);
    snprintf(message, 128, "IPv4 subnet mask: %s", WiFi.subnetMask().toString().c_str());
    log(message, LogLevel::DEBUG);
    snprintf(message, 128, "IPv4 gateway: %s", WiFi.gatewayIP().toString().c_str());
    log(message, LogLevel::DEBUG);
    snprintf(message, 128, "Primary DNS server: %s", WiFi.dnsIP(0).toString().c_str());
    log(message, LogLevel::DEBUG);
    snprintf(message, 128, "Secondary DNS server: %s", WiFi.dnsIP(1).toString().c_str());
    log(message, LogLevel::DEBUG);
}
