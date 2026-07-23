#include <mqtt_v2.h>
#include <OneWire.h>
#include <Preferences.h>

#include <cl_ds18b20.h>

#define DEF_PATH "V2.0/CHICKEN"
#define DEF_SUBPATH_DS18B20     "dht22"
#define DEF_RELAYS_NUMS  3

int pins[] = {
   23, //свет
   5, //вытяжка
   14 //отопление
}; 

void Subscribe();

mqtt_v2 client( 
  "ESP32_CHICKEN",
  DEF_PATH,
  Subscribe);

OneWire oneWire(32);
DallasTemperature ds18b20(&oneWire);
cl_ds18b20 sens_temperature(&ds18b20, &client, String(DEF_SUBPATH_DS18B20), 30000, false);

struct s_relay { 
  String name;
  int state;
  int mode;
};

struct s_ini{
  int t_on;
  int t_off;
  int t_max;
};

struct s_state{
  s_relay relays[DEF_RELAYS_NUMS];
  s_ini ini;
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
    state->relays[i].mode = preferences.getInt(t.c_str(), 0);
    if(state->relays[i].mode < 0 || state->relays[i].mode > 2) state->relays[i].mode = 0;
    if(debug) Serial.println("read " + t + " = " + String(state->relays[i].mode));
  }
  
  state->ini.t_on = preferences.getInt("t_on", 5);  if(debug) Serial.println("read t_on = " + String(state->ini.t_on));
  state->ini.t_off = preferences.getInt("t_off", 12);  if(debug) Serial.println("read t_off = " + String(state->ini.t_off));
  state->ini.t_max = preferences.getInt("t_max", 20);  if(debug) Serial.println("read t_max = " + String(state->ini.t_max));
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

  if(cur_state.ini.t_on != readed.ini.t_on) {
    preferences.putInt("t_on", cur_state.ini.t_on);
    Serial.println("write t_on = " + String(cur_state.ini.t_on));
  }
  if(cur_state.ini.t_off != readed.ini.t_off) {
    preferences.putInt("t_off", cur_state.ini.t_off);
    Serial.println("write t_off = " + String(cur_state.ini.t_off));
  }
  if(cur_state.ini.t_max != readed.ini.t_max) {
    preferences.putInt("t_max", cur_state.ini.t_max);
    Serial.println("write t_max = " + String(cur_state.ini.t_max));
  }
}

void setup() {
  Serial.begin(115200);                                         
  Serial.println("");  Serial.println("Start!");

  preferences.begin("settings", false);

  sens_temperature.begin();

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

  if (mode == 0 )  {
    client.Publish("settings/t/on", String(cur_state.ini.t_on));
    client.Publish("settings/t/off", String(cur_state.ini.t_off));
    client.Publish("settings/t/max", String(cur_state.ini.t_max));
  }

  client.flag_start = false;
}

void loop() {
  client.loop();

  sens_temperature.loop( );
  
  s_ds18b20_val sens_temperature_val = sens_temperature.get_value();

//свет
  switch(cur_state.relays[0].mode){
    case 1: cur_state.relays[0].state = HIGH; break;
    case 2: cur_state.relays[0].state = LOW; break;
  }

//вытяжка
  switch(cur_state.relays[1].mode){
    case 1: cur_state.relays[1].state = HIGH; break;
    case 2: cur_state.relays[1].state = LOW; break;
    default:
    break;
  }
  
//отопление
  switch(cur_state.relays[2].mode){
    case 1: 
      if(sens_temperature_val.f_value > cur_state.ini.t_max) {
        cur_state.relays[2].state = LOW; 
      } else {
        cur_state.relays[2].state = HIGH; 
      }
    break;
    case 2: cur_state.relays[2].state = LOW; break;
    default:
      if(sens_temperature_val.readed){
        if(sens_temperature_val.f_value <= cur_state.ini.t_on) {
          cur_state.relays[2].state = HIGH;
        } else if(sens_temperature_val.f_value > cur_state.ini.t_off) {
          cur_state.relays[2].state = LOW;
        }
      }
    break;
  }

  report((client.flag_start ? 0 : 1)); //отправляем все
}

void Msg_t_on( const String &message ){
  cur_state.ini.t_on = message.toInt( );
  write_eeprom();
}
void Msg_t_off( const String &message ){
  cur_state.ini.t_off = message.toInt( );
  write_eeprom();
}
void Msg_t_max( const String &message ){
  cur_state.ini.t_max = message.toInt( );
  write_eeprom();
}
void Msg_relays_mode(const String &topic, const String &message) {
  for(int i = 0; i < DEF_RELAYS_NUMS; i++) {
    if(topic == "CHICKEN/modes/" + getName(i)) {
      cur_state.relays[i].mode = message.toInt();
      if(cur_state.relays[i].mode < 0 || cur_state.relays[i].mode > 2) cur_state.relays[i].mode = 0;
      write_eeprom();
      return;
    }    
  }
}

void Subscribe(){
  sens_temperature.subscribe( );

  for(int i = 0; i < DEF_RELAYS_NUMS; i++) {
    client.Subscribe("modes/" + getName(i), Msg_relays_mode); 
  }
  
  client.Subscribe("settings/t/on", Msg_t_on); 
  client.Subscribe("settings/t/off", Msg_t_off); 
  client.Subscribe("settings/t/max", Msg_t_max); 
}