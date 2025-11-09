#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <mqtt_client.h>
#include <MQTTbh1750fvi.h>
#include <MQTTHLK_LD2410C.h>
#include <FastLED.h>
#include "GyverButton.h"
#include <Preferences.h>

#define DEF_TOPIC "V2/MIRROW_v2"
#define PIN_LED               19
#define PIN_BH1750FVI_ADDR    16
#define HLK_RX 32
#define HLK_TX 33
#define HLK_OUT 25  
#define PIN_BTN               18

#define NUM_LEDS              5 //168
#define delayFL 5
CRGB leds[NUM_LEDS];
#define CURRENT_LIMIT 3000    // лимит по току в миллиамперах

#define TOPIC_LUX_OFF "settings/LuxOff"
#define TOPIC_LUX_ON "settings/LuxOn"
#define TOPIC_LED_STATE "LedState"
#define TOPIC_LED_MODE "LedMode"
#define TOPIC_LED_PIC "LedPic"
#define TOPIC_ALARM "Alarm"
#define TOPIC_RGB_R "rgb/r"
#define TOPIC_RGB_G "rgb/g"
#define TOPIC_RGB_B "rgb/b"
#define TOPIC_LED_BRIGHTNESS "settings/brightness"

#define DEF_LUX_ON 5
#define DEF_LUX_OFF 50
#define DEF_LUX_MIN 0
#define DEF_LUX_MAX 65535
#define DEF_BRIGHTNESS 20
#define DEF_MIN_BRIGHTNESS 5
#define DEF_LED_R 255
#define DEF_LED_G 198
#define DEF_LED_B 51
#define DEF_HLK_INTERVAL 100
#define DEF_LUX_INTERVAL 500

void ReadStates(boolean refresh = false);
void SetLuxOn(const String &message);
void SetLuxOff(const String &message);
void SetMode(const String &message);
void SetAlarm(const String &message);
void SetBrightness(const String &message);
void SetPic(const String &message);
void SetR(const String &message);
void SetG(const String &message);
void SetB(const String &message);

enum MIRROW_LED_PIC {
  rainbow = 0,
  color = 1
};

struct s_state{
  int LuxOff = DEF_LUX_OFF;
  int LuxOn = DEF_LUX_ON;
  int brightness = DEF_BRIGHTNESS;
  int r = DEF_LED_R;
  int g = DEF_LED_G;
  int b = DEF_LED_B;
  int ledMode = 0;
  int ledPic = rainbow;
  int alarm = 0;

  int ledState = 0;

  int cur_brightness;

  uint32_t hlkInterval = DEF_HLK_INTERVAL;
  uint32_t luxInterval = DEF_LUX_INTERVAL;
};

s_state cur_state;

void SendOnStart();

MQTTClient client( 
  DEF_TOPIC,
  "192.168.1.40",
  "MIRROW_v2",
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
    SendOnStart();
    delay(500);
    client.subscribeWithPref(TOPIC_LED_MODE, SetMode); 
    client.subscribeWithPref(TOPIC_LUX_OFF, SetLuxOff); 
    client.subscribeWithPref(TOPIC_LUX_ON, SetLuxOn); 
    client.subscribeWithPref(TOPIC_LED_BRIGHTNESS, SetBrightness); 
    client.subscribeWithPref(TOPIC_LED_PIC, SetPic); 
    client.subscribeWithPref(TOPIC_ALARM, SetAlarm); 
    client.subscribeWithPref(TOPIC_RGB_R, SetR); 
    client.subscribeWithPref(TOPIC_RGB_G, SetG); 
    client.subscribeWithPref(TOPIC_RGB_B, SetB); 
  }, //onReadDevicesCallback
  [](boolean refresh){ //read devices
    ReadStates(refresh);
  },
  1883,
  true, //send working
  180, //wdt
  true //console log
);

BH1750FVI::eDeviceAddress_t DEVICEADDRESS = BH1750FVI::k_DevAddress_H;
BH1750FVI::eDeviceMode_t DEVICEMODE = BH1750FVI::k_DevModeContHighRes2;
MQTTbh1750fvi bh1750fvi(new BH1750FVI(PIN_BH1750FVI_ADDR, DEVICEADDRESS, DEVICEMODE), 
                        &client, 
                        "sensors/bh1750fvi");

MQTTHLK_LD2410 hlk(new ld2410(), &Serial1, &client, "sensors/LD2410", HLK_TX, HLK_RX, HLK_OUT);

GButton butt(PIN_BTN);

Preferences preferences;

