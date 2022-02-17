#pragma once

// HTTP server port
#define HTTP_SERVER_PORT 80
// HTTP metrics endpoint
#define HTTP_METRICS_ENDPOINT "/metrics"

// GPIO 16 must be wired to RST
// #define USE_DEEP_SLEEP 0

#define COMPUTE_HEAT_INDEX 1
#define COMPUTE_DEW_POINT 1

#define VERSION "1.0.0"
// Debug mode is enabled if not zero
#define DEBUG_MODE 0
// Room name, defined in platformio.ini
// #define ROOM ""
// Board name
#define BOARD_NAME "d1_mini"
#define SENSOR_NAME "bme280"

// 1 - I2C
// 2 - SPI
// 3 - Software SPI
#define BME_MODE 1

#define BME_SCK 2
#define BME_MISO 5
#define BME_MOSI 0
#define BME_CS 4

// Temperature offset in degrees Celsius
#define TEMPERATURE_CORRECTION_OFFSET 0
// Humidity offset in percent
#define HUMIDITY_CORRECTION_OFFSET 0
// How long to sleep between requests, in seconds
#define SLEEP_INTERVAL 30
// Wi-Fi SSID (required)
// #define WIFI_SSID ""
// Wi-Fi password (required)
// #define WIFI_PASSWORD ""
// Hostname for DHCP DDNS, overrides the default (uncomment to enable)
// #define WIFI_HOSTNAME ""
// Use static IPv4 addressing, disable for DHCPv4
#define WIFI_IPV4_STATIC true
// Static IPv4 address
// set in env
#define WIFI_IPV4_ADDRESS 10, 0, 50, WIFI_NUM
// Static IPv4 gateway address
#define WIFI_IPV4_GATEWAY 10, 0, 50, 1
// Static IPv4 subnet mask
#define WIFI_IPV4_SUBNET_MASK 255, 255, 255, 0
// Static IPv4 primary DNS server
#define WIFI_IPV4_DNS_1 10, 0, 50, 1
// Static IPv4 secondary DNS server
#define WIFI_IPV4_DNS_2 1, 1, 1, 1
// Prometheus namespace, aka metric prefix
#define PROM_NAMESPACE "iot"
