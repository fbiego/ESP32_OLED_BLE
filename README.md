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

The companion app is available [`here`](https://github.com/fbiego/DT78-App-Android)


![2](image2.jpg?raw=true "2")


## Libraries

Extract the `OLED_I2C` & `ESP32Time` library to your Arduino libraries folder

The `OLED_I2C` library has been modified to include the `flipMode()` function

Download `ESP32Time` library from Arduino Library Manager or [`here`](https://www.arduinolibraries.info/libraries/esp32-time)