void SendOnStart(){
    client.Publish(TOPIC_LED_PIC, String(cur_state.ledPic));
    client.Publish(TOPIC_LED_MODE, String(cur_state.ledMode));
    client.Publish(TOPIC_LED_BRIGHTNESS, String(cur_state.brightness));
    client.Publish(TOPIC_LUX_OFF, String(cur_state.LuxOff));
    client.Publish(TOPIC_LUX_ON, String(cur_state.LuxOn));
    client.Publish(TOPIC_RGB_R, String(cur_state.r));
    client.Publish(TOPIC_RGB_G, String(cur_state.g));
    client.Publish(TOPIC_RGB_B, String(cur_state.b));
    hlk.PublicIni();
    bh1750fvi.PublicIni();
}

void read_settings(){
  cur_state.ledMode = preferences.getInt("mode", 0);
  if(cur_state.ledMode < 0 || cur_state.ledMode > 1) cur_state.ledMode = 0;
  Serial.println("ledMode = " + String(cur_state.ledMode));

  cur_state.ledPic = preferences.getInt("pic", 0);
  if(cur_state.ledPic < 0 || cur_state.ledPic > 1) cur_state.ledPic = 0;
  Serial.println("ledPic = " + String(cur_state.ledPic));

  cur_state.LuxOff = preferences.getInt("lOff", DEF_LUX_OFF);
  if(cur_state.LuxOff < DEF_LUX_MIN || cur_state.LuxOff > DEF_LUX_MAX) cur_state.LuxOff = DEF_LUX_OFF;
  Serial.println("LuxOff = " + String(cur_state.LuxOff));

  cur_state.LuxOn = preferences.getInt("lOn", DEF_LUX_ON);
  if(cur_state.LuxOn < DEF_LUX_MIN || cur_state.LuxOn > DEF_LUX_MAX) cur_state.LuxOn = DEF_LUX_ON;
  Serial.println("LuxOn = " + String(cur_state.LuxOn));

  cur_state.brightness = preferences.getInt("brgth", DEF_BRIGHTNESS);
  if(cur_state.brightness < DEF_MIN_BRIGHTNESS || cur_state.brightness > 255) cur_state.brightness = DEF_BRIGHTNESS;
  Serial.println("brightness = " + String(cur_state.brightness));

  cur_state.r = preferences.getInt("r", DEF_LED_R);
  if(cur_state.r < 0 || cur_state.r > 255) cur_state.r = DEF_LED_R;
  Serial.println("r = " + String(cur_state.r));

  cur_state.g = preferences.getInt("g", DEF_LED_G);
  if(cur_state.g < 0 || cur_state.g > 255) cur_state.g = DEF_LED_G;
  Serial.println("g = " + String(cur_state.g));

  cur_state.b = preferences.getInt("b", DEF_LED_B);
  if(cur_state.b < 0 || cur_state.b > 255) cur_state.b = DEF_LED_B;
  Serial.println("b = " + String(cur_state.b));

  cur_state.hlkInterval = preferences.getUInt("hlki", DEF_HLK_INTERVAL);
  if(cur_state.hlkInterval < 0) cur_state.hlkInterval = DEF_HLK_INTERVAL;
  Serial.println("hlkInterval = " + String(cur_state.hlkInterval));

  cur_state.luxInterval = preferences.getUInt("luxi", DEF_LUX_INTERVAL);
  if(cur_state.luxInterval < 0) cur_state.luxInterval = DEF_LUX_INTERVAL;
  Serial.println("luxInterval = " + String(cur_state.luxInterval));
}

void setup() {
  Serial.begin(115200);
  Serial.println("");  Serial.println("Start!");

  preferences.begin("settings", false);
  read_settings();

  hlk.setInterval(cur_state.hlkInterval);
  hlk.begin();

  bh1750fvi.setInterval(cur_state.luxInterval);
  bh1750fvi.begin();
  
  FastLED.addLeds<WS2812B, PIN_LED, GRB>(leds, NUM_LEDS);  // GRB ordering is assumed
  if (CURRENT_LIMIT > 0) FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT);
  FastLED.clear();
  FastLED.show();

  client.begin();

  butt.setType(LOW_PULL);
  butt.setDirection(NORM_OPEN);

  sei();
  delay(200);
}

uint32_t LastLedStep;
void SetColor(){
  for (int i = 0 ; i < NUM_LEDS; i++ )
  {
    leds[i].setRGB(cur_state.r, cur_state.g, cur_state.b);
  }
}

int ihue = 0;                //-HUE (0-255)
int thisdelay = 10;          //-FX LOOPS DELAY VAR
void loop_rainbow() {        //-m88-RAINBOW FADE FROM FAST_SPI2
  ihue -= 1;
  fill_rainbow( leds, NUM_LEDS, ihue );
  FastLED.show();
  delay(thisdelay);
}

