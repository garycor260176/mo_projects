#include "mqtt_client.h"
#include "MQTTButtonClick.h"

#define DEF_TOPIC "CHICKENW"

void ReadStates(boolean refresh = false);

MQTTClient client( 
  DEF_TOPIC,
  "192.168.1.40",
  DEF_TOPIC,
  "default",
  "nthfgtdn",
  "admin",
  "1",
  [](const String &payload) { //on topic command
    if (payload == "refresh") {
      ReadStates(true);
    }
  },
  [](){
  }, //on connection
  [](boolean refresh){ //read devices
    ReadStates(refresh);
  },
  1883,
  true, //send working
  180, //wdt
  true //console log
);

#define PIN_FLOAT_UP  18
MQTTButtonClick floatUp(PIN_FLOAT_UP, &client, "sensors/float_up");
#define PIN_FLOAT_DN  19
MQTTButtonClick floatDn(PIN_FLOAT_DN, &client, "sensors/float_dn");

void setup() {
  Serial.begin(115200);
  Serial.println("");  Serial.println("Start!");
  
  client.begin();

  sei();
  delay(200);
}

void loop() {
  client.loop();
}

void ReadStates(boolean refresh){
  floatUp.loop(refresh);
  floatDn.loop(refresh);
}