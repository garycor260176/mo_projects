#include <mqtt_v2.h>
#include <OneWire.h>
#include <Preferences.h>
#include <cl_ds18b20.h>
#include <cl_bme280.h>
#include <cl_bh1750fvi.h>
#include <cl_rg11.h>
#include <cl_wdir.h>
#include <cl_wspeed.h>

#define PIN_PWR_SENS        15
#define PIN_DS18B20         19
uint8_t PIN_BH1750FVI_ADDR  = 16;
#define PIN_RG11            13
#define PIN_WDIR            32
#define PIN_WSPEED          33

#define DEF_PATH              "V3/meteo"
#define DEF_SUBPATH_DS18B20   "ds18b20"
#define DEF_SUBPATH_BME280    "bme280"
#define DEF_SUBPATH_BH1750FVI "bh1750fvi"
#define DEF_SUBPATH_RG11      "rg11"
#define DEF_SUBPATH_WDIR      "wdir"
#define DEF_SUBPATH_WSPEED    "wspeed"

void Subscribe();
void refresh(boolean refresh=false);

void onRefresh(){
  refresh(true);
}

mqtt_v2 client( 
  "ESP32_METEO",
  DEF_PATH,
  Subscribe,
  NULL,
  onRefresh
);

OneWire oneWire(PIN_DS18B20);
DallasTemperature ds18b20(&oneWire);
cl_ds18b20 sens_temperature(&ds18b20, &client, String(DEF_SUBPATH_DS18B20), 30000, false);

iarduino_Pressure_BMP bmp(0x76);
cl_bme280 bme280(&bmp, &client, String(DEF_SUBPATH_BME280), 30000, false);

uint32_t LastRead_bh1750fvi = 0; //запоминаем время последней публикации
BH1750FVI::eDeviceAddress_t DEVICEADDRESS = BH1750FVI::k_DevAddress_H;
BH1750FVI::eDeviceMode_t DEVICEMODE = BH1750FVI::k_DevModeContHighRes2;
BH1750FVI _BH1750FVI(PIN_BH1750FVI_ADDR, DEVICEADDRESS, DEVICEMODE);
cl_bh1750fvi bh1750fvi(&_BH1750FVI, &client, String(DEF_SUBPATH_BH1750FVI), 30000, false);

#define BUCKET_SIZE         0.01
cl_rg11 rg11(PIN_RG11, &client, String(DEF_SUBPATH_RG11), BUCKET_SIZE, false);

cl_wdir wdir(ADC1_CHANNEL_4, &client, String(DEF_SUBPATH_WDIR), 1000, false);

cl_wspeed wspeed(ADC1_CHANNEL_5, &client, String(DEF_SUBPATH_WSPEED), 100, false);

void refresh(boolean refresh){  
  sens_temperature.loop(refresh);
  bme280.loop(refresh);
  bh1750fvi.loop(refresh);
  rg11.loop();
  wdir.loop(refresh);
  wspeed.loop(refresh);
}

void setup() {
  Serial.begin(115200);                                         
  Serial.println("");  Serial.println("Start!");
  
  pinMode(PIN_PWR_SENS, OUTPUT); digitalWrite(PIN_PWR_SENS, HIGH);

  sens_temperature.begin();
  bme280.begin();
  bh1750fvi.begin();
  rg11.begin( );
  wdir.begin();
  wspeed.begin();

  client.begin(true);
}

void report(int mode ){

  client.flag_start = false;
}

void loop() {
  client.loop();

  refresh();

  report((client.flag_start ? 0 : 1)); //отправляем все
}

void Subscribe(){
  sens_temperature.subscribe( );
  bme280.subscribe( );
  bh1750fvi.subscribe( );
  wdir.subscribe( );
  wspeed.subscribe( );
}