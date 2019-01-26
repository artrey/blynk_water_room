#include <EEPROM.h>
#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN_RELAY_1 5
#define PIN_RELAY_2 6
#define PIN_RELAY_3 7
#define PIN_RELAY_4 8
#define PIN_TEMP_1 15
#define PIN_TEMP_2 16

#define TEMP_UPDATE_TIME 1000
#define EEPROM_INITIAL_ADDRESS 10
#define MAX_SECONDS_IN_DAY 86400

bool isFirstConnect = true;
const char auth[] = "30d5a0b06dc74b02b273ce102f9c476a";

int relays[] = { PIN_RELAY_1, PIN_RELAY_2, PIN_RELAY_3, PIN_RELAY_4 };
const int relayCount = sizeof(relays) / sizeof(int);

BlynkTimer timer;
WidgetRTC rtc;

OneWire ds18b20[] = { PIN_TEMP_1, PIN_TEMP_2 };
const int oneWireCount = sizeof(ds18b20) / sizeof(OneWire);
DallasTemperature sensors[oneWireCount];
float tempValues[oneWireCount];
long lastUpdateTime = 0;

void setup()
{  
  pinMode(PIN_RELAY_1, OUTPUT);
  pinMode(PIN_RELAY_2, OUTPUT);
  pinMode(PIN_RELAY_3, OUTPUT);
  pinMode(PIN_RELAY_4, OUTPUT);

  digitalWrite(PIN_RELAY_1, HIGH);
  digitalWrite(PIN_RELAY_2, HIGH);
  digitalWrite(PIN_RELAY_3, HIGH);
  digitalWrite(PIN_RELAY_4, HIGH);
  
  Blynk.begin(auth);

  setSyncInterval(10 * 60);

  DeviceAddress deviceAddress;
  for (int i = 0; i < oneWireCount; ++i)
  {
    sensors[i].setOneWire(&ds18b20[i]);
    sensors[i].begin();
    if (sensors[i].getAddress(deviceAddress, 0))
    {
      sensors[i].setResolution(deviceAddress, 12);
      sensors[i].requestTemperatures();
    }
  } 
}

inline unsigned long secondsInDay(byte hours, byte minutes, byte seconds)
{
  return hours * 3600UL + minutes * 60UL + seconds;
}

BLYNK_CONNECTED()
{
  rtc.begin();
  if (isFirstConnect) {
//    Blynk.syncAll();
    
    // here restore from eeprom
    unsigned long currTime = secondsInDay(hour(), minute(), second());
    for (int i = 0; i < relayCount; ++i)
    {
      unsigned long start = secondsInDay(
        EEPROM.read(EEPROM_INITIAL_ADDRESS + i * 6),
        EEPROM.read(EEPROM_INITIAL_ADDRESS + i * 6 + 1),
        EEPROM.read(EEPROM_INITIAL_ADDRESS + i * 6 + 2)
      );
      unsigned long finish = secondsInDay(
        EEPROM.read(EEPROM_INITIAL_ADDRESS + i * 6 + 3),
        EEPROM.read(EEPROM_INITIAL_ADDRESS + i * 6 + 4),
        EEPROM.read(EEPROM_INITIAL_ADDRESS + i * 6 + 5)
      );

      if (start < MAX_SECONDS_IN_DAY && finish < MAX_SECONDS_IN_DAY)
      {
        if (start <= finish)
        {
          if (currTime >= start && currTime <= finish)
          {
            digitalWrite(relays[i], LOW);
          }
        }
        else
        {
          if (currTime >= start || currTime <= finish)
          {
            digitalWrite(relays[i], LOW);
          }
        }
      }
      else if (start < MAX_SECONDS_IN_DAY || finish < MAX_SECONDS_IN_DAY)
      {
        if (start < MAX_SECONDS_IN_DAY && currTime >= start)
        {
          digitalWrite(relays[i], LOW);
        }
        else if (finish < MAX_SECONDS_IN_DAY && currTime <= finish)
        {
          digitalWrite(relays[i], LOW);
        }
      }
    }
    
    isFirstConnect = false;
  }
}

inline void writeTimeToEeprom(int offset)
{
  EEPROM.write(offset, (byte)hour());
  EEPROM.write(offset + 1, (byte)minute());
  EEPROM.write(offset + 2, (byte)second());
}

BLYNK_WRITE(V1)
{
  digitalWrite(PIN_RELAY_1, param.asInt());
  param.asInt() ? writeTimeToEeprom(EEPROM_INITIAL_ADDRESS + 3) : writeTimeToEeprom(EEPROM_INITIAL_ADDRESS);
}

BLYNK_WRITE(V2)
{
  digitalWrite(PIN_RELAY_2, param.asInt());
  param.asInt() ? writeTimeToEeprom(EEPROM_INITIAL_ADDRESS + 9) : writeTimeToEeprom(EEPROM_INITIAL_ADDRESS + 6);
}

BLYNK_WRITE(V3)
{
  digitalWrite(PIN_RELAY_3, param.asInt());
  param.asInt() ? writeTimeToEeprom(EEPROM_INITIAL_ADDRESS + 15) : writeTimeToEeprom(EEPROM_INITIAL_ADDRESS + 12);
}

BLYNK_WRITE(V4)
{
  digitalWrite(PIN_RELAY_4, param.asInt());
  param.asInt() ? writeTimeToEeprom(EEPROM_INITIAL_ADDRESS + 21) : writeTimeToEeprom(EEPROM_INITIAL_ADDRESS + 18);
}

BLYNK_READ(V5)
{  
  Blynk.virtualWrite(V5, tempValues[0]);
}

BLYNK_READ(V6)
{ 
  Blynk.virtualWrite(V6, tempValues[1]);
}

void loop()
{
  Blynk.run();
  timer.run();

  if (millis() - lastUpdateTime > TEMP_UPDATE_TIME)
  {
    lastUpdateTime = millis();
    for (int i = 0; i < oneWireCount; ++i)
    {
      tempValues[i] = sensors[i].getTempCByIndex(0);      
      sensors[i].requestTemperatures();
    }
  }
}
