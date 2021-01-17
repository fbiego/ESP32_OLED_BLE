/*
 * MIT License
 * 
 * Copyright (c) 2021 Felix Biego
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <OLED_I2C.h>
#include "time.h"
#include <sys/time.h>
#include "graphics.h"

#define BUILTINLED 2

uint8_t type[] = {0xAB, 0x00, 0x11, 0xFF, 0x92, 0xC0, 0x01, 0x01, 0x38, 0x81, 0x10, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xA2, 0x00, 0x80};  // id for DT78 app
String days[] = {"Sun","Mon","Tue","Wed","Thur","Fri","Sat"};
String months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

#define SERVICE_UUID              "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_RX    "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_TX    "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

static BLECharacteristic* pCharacteristicTX;
static BLECharacteristic* pCharacteristicRX;

OLED  myOLED(18, 19); //(SDA, SCL)

extern uint8_t SmallFont[], MediumNumbers[];

static bool deviceConnected = false;
static int id = 0;
long timeout = 10000, timer = 0, scrTimer = 0;
bool rotate = false, flip = false, hr24 = true, notify = true, screenOff = false, scrOff = false, b1;
int scroll = 0, bat = 0, lines = 0, msglen = 0;
char msg[126];
String msg0, msg1, msg2, msg3, msg4, msg5;


class MyServerCallbacks: public BLEServerCallbacks{
  void onConnect(BLEServer* pServer){
    deviceConnected = true;
    
  }
  void onDisconnect(BLEServer* pServer){
    deviceConnected = false;
    id = 0;
  }
};

class MyCallbacks: public BLECharacteristicCallbacks {

  void onStatus(BLECharacteristic* pCharacteristic, Status s, uint32_t code){
    Serial.print("Status ");
    Serial.print(s);
    Serial.print(" on characteristic ");
    Serial.print(pCharacteristic->getUUID().toString().c_str());
    Serial.print(" with code ");
    Serial.println(code);
  }

  void onNotify(BLECharacteristic *pCharacteristic){
    uint8_t* pData;
    std::string value = pCharacteristic->getValue();
    int len = value.length();
    pData = pCharacteristic->getData();
    if (pData != NULL){
      Serial.print("Notify callback for characteristic ");
      Serial.print(pCharacteristic->getUUID().toString().c_str());
      Serial.print(" of data length ");
      Serial.println(len);
      Serial.print("TX  ");
      for (int i = 0; i < len; i++){
        Serial.printf("%02X ", pData[i]);
      }
      Serial.println();
    }
  }

  void onWrite(BLECharacteristic *pCharacteristic){
    uint8_t* pData;
    std::string value = pCharacteristic->getValue();
    int len = value.length();
    pData = pCharacteristic->getData();
    if (pData != NULL){
      Serial.print("Write callback for characteristic ");
      Serial.print(pCharacteristic->getUUID().toString().c_str());
      Serial.print(" of data length ");
      Serial.println(len);
      Serial.print("RX  ");
      for (int i = 0; i < len; i++){
        Serial.printf("%02X ", pData[i]);
      }
      Serial.println();

      if (pData[0] == 0xAB){
        switch (pData[4]){
          case 0x93:
            setTime(pData[7]*256+pData[8], pData[9], pData[10], pData[11], pData[12], pData[13]);
          break;
          case 0x7C:
            hr24 = pData[6] == 0;
          break;
          case 0x78:
            rotate = pData[6] == 1;
          break;
          case 0x74:
            flip = pData[10] == 1;
          break;
          case 0x7B:
            if (pData[6] >= 5 && pData[6] <=30){
              timeout = pData[6]*1000;
            }
            
          break;
          case 0x23:
            screenOff = pData[6] == 1;
          break;
          case 0x91:
            bat = pData[7];
          break;
          case 0x72:
            timer = millis();
            msglen = pData[2]-5;
            lines = ceil(float(msglen)/21);
            scroll = 0;
            msg[msglen] = 0;
            scrOff = false;
            if (pData[6] == 1) {
              //call
              timer = millis()+15000;
              for (int x = 0; x < len; x++){
                msg[x] = char(pData[x+8]);
              }
            } else if (pData[6] == 2){
              //cancel call
              timer = millis()-timeout;
              scrOff = true;
            } else {
              //notification
              for (int x = 0; x < len; x++){
                msg[x] = char(pData[x+8]);
              }
              
            }
          break;
        }
        
      } else {
        switch (pData[0]){
          case 0:
            for (int x = 0; x < len-1; x++){
              msg[x+12] = char(pData[x+1]);
            }
          break;
          case 1:
            for (int x = 0; x < len-1; x++){
              msg[x+31] = char(pData[x+1]);
            }
          break;
          case 2:
            for (int x = 0; x < len-1; x++){
              msg[x+50] = char(pData[x+1]);
            }
          break;
          case 3:
            for (int x = 0; x < len-1; x++){
              msg[x+69] = char(pData[x+1]);
            }
          break;
          case 4:
            for (int x = 0; x < len-1; x++){
              msg[x+88] = char(pData[x+1]);
            }
          break;
          case 5:
            for (int x = 0; x < len-1; x++){
              msg[x+107] = char(pData[x+1]);
            }
          break;
        }
      }
      
      
    }
      
  }

  void setTime(int yr, int mt, int dy, int hr, int mn, int sc){
    struct tm t = {0};  // Initalize to all 0's
    t.tm_year = yr-1900;  // This is year-1900, so 121 = 2021
    t.tm_mon = mt-1;
    t.tm_mday = dy;
    t.tm_hour = hr;
    t.tm_min = mn;
    t.tm_sec = sc;
    time_t timeSinceEpoch = mktime(&t);
    struct timeval tv;
    tv.tv_sec = timeSinceEpoch;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
  }
  
};

void initBLE(){
  BLEDevice::init("Esp32 Watch");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristicTX = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY );
  pCharacteristicRX = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pCharacteristicRX->setCallbacks(new MyCallbacks());
  pCharacteristicTX->setCallbacks(new MyCallbacks());
  pCharacteristicTX->addDescriptor(new BLE2902());
  pCharacteristicTX->setNotifyProperty(true);
  pService->start();

  
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID); 
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  pinMode(BUILTINLED, OUTPUT);

  if(!myOLED.begin(SSD1306_128X32))
    while(1);   // In case the library failed to allocate enough RAM for the display buffer...
    
  myOLED.setFont(SmallFont);
  initBLE();

  struct timeval tv;
  tv.tv_sec = 1609459200;   //set time to 1st Jan 2021
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);
}

void loop() {

  button(digitalRead(4) == HIGH);   // input pin
  myOLED.clrScr();
  
  myOLED.flipMode(flip^rotate);
  myOLED.rotateDisplay(rotate);
  myOLED.sleepMode(scrOff & screenOff);
  
  if (deviceConnected){
    digitalWrite(BUILTINLED, HIGH);
    
    if (id < 512){
      if (id >= 510){
        pCharacteristicTX->setValue(type, 20);
        pCharacteristicTX->notify();
        delay(10);
      }
      id++;
    }
  } else {
    digitalWrite(BUILTINLED, LOW);
  }

  

  long cur = millis();
  if (cur < timer+timeout) {
    scrTimer = cur;
    long rem = (timer+timeout-cur);
    if (lines > 3 && rem < timeout-(timeout/3) && rem > (timeout/3)){
      scroll = map(rem, timeout-(timeout/3), (timeout/3), 0, (lines-3)*11); //scroll if longer than 3 lines
    }
    showNotification();   // show notification
  } else {
    if (!scrOff && screenOff && cur > scrTimer+timeout){
      scrOff = true;
    }
    
    printLocalTime();   // display time
    if (scrOff & screenOff){
      myOLED.clrScr();
    }
  }
  
  myOLED.update();
  
}

void showNotification(){
  myOLED.setFont(SmallFont);
  String s = String(msg);
  copyMsg(s);
  if (s.length() == 0){
    msg0 = "No messsage";
  }
  myOLED.print(msg0, LEFT, 1 - scroll);
  myOLED.print(msg1, LEFT, 12 - scroll);
  myOLED.print(msg2, LEFT, 23 - scroll);
  myOLED.print(msg3, LEFT, 34 - scroll);
  myOLED.print(msg4, LEFT, 45 - scroll);
  myOLED.print(msg5, LEFT, 56 - scroll);
  
 
}

void printLocalTime() {
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  
  int hr = timeinfo.tm_hour;
  int mn = timeinfo.tm_min;
  int sc = timeinfo.tm_sec;
  int dy = timeinfo.tm_mday;
  int mt = timeinfo.tm_mon;
  int yr = timeinfo.tm_year;
  int wkd = timeinfo.tm_wday;
  if (!hr24){
    if (hr > 12){
      hr = 12 - (24 - hr);
      myOLED.print("PM", RIGHT, 10);
    }  else {
      myOLED.print("AM", RIGHT, 10);
    }
  }
  String tim = (hr<10 ? "0" : "")+String(hr)+":" +(mn<10 ? "0" : "")+ mn + ":"+(sc<10 ? "0" : "")+sc;
  String dt = days[wkd]+ " "+String(dy<10 ? "0" : "")+String(dy)+" "+months[mt];

  myOLED.setFont(MediumNumbers);
  myOLED.print(tim, CENTER, 0);
  myOLED.setFont(SmallFont);
  myOLED.print(dt, CENTER, 21);
  
  if (deviceConnected){
    myOLED.drawBitmap(0, 15, bluetooth, 16, 16);
    myOLED.drawRect(110,23,127,30);
    myOLED.drawRectFill(108,25,110,28);
    myOLED.drawRectFill(map(bat, 0, 100, 127, 110),23,127,30);
  }
  
  
}

void copyMsg(String ms){
  switch (lines){
    case 1:
      msg0 = ms.substring(0, msglen);
      msg1 = "";
      msg2 = "";
      msg3 = "";
      msg4 = "";
      msg5 = "";
    break;
    case 2:
      msg0 = ms.substring(0, 21);
      msg1 = ms.substring(21, msglen);
      msg2 = "";
      msg3 = "";
      msg4 = "";
      msg5 = "";
    break;
    case 3:
      msg0 = ms.substring(0, 21);
      msg1 = ms.substring(21, 42);
      msg2 = ms.substring(42, msglen);
      msg3 = "";
      msg4 = "";
      msg5 = "";
    break;
    case 4:
      msg0 = ms.substring(0, 21);
      msg1 = ms.substring(21, 42);
      msg2 = ms.substring(42, 63);
      msg3 = ms.substring(63, msglen);
      msg4 = "";
      msg5 = "";
    break;
    case 5:
      msg0 = ms.substring(0, 21);
      msg1 = ms.substring(21, 42);
      msg2 = ms.substring(42, 63);
      msg3 = ms.substring(63, 84);
      msg4 = ms.substring(84, msglen);
      msg5 = "";
    break;
    case 6:
      msg0 = ms.substring(0, 21);
      msg1 = ms.substring(21, 42);
      msg2 = ms.substring(42, 63);
      msg3 = ms.substring(63, 84);
      msg4 = ms.substring(84, 105);
      msg5 = ms.substring(105, msglen);
    break;
  }
}

void button(bool b){
  if(b){
    if (!b1){
      // button debounce, code to be executed between this and the next comment

      long current = millis();
      if (scrOff){
        scrOff = false;
        scrTimer = current;
      } else {
        if (current < timer+timeout){
          timer = current-timeout;
        } else {
          timer = current;
        }
        scroll = 0;
        scrOff = false;
      }

      
      // end button debounce
      b1 = true;
    }
  } else {
    b1 = false;
  }
}


  
