/*
   MIT License

   Copyright (c) 2021 Felix Biego

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#include <OLED_I2C.h>
#include <ChronosESP32.h>
#include <WS2812FX.h>
#include "graphics.h"

#define LED_COUNT 1
#define LED_PIN 27
#define BUILTINLED 2
#define BUTTON_PIN 0

OLED myOLED(25, 26); //(SDA, SCL)
ChronosESP32 watch("ESP32 Watch");
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

ChronosTimer notify;

extern uint8_t SmallFont[], MediumNumbers[];

static bool deviceConnected = false;
static int id = 0;
long timeout = 10000, timer = 0, scrTimer = 0;
bool rotate = false, flip = false, hr24 = true, screenOff = false, scrOff = false, b1, call = false;
int scroll = 0, bat = 0, lines = 0, msglen = 0, msgIndex = 0;
char msg[126];
String msg0, msg1, msg2, msg3, msg4, msg5;

volatile bool buttonPressed = false;

void IRAM_ATTR buttonISR()
{
  buttonPressed = true;
}

uint32_t appColor(int app)
{
  switch (app)
  {
  case 0x03:
    return GREEN; //"Message";
  case 0x04:
    return CYAN; //"Mail";
  case 0x07:
    return BLUE; //"Tencent";
  case 0x08:
    return BLUE; //"Skype";
  case 0x09:
    return GREEN; //"Wechat";
  case 0x0A:
    return GREEN; //"WhatsApp";
  case 0x0B:
    return RED; // "Gmail";
  case 0x0E:
    return YELLOW; //"Line";
  case 0x0F:
    return CYAN; //"Twitter";
  case 0x10:
    return BLUE; //"Facebook";
  case 0x11:
    return BLUE; // "Messenger";
  case 0x12:
    return RED; //"Instagram";
  case 0x13:
    return RED; //"Weibo";
  case 0x14:
    return YELLOW; //"KakaoTalk";
  case 0x16:
    return PURPLE; //"Viber";
  case 0x17:
    return BLUE; //"Vkontakte";
  case 0x18:
    return CYAN; //"Telegram";
  case 0x1B:
    return BLUE; //"DingTalk";
  case 0x20:
    return GREEN; // "WhatsApp Business";
  case 0x22:
    return WHITE; //"WearFit Pro";
  case 0xC0:
    return PINK; //"ChronosESP32";
  default:
    return GREEN; //"Message";
  }
  return GREEN;
}

void notificationCallback(Notification notification)
{
  Serial.print("Notification received at ");
  Serial.println(notification.time);
  Serial.print("From: ");
  Serial.print(notification.app);
  Serial.print("\tIcon: ");
  Serial.println(notification.icon);
  Serial.println(notification.message);

  copyMsg(notification.message);
  msgIndex = 0;

  notify.time = millis();
  notify.active = true;
  ws2812fx.setMode(FX_MODE_BLINK);
  ws2812fx.setColor(appColor(notification.icon));
  ws2812fx.setBrightness(100);
  ws2812fx.setSpeed(500);
}

void ringerCallback(String caller, bool state)
{
  call = state;
  if (state)
  {
    Serial.print("Ringer: Incoming call from ");
    Serial.println(caller);
    msg0 = caller;

    ws2812fx.setMode(FX_MODE_TWINKLE_FADE_RANDOM);
    ws2812fx.setBrightness(200);
    ws2812fx.setSpeed(200);
  }
  else
  {
    Serial.println("Ringer dismissed");
    ws2812fx.setBrightness(0);
  }
}

void rawCallback(uint8_t *data, int length)
{
  Serial.println("Received Raw");
  for (int i = 0; i < length; i++)
  {
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();
}

void dataCallback(uint8_t *data, int length)
{
  Serial.println("Received Data");
  for (int i = 0; i < length; i++)
  {
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();
}

void configCallback(Config config, uint32_t a, uint32_t b)
{
  switch (config)
  {
  case CF_APP:
    // state is saved internally
    Serial.print("App Version Code [");
    Serial.print(a); // int code = watch.getAppCode();
    Serial.print("] Name -> ");
    Serial.println(watch.getAppVersion());
    break;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Watch");
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUILTINLED, OUTPUT);

  if (!myOLED.begin(SSD1306_128X64))
    while (1)
      ; // In case the library failed to allocate enough RAM for the display buffer...

  myOLED.setFont(SmallFont);

  watch.setConfigurationCallback(configCallback);
  watch.setNotificationCallback(notificationCallback);
  watch.setRingerCallback(ringerCallback);
  //  watch.setDataCallback(dataCallback);
  //  watch.setRawDataCallback(rawCallback);
  watch.begin();

  watch.set24Hour(true);
  watch.setBattery(80);

  ws2812fx.init();
  ws2812fx.setBrightness(0);
  ws2812fx.setSpeed(500);
  ws2812fx.setColor(0x007BFF);
  ws2812fx.setMode(FX_MODE_STATIC);
  ws2812fx.start();

  attachInterrupt(BUTTON_PIN, buttonISR, FALLING);
}

void loop()
{

  watch.loop();
  ws2812fx.service();

  if (buttonPressed)
  {
    buttonPressed = false;

    int c = watch.getNotificationCount();
    if (c == 0)
    {
      copyMsg("No notifications");
    }
    else
    {
      copyMsg(watch.getNotificationAt(msgIndex).message);
    }

    if (notify.active)
    {
      // notify.active = false;
      ws2812fx.setBrightness(0);
      notify.time = millis();
      if (c == msgIndex)
      {
        notify.active = false;
        msgIndex = 0;
      }
      else
      {
        msgIndex++;
      }
    }
    else
    {
      notify.time = millis();
      notify.active = true;
    }
  }
  myOLED.clrScr();

  digitalWrite(BUILTINLED, watch.isConnected());

  if (call)
  {
    myOLED.print("Incoming Call", CENTER, 12);
    myOLED.print(msg0, CENTER, 34);
  }
  else
  {
    if (notify.active)
    {
      if (notify.time + notify.duration < millis())
      {
        // timer end
        notify.active = false;
        msgIndex = 0;
        ws2812fx.setBrightness(0);
      }
      showNotification();
    }
    else
    {
      printLocalTime(); // display time
    }
  }

  myOLED.update();
}

void showNotification()
{
  myOLED.setFont(SmallFont);
  myOLED.print(msg0, LEFT, 1 - scroll);
  myOLED.print(msg1, LEFT, 12 - scroll);
  myOLED.print(msg2, LEFT, 23 - scroll);
  myOLED.print(msg3, LEFT, 34 - scroll);
  myOLED.print(msg4, LEFT, 45 - scroll);
  myOLED.print(msg5, LEFT, 56 - scroll);
}

void printLocalTime()
{
  myOLED.print(watch.getAmPmC(false), RIGHT, 10);

  myOLED.setFont(MediumNumbers);
  myOLED.print(watch.getHourZ() + watch.getTime(":%M:%S"), CENTER, 0);
  myOLED.setFont(SmallFont);
  myOLED.print(watch.getTime("%a %d %b"), CENTER, 21);

  myOLED.print(String(watch.getWeatherAt(0).temp) + "C", LEFT, 44);
  myOLED.print(watch.getWeatherCity(), LEFT, 54);

  if (watch.isConnected())
  {
    myOLED.drawBitmap(0, 15, bluetooth, 16, 16);
    myOLED.drawRect(110, 23, 127, 30);
    myOLED.drawRectFill(108, 25, 110, 28);
    myOLED.drawRectFill(map(watch.getPhoneBattery(), 0, 100, 127, 110), 23, 127, 30);
    myOLED.print(String(watch.getPhoneBattery()) + "%", RIGHT, 34);
  }
}

void copyMsg(String ms)
{
  msglen = ms.length();
  lines = ceil(float(msglen) / 21);
  switch (lines)
  {
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
  default:

    break;
  }
}
