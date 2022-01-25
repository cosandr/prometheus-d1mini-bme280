# Push metrics from BME280 sensor to Prometheus' Pushgateway

Intended to be used on battery power, device goes into deep sleep between pushes.

## Metrics

| Metric | Description | Unit |
| - | - | - |
| `bme_up` | Status and metadata | |
| `bme_humidity` | Air humidity. | `%` |
| `bme_temperature` | Air temperature. | `°C` |
| `bme_heat_index` | Apparent air temperature, based on temperature and humidity. | `°C` |
| `bme_dew_point` | Dew point, based on temperature and humidity. | `°C` |

## Requirements

### Hardware

- ESP8266-based board (or some other appropriate Arduino-based board).
    - Tested with "WEMOS D1 Mini".
- BME280 sensor.
    - Tested with [this one from AliExpress](https://www.aliexpress.com/item/1005003622447376).

### Software

- A Pushgateway server
- [PlatformIO](https://platformio.org/)
- [esp8266 library for Arduino](https://github.com/esp8266/Arduino)
- [Adafruit BME280 library](https://github.com/adafruit/Adafruit_BME280_Library)

## Variables

- `URL`Pushgateway server, no trailing slash
- `COMPUTE_HEAT_INDEX` set to 0 to disable heat index compute, will return NaN
- `COMPUTE_DEW_POINT` same but for dew point
- `PUSH_INTERVAL` sets push interval (sleep duration) in seconds
- `USE_DEEP_SLEEP` use deep sleep instead of turning wifi off

## Building

### Hardware

If using deep sleep, you will need to wire D16 to RST so the chip can wake itself up.

### Software

Using PlatformIO (VSCode).

Create .env file and source it
```sh
export WIFI_SSID=""
export WIFI_PASSWORD=""
```

On Windows create `.env.ps1`
```powershell
$env:WIFI_SSID=""
$env:WIFI_PASSWORD=""
```

Either build using the CLI, or export the variables and start VSCode.

Build

```sh
source .env
# On windows use
. .env.ps1
pio run -e bme01
```

Upload (also builds)

```sh
pio run -e bme01 -t upload
```

Monitor

```sh
pio device monitor -e bme01
```

## Credits

[HON95 for his Prometheus exporter](https://github.com/HON95/prometheus-esp8266-dht-exporter)
[homecircuits.eu blog](https://homecircuits.eu/blog/battery-powered-esp8266-iot-logger/)

## License

GNU General Public License version 3 (GPLv3).
