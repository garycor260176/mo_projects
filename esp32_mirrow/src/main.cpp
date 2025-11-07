#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <Arduino.h>
#include <mqtt_client.h>
#include <MQTTbh1750fvi.h>
#include "MQTTHLK_LD2450.h"
#include <FastLED.h>

#define DEF_TOPIC "MIRROW01"
#define PIN_BTN               18
#define PIN_LED               32
#define PIN_BH1750FVI_ADDR    16

#define NUM_LEDS              168
#define delayFL 5
CRGB leds[NUM_LEDS];
#define CURRENT_LIMIT 3000    // лимит по току в миллиамперах

void ReadStates(boolean refresh = false);
void SetLuxOn(const String &message);
void SetLuxOff(const String &message);

MQTTClient client( 
  DEF_TOPIC,
  "192.168.1.40",
  "MIRROW01",
  "default",
  "nthfgtdn",
  "admin",
  "1",
  [](const String &payload) { //on topic command
    if (payload == "refresh") {
      ReadStates(true);
    }
  },
  [](){ //onConnectionCallback
    client.subscribeWithPref("states/", SetLuxOff); 
    client.subscribeWithPref("states/", SetLuxOn); 
  }, //onReadDevicesCallback
  [](boolean refresh){ //read devices
    ReadStates(refresh);
  },
  1883,
  true, //send working
  180, //wdt
  false //console log
);

BH1750FVI::eDeviceAddress_t DEVICEADDRESS = BH1750FVI::k_DevAddress_H;
BH1750FVI::eDeviceMode_t DEVICEMODE = BH1750FVI::k_DevModeContHighRes2;
MQTTbh1750fvi bh1750fvi(new BH1750FVI(PIN_BH1750FVI_ADDR, DEVICEADDRESS, DEVICEMODE), 
                        &client, 
                        "sensors/bh1750fvi");

void setup() {
  Serial.begin(115200);
  Serial.println("");  Serial.println("Start!");

  bh1750fvi.begin();
  client.begin();

  sei();
  delay(200);
}

void loop() {
  client.loop();
}

void ReadStates(boolean refresh){
  bh1750fvi.loop();
}

void SetLuxOff(const String &message) {
  Serial.println("SetLuxOff = " + message);

}

void SetLuxOn(const String &message) {
  Serial.println("SetLuxOn = " + message);
  
}
