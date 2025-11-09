#ifndef MQTTHLK_LD2410_h
#define MQTTHLK_LD2410_h

#include "sensor.h"
#include "mqtt_client.h"
#include <ld2410.h>

#define HLK_LD2410_TOPIC_INTERVAL "/settings/interval"
#define HLK_LD2410_INTERVAL_MIN 100
#define HLK_LD2410_VALUE_TOPIC "/value"

class MQTTHLK_LD2410;

struct s_ld2410c{
  int pin_IO_value;
  int presenceDetected;
  
  int stationaryTargetDetected;
  uint16_t stationaryTargetDistance;
  uint8_t stationaryTargetEnergy;
  
  int movingTargetDetected;
  uint16_t movingTargetDistance;
  uint8_t movingTargetEnergy;

  int no_target;
};

class MQTTHLK_LD2410: public sensor {
    public:
        MQTTHLK_LD2410(ld2410* _sens, HardwareSerial* _serial, MQTTClient* _mqttClient, String _topicSensor, 
                       int _pinTX, int _pinRX, int _pinOUT = -1) : 
        sensor(_mqttClient, _topicSensor, HLK_LD2410_INTERVAL_MIN) {
            pinTX = _pinTX;
            pinRX = _pinRX;
            pinOUT = _pinOUT;
            sens = _sens;
            hlkSerial = _serial;
        };

        virtual ~MQTTHLK_LD2410(){

        };

        void begin(){
            if(sens == NULL) return;
            connect(256000);
            if(pinOUT > 0) pinMode(pinOUT, INPUT);
            Serial.print(F("\nConnect LD2410 radar TX to GPIO:"));
            Serial.println(pinRX);
            Serial.print(F("Connect LD2410 radar RX to GPIO:"));
            Serial.println(pinTX);
            Serial.print(F("LD2410 radar sensor initialising: "));
            if (sens->begin(*hlkSerial)) {
                Serial.println(F("OK"));
                Serial.print(F("LD2410 firmware version: "));
                Serial.print(sens->firmware_major_version);
                Serial.print('.');
                Serial.print(sens->firmware_minor_version);
                Serial.print('.');
                Serial.println(sens->firmware_bugfix_version, HEX);
            } else {
                Serial.println(F("not connected"));
            }
            BeginOK = true;
        }

        virtual void PublicIni() override{
            mqttClient->Publish(topicSensor + HLK_LD2410_TOPIC_INTERVAL, String(interval));
        }

        virtual void Subscribe() override{
            mqttClient->subscribeWithPref(topicSensor + HLK_LD2410_TOPIC_INTERVAL, 
                [this](const String &message) { this->CallbackSettings(message); });
        }

        virtual void _ReadSensor() override{
            s_ld2410c bef = LastValue;
            s_ld2410c val = read();
            if(mqttClient == NULL) return;

            if(bef.pin_IO_value != val.pin_IO_value) {
                mqttClient->Publish(topicSensor + HLK_LD2410_VALUE_TOPIC + "/pin_IO_value", String(val.pin_IO_value));
            }
            if(bef.presenceDetected != val.presenceDetected) {
                mqttClient->Publish(topicSensor + HLK_LD2410_VALUE_TOPIC + "/presenceDetected", String(val.presenceDetected));
            }
            if(bef.stationaryTargetDetected != val.stationaryTargetDetected) {
                mqttClient->Publish(topicSensor + HLK_LD2410_VALUE_TOPIC + "/stationary/TargetDetected", String(val.stationaryTargetDetected));
            }
            /*if(bef.stationaryTargetDistance != val.stationaryTargetDistance) {
                mqttClient->Publish(topicSensor + HLK_LD2410_VALUE_TOPIC + "/stationary/TargetDistance", String(val.stationaryTargetDistance));
            }
            if(bef.stationaryTargetEnergy != val.stationaryTargetEnergy) {
                mqttClient->Publish(topicSensor + HLK_LD2410_VALUE_TOPIC + "/stationary/TargetEnergy", String(val.stationaryTargetEnergy));
            }*/
            if(bef.movingTargetDetected != val.movingTargetDetected) {
                mqttClient->Publish(topicSensor + HLK_LD2410_VALUE_TOPIC + "/moving/TargetDetected", String(val.movingTargetDetected));
            }
            /*if(bef.movingTargetDistance != val.movingTargetDistance) {
                mqttClient->Publish(topicSensor + HLK_LD2410_VALUE_TOPIC + "/moving/TargetDistance", String(val.movingTargetDistance));
            }
            if(bef.movingTargetEnergy != val.movingTargetEnergy) {
                mqttClient->Publish(topicSensor + HLK_LD2410_VALUE_TOPIC + "/moving/TargetEnergy", String(val.movingTargetEnergy));
            }*/
            if(bef.no_target != val.no_target) {
                mqttClient->Publish(topicSensor + HLK_LD2410_VALUE_TOPIC + "/no_target", String(val.no_target));
            }
        }        

        s_ld2410c read(){
            s_ld2410c val;
            if(sens == NULL) return val;
            sens->read();
            if(sens->isConnected())
            {
                val.presenceDetected = sens->presenceDetected() ? 1 : 0;
                val.no_target = 0;
                if(val.presenceDetected) {
                    val.stationaryTargetDetected = sens->stationaryTargetDetected() ? 1 : 0;
                    if(val.stationaryTargetDetected) {
                        val.stationaryTargetDistance = sens->stationaryTargetDistance();
                        val.stationaryTargetEnergy = sens->stationaryTargetEnergy();
                    }
                    val.movingTargetDetected = sens->movingTargetDetected() ? 1 : 0;
                    if(val.movingTargetDetected)
                    {
                        val.movingTargetDistance = sens->movingTargetDistance();
                        val.movingTargetEnergy = sens->movingTargetEnergy();
                    }
                }
                else
                {
                    val.no_target = 1;
                }
            }
            val.pin_IO_value = digitalRead(pinOUT);

            LastValue = val;            
            return val;
        };        

        s_ld2410c getValue(){
            return LastValue;
        };

    protected:

    private:
        int pinTX;
        int pinRX;
        int pinOUT;
        bool radarConnected = false;
        HardwareSerial* hlkSerial;    
        ld2410* sens;
        boolean BeginOK = false;
        s_ld2410c LastValue;
        bool wasDetecting = false;

        void connect(int bitrate){
            if(bitrate <= 0) {
                bitrate = 256000;
            }
            hlkSerial->begin(bitrate, SERIAL_8N1, pinRX, pinTX);
            delay(500);
        };

        void CallbackSettings(const String &message){
            uint32_t val = interval = message.toInt();
            interval = (interval<HLK_LD2410_INTERVAL_MIN ? HLK_LD2410_INTERVAL_MIN : interval);
            if(interval<=0 || val!=interval) {
                mqttClient->Publish(topicSensor + HLK_LD2410_TOPIC_INTERVAL, String(interval));
            }
        }
    };

#endif