bool ledPic1 = false;
void showPic(){
  switch(cur_state.ledPic){
    case 1: 
            SetColor(); 
            if(!ledPic1) {
              FastLED.show();
              ledPic1 = true;
            }
            break;
    
    default: 
            loop_rainbow(); 
            break;
  }
}

void On(){
  if(cur_state.ledState != 1){
    ledPic1 = false;
    cur_state.ledState = 1;
    client.Publish(TOPIC_LED_STATE, String(cur_state.ledState));
  }
  int dir = 0;
  if(cur_state.cur_brightness != cur_state.brightness){
    if(cur_state.cur_brightness < cur_state.brightness){
      dir = 1;
    } else {
      dir = -1;
    }
    if(millis() - LastLedStep > delayFL){
      cur_state.cur_brightness = cur_state.cur_brightness + dir;
      FastLED.setBrightness(cur_state.cur_brightness);
      LastLedStep = millis();
    }
  }
}

void Off(){
  if(cur_state.ledState != 0){
    cur_state.ledState = 0;
    client.Publish(TOPIC_LED_STATE, String(cur_state.ledState));
  }
  if(cur_state.cur_brightness > 0){
    if(millis() - LastLedStep > delayFL){
      FastLED.setBrightness(--cur_state.cur_brightness);
      FastLED.show();
      LastLedStep = millis();
    }
  }
}

void Auto(){
  s_ld2410c hlk_state = hlk.getValue();
  s_bh1750fvi_val lux_state = bh1750fvi.getValue();
  if(lux_state.lux < cur_state.LuxOn) {
    if(hlk_state.pin_IO_value == 1 || hlk_state.presenceDetected == 1){
      On();
    } else {
      Off();
    }
  } else if(lux_state.lux > cur_state.LuxOff) {
    Off();
  }
}
uint8_t hue; 
void colorsRoutine(){
  hue += 10;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue, 255, 255);
  }
  FastLED.show();
}

void Alarm(){
  if(cur_state.cur_brightness < 255) {
    FastLED.setBrightness(255);
    cur_state.cur_brightness = 255;
  }
  colorsRoutine();
}

void procButt(){
  butt.tick();

  int ledMode = cur_state.ledMode;
  int ledPic = cur_state.ledPic;

  int cnt = 0;
  // удержание
  if (butt.isStep()) {
    cnt = 1;
    butt.resetStates();
  }

  // один клик + удержание
  if (butt.isStep(1)) {
    cnt = 2;
    butt.resetStates();
  }

  // два клика + удержание
  if (butt.isStep(2)) {
    cnt = 3;
    butt.resetStates();
  }

  switch(cnt){
    case 1:
      if(cur_state.ledState == 0) {
        ledMode = 1;
      } else {
        ledMode = 2;
      }
      break;

    case 2:
      ledMode = 0;
      break;

    case 3:
      ledPic++; if(ledPic > 1) ledPic = 0;
      break;
  }

  if(ledMode != cur_state.ledMode){
    cur_state.ledMode = ledMode;
    preferences.putInt("mode", cur_state.ledMode);
    client.Publish(TOPIC_LED_MODE, String(cur_state.ledMode));
  }

  if(ledPic != cur_state.ledPic){
    cur_state.ledPic = ledPic;
    preferences.putInt("pic", cur_state.ledPic);
    client.Publish(TOPIC_LED_PIC, String(cur_state.ledPic));
  }
}

void loop() {
  client.loop();
  procButt();

  uint32_t hlki = hlk.getInterval();
  if(cur_state.hlkInterval != hlki){
    cur_state.hlkInterval = hlki;
    preferences.putUInt("hlki", cur_state.hlkInterval);
  }

  uint32_t luxi = bh1750fvi.getInterval();
  if(cur_state.luxInterval != luxi){
    cur_state.luxInterval = luxi;
    preferences.putUInt("luxi", cur_state.luxInterval);
  }

  if(cur_state.alarm != 1) {
    switch(cur_state.ledMode){
      case 1:  On(); break;
      case 2:  Off(); break;
      default: Auto(); break;
    }
    if(cur_state.cur_brightness > 0) {
      showPic();
    }
  } else { //Alarm
    Alarm();
  }
}

void ReadStates(boolean refresh){
  bh1750fvi.loop();
  hlk.loop();
}

void SetLuxOff(const String &message) {
  int sval = cur_state.LuxOn;
  int val = cur_state.LuxOff = message.toInt();
  cur_state.LuxOff = (cur_state.LuxOff <= DEF_LUX_MIN || cur_state.LuxOff > DEF_LUX_MAX ? DEF_LUX_MAX : cur_state.LuxOff);
  if(val!=cur_state.LuxOff) {
    client.Publish(TOPIC_LUX_OFF, String(cur_state.LuxOff));
  }
  if(sval != cur_state.LuxOff){
    Serial.println("write LuxOff = " + String(cur_state.LuxOff));
    preferences.putInt("lOff", cur_state.LuxOff);
  }
}

