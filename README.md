# ESP32_OLED_BLE
Receive Android notifications on ESP32 with an OLED screen via Bluetooth LE. 

Also functions as a clock, the time is set automatically via BLE when connected to the app.

## Videos
+ [`ESP32 OLED notifications`](https://youtu.be/205QgAjmryA)
+ [`How to connect`](https://youtu.be/4o1O2qxbPlw)

## Features
+ Receive notifications
+ Clock display (12 & 24hr mode)
+ Rotate and/or flip OLED display
+ Screen & Notification timeout settings

![1](image1.jpg?raw=true "1")

## Android app

Chronos

<a href='https://chronos.ke/app?id=esp32'><img alt='Download Chronos' height="100px" src='https://chronos.ke/img/chronos.png'/></a>

### Setup
- Go to the watch tab and click on the Watches button
- Pair new watch and select the ESP32
- To Flip the display toggle Units (Metric, Imperial)
- To Rotate the display toggle Temperature (Celsius, Farhenheit)


## Libraries

Download [`ChronosESP32`](https://www.arduinolibraries.info/libraries/chronos-esp32) library from Arduino Library Manager or [`ChronosESP32`](https://github.com/fbiego/chronos-esp32)

Extract the `OLED_I2C` & `ChronosESP32` library to your Arduino libraries folder

The `OLED_I2C` library has been modified to include the `flipMode()` function



