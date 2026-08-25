#include <mqtt_v2.h>

#define DEF_PATH "V3/CHICKEN_FL"
#define DEF_RELAYS_NUMS  2

int pins[] = {
   4,
   5
}; 

void Subscribe();

mqtt_v2 client( 
  "ESP32_CHICKEN_FL",
  DEF_PATH,
  Subscribe);

struct s_relay { 
  String name;
  int state;
  int mode;
};

struct s_state{
  s_relay relays[DEF_RELAYS_NUMS];
};

s_state cur_state;

String getName(int i) {
  String ret = String(i);
  if(ret.length() == 1) ret = "r0" + ret;
  else ret = "r" + ret;
  return ret;
}

void setup() {
  Serial.begin(115200);                                         
  Serial.println("");  Serial.println("Start!");

  for(int i = 0; i < DEF_RELAYS_NUMS; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
    cur_state.relays[i].name = getName(i);
  }
  client.begin(true);
}

void report(int mode ){
  for(int i = 0; i < DEF_RELAYS_NUMS; i++) {
    if (mode == 0 || ( mode == 1 && cur_state.relays[i].state != digitalRead(pins[i]) ) )  {

      digitalWrite(pins[i], cur_state.relays[i].state);
      client.Publish("states/" + getName(i), String(cur_state.relays[i].state));
    }
    if (mode == 0 )  {
      client.Publish("modes/" + getName(i), String(cur_state.relays[i].mode));
    }
  }
  client.flag_start = false;
}

void loop() {
  client.loop();

  switch(cur_state.relays[0].mode){
    case 1: cur_state.relays[0].state = HIGH; break;
    case 2: cur_state.relays[0].state = LOW; break;
    default:
    break;
  }

  switch(cur_state.relays[1].mode){
    case 1: cur_state.relays[1].state = HIGH; break;
    case 2: cur_state.relays[1].state = LOW; break;
    default:
    break;
  }

  report((client.flag_start ? 0 : 1)); //отправляем все
}

void Msg_relays_mode(const String &topic, const String &message) {
  for(int i = 0; i < DEF_RELAYS_NUMS; i++) {
    if(topic == String(DEF_PATH) + "/modes/" + getName(i)) {
      cur_state.relays[i].mode = message.toInt();
      if(cur_state.relays[i].mode < 0 || cur_state.relays[i].mode > 2) cur_state.relays[i].mode = 0;
      return;
    }    
  }
}

void Subscribe(){
  for(int i = 0; i < DEF_RELAYS_NUMS; i++) {
    client.Subscribe("modes/" + getName(i), Msg_relays_mode); 
  }
}