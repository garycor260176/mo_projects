#include <mqtt_v2.h>
#include <Preferences.h>

#define DEF_PATH "V3/CHICKEN_LAMP"
#define DEF_RELAYS_NUMS  1

int pins[] = {
   19, //свет
}; 

void Subscribe();

mqtt_v2 client( 
  "ESP32_CHICKEN_LAMP",
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

Preferences preferences; 

String getName(int i) {
  String ret = String(i);
  if(ret.length() == 1) ret = "r0" + ret;
  else ret = "r" + ret;
  return ret;
}

void read_eeprom(s_state* state, boolean debug = true){
  for(int i = 0; i < DEF_RELAYS_NUMS; i++) {
    String t = getName(i) + "_mode";
    state->relays[i].mode = preferences.getInt(t.c_str(), 2);
    if(state->relays[i].mode < 0 || state->relays[i].mode > 2) state->relays[i].mode = 0;
    if(debug) Serial.println("read " + t + " = " + String(state->relays[i].mode));
  }
}

void write_eeprom(){
  s_state readed;
  read_eeprom(&readed, false);

  for(int i = 0; i < DEF_RELAYS_NUMS; i++) {
    if(cur_state.relays[i].mode != readed.relays[i].mode) {
      String t = getName(i) + "_mode";
      preferences.putInt(t.c_str(), cur_state.relays[i].mode);
      Serial.println("write " + t + " = " + String(cur_state.relays[i].mode));
    }
  }
}

void setup() {
  Serial.begin(115200);                                         
  Serial.println("");  Serial.println("Start!");

  preferences.begin("settings", false);

  for(int i = 0; i < DEF_RELAYS_NUMS; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
    cur_state.relays[i].name = getName(i);
  }
  read_eeprom(&cur_state);

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

//свет
  switch(cur_state.relays[0].mode){
    case 1: cur_state.relays[0].state = HIGH; break;
    case 2: cur_state.relays[0].state = LOW; break;
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
      write_eeprom();
      return;
    }    
  }
}

void Subscribe(){
  for(int i = 0; i < DEF_RELAYS_NUMS; i++) {
    client.Subscribe("modes/" + getName(i), Msg_relays_mode); 
  }
}