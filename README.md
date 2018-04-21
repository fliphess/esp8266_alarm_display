# esp8266_alarm_display

esp8266 arduino sketch to disable home assistant alarm using an RFID token and passcodes.

This little tool consists of an arduino sketch to flash onto your nodemcu and a python server component that listens to MQTT requests and responds if the UID is recognized and the alarm should be disabled.

I use it on a nodemcu combined with a `MPC23017` GPIO expansion board to add the additional pins for my 4x3 matrix keypad and a `MFRC522` RFID tag reader.

The authentication part is done using an mqtt listener server written in python that looks up the UID of the RFID token and the passcode in it's yaml settings configuration.
At the moment the alarmcode and the uid are send in plaintext to an MQTT topic. For me this is not secure enought for a depending security system,
so I plan to add some psk encryption or use mqtt tls to prevent the uid and passcode to be send in plain text over the WIFI soon, but I'm still searching for a lightweight library that runs smoothly on the nodemcu.

## Setup

### Connect OLED

| OLED Pin | ESP8266 Pin |
| -----    | -----       |
| `GND`    | `GND`       |
| `VDO`    | `3v3`       |
| `SCK`    | `D1`        |
| `SDA`    | `D2`        |


### Connect MPC23017 GPIO Expander

| MPC23017 Pin  | ESP8266 Pin |
| -----         | -----       |
| `GND`         | `GND`       |
| `VCC`         | `3v3`       |
| `SCL`         | `D1`        |
| `SDA`         | `D2`        |


### Connect MFRC522

| RFID Pin | ESP8266 Pin |
| -----    | -----       |
| `SDA`    | `D4`        |
| `SCK`    | `D5`        |
| `MOSI`   | `D7`        |
| `MISO`   | `D6`        |
| `RQ`     | `N/C`       |
| `GND`    | `GND`       |
| `RST`    | `D3`        |
| `3V3`    | `3v3`       |


### Connect Keypad

| MPC23017 Pin  | keypad Pin |
| -----         | -----      |
| `PA1`         | `1`        |
| `PA2`         | `2`        |
| `PA3`         | `3`        |
| `PA4`         | `4`        |
| `PA5`         | `5`        |
| `PA6`         | `6`        |
| `PA7`         | `7`        |


## TODO

- Use PSK encryption to send the uid to the server and back to the nodemcu