void SetLuxOn(const String &message) {
  int sval = cur_state.LuxOn;
  int val = cur_state.LuxOn = message.toInt();
  cur_state.LuxOn = (cur_state.LuxOn < 0 || cur_state.LuxOn > DEF_LUX_MAX ? DEF_LUX_MIN : cur_state.LuxOn);
  if(val!=cur_state.LuxOn) {
    client.Publish(TOPIC_LUX_ON, String(cur_state.LuxOn));
  }
  if(sval != cur_state.LuxOn){
    Serial.println("write LuxOn = " + String(cur_state.LuxOn));
    preferences.putInt("lOn", cur_state.LuxOn);
  }
}

void SetMode(const String &message) {
  int sval = cur_state.ledMode;
  int val = cur_state.ledMode = message.toInt();
  cur_state.ledMode = (cur_state.ledMode < 0 || cur_state.ledMode > 2 ? 0 : cur_state.ledMode);
  if(val!=cur_state.ledMode) {
    client.Publish(TOPIC_LED_MODE, String(cur_state.ledMode));
  }
  if(sval != cur_state.ledMode){
    Serial.println("write ledMode = " + String(cur_state.ledMode));
    preferences.putInt("mode", cur_state.ledMode);
  }
}

void SetAlarm(const String &message) {
  int sval = cur_state.alarm;
  int val = cur_state.alarm = message.toInt();
  cur_state.alarm = (cur_state.alarm < 0 || cur_state.alarm > 1 ? 0 : cur_state.alarm);
  if(val != cur_state.LuxOn) {
    client.Publish(TOPIC_ALARM, String(cur_state.alarm));
  }
  if(cur_state.alarm == 0 && sval != 0){
    FastLED.setBrightness(0);
    cur_state.cur_brightness = 0;
  }
}

void SetBrightness(const String &message) {
  int sval = cur_state.brightness;
  int val = cur_state.brightness = message.toInt();
  cur_state.brightness = (cur_state.brightness < DEF_MIN_BRIGHTNESS || cur_state.brightness > 255 ? DEF_MIN_BRIGHTNESS : cur_state.brightness);
  if(val != cur_state.brightness) {
    client.Publish(TOPIC_LED_BRIGHTNESS, String(cur_state.brightness));
  }
  if(sval != cur_state.brightness){
    Serial.println("write brightness = " + String(cur_state.brightness));
    preferences.putInt("brgth", cur_state.brightness);
  }
}

void SetPic(const String &message) {
  int sval = cur_state.ledPic;
  int val = cur_state.ledPic = message.toInt();
  cur_state.ledPic = (cur_state.ledPic < 0 || cur_state.ledPic > 1 ? 0 : cur_state.ledPic);
  if(val != cur_state.ledPic) {
    client.Publish(TOPIC_LED_PIC, String(cur_state.ledPic));
  }
  if(sval != cur_state.ledPic){
    Serial.println("write ledPic = " + String(cur_state.ledPic));
    preferences.putInt("pic", cur_state.ledPic);
  }
}

void SetR(const String &message) {
  int sval = cur_state.r;
  int val = cur_state.r = message.toInt();
  cur_state.r = (cur_state.r < 0 ? 0 : cur_state.r);
  cur_state.r = (cur_state.r >255 ? 255 : cur_state.r);
  if(val != cur_state.r) {
    client.Publish(TOPIC_RGB_R, String(cur_state.r));
  }
  if(sval != cur_state.r){
    Serial.println("write r = " + String(cur_state.r));
    preferences.putInt("r", cur_state.r);
  }
}
void SetG(const String &message) {
  int sval = cur_state.g;
  int val = cur_state.g = message.toInt();
  cur_state.g = (cur_state.g < 0 ? 0 : cur_state.g);
  cur_state.g = (cur_state.g >255 ? 255 : cur_state.g);
  if(val != cur_state.g) {
    client.Publish(TOPIC_RGB_G, String(cur_state.g));
  }
  if(sval != cur_state.g){
    Serial.println("write g = " + String(cur_state.g));
    preferences.putInt("g", cur_state.g);
  }
}
void SetB(const String &message) {
  int sval = cur_state.b;
  int val = cur_state.b = message.toInt();
  cur_state.b = (cur_state.b < 0 ? 0 : cur_state.b);
  cur_state.b = (cur_state.b >255 ? 255 : cur_state.b);
  if(val != cur_state.b) {
    client.Publish(TOPIC_RGB_B, String(cur_state.b));
  }
  if(sval != cur_state.b){
    Serial.println("write b = " + String(cur_state.b));
    preferences.putInt("b", cur_state.b);
  }
}
