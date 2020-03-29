//#define DEBUG

#ifdef DEBUG
  #define WHITE_LED 10
#endif

#define BATTERY_ADC A0
#define BATTERY_SWITCH 2
#define MPR_CHIP_SELECT 3
#define DS18B20_VDD 4
#define DS18B20_ONEWIRE 5

#include "RTC.h"
#include "STM32L0.h"
#include "lora.h"
#include "MPR.h"
#include "DS18B20.h"
#include "sensors.h"


// Global variables
int counter = 0;
int interval = 60;
int mode = 1;
float battery;
float pressures[6];


void setup() {

  #ifdef DEBUG
    // blink to signal initialisation
    pinMode(WHITE_LED, OUTPUT);
    digitalWrite(WHITE_LED, HIGH); 
    delay(200);
    digitalWrite(WHITE_LED, LOW);
  
    // Debug over Serial
    Serial1.begin(9600);
    while(!Serial1);
  
    Serial1.println("Start Initialisation");

    interval = 10;
  #endif
  
  // init voltage reading on supercap
  analogReadResolution(8);
  pinMode(BATTERY_ADC, INPUT);
  pinMode(BATTERY_SWITCH, OUTPUT);
  digitalWrite(BATTERY_SWITCH, LOW);

  // init pressure sensor
  mpr = new MPR(MPR_CHIP_SELECT);

  // init temperature sensor
  pinMode(DS18B20_VDD, OUTPUT);
  digitalWrite(DS18B20_VDD, LOW); 
  ds18b20 = new DS18B20(DS18B20_ONEWIRE);

  // join TTN over LoRaWAN
  initLoRa();

  // set power scheme for first ever cycle
  battery = readBattery();
  updateInterval(battery); 
  updateMode(battery);

  // start clock
  RTC.setEpoch(0);
}


void loop() {
  
  #ifdef DEBUG
    Serial1.println();
    Serial1.print("Cycle: ");   Serial1.println(counter);
  #endif

  // Determine power scheme for next batch based on battery life
  if(counter == 5){
    battery = readBattery();
    updateInterval(battery); 
  }

  // Schedule next alarm
  RTC.setAlarmEpoch(RTC.getEpoch() + interval);
  RTC.enableAlarm(RTC.MATCH_YYMMDDHHMMSS);
  RTC.attachInterrupt(alarmMatch);
    
  // Read pressure
  pressures[counter] = readPressure(25);

  if(counter == 5){  // end of batch

    // Convert pressure readings to flow rates in mm/h multiplied by 100, and find the index of the first flow that is greater than ~2mm/h
    int flows[5];
    int index = 5;
    for(int i=0; i<5; i++){
      flows[i] = (int)round(100 * (3600 / interval) * (pressures[i+1] - pressures[i]) / (4 * 9.80665));
      if(flows[i] < 200){ //flow rate is smaller than ~2mm/h, discard reading
        flows[i] = 0;
      }else if(index == 5){
        index = i;
      }
    }
    
    // if there is information at the first index, send the batch along with mode and temperature
    if(index == 0){
    
      // Read temperature
      float temperature = readTemp();
  
      // Pack payload in 12 bytes
      payload[0] = ((mode << 6) & 0xC0) + (((int)round(100 * (temperature + 55)) >> 8) & 0x3F); // mode - 2 bits, temp - 6 bits
      payload[1] = (int)round(100 * (temperature + 55)) & 0xFF; // temp - 1 byte
      for(int i=2; i<12; i+=2){ // flow rates - 5 * 2 byte
        payload[i] = (flows[i/2-1] >> 8) & 0xFF;
        payload[i] = flows[i/2-1] & 0xFF;
      }
      
      #ifdef DEBUG
        for(int i=0; i<12; i++){
          Serial1.print(payload[i], HEX);
          Serial1.print(" ");
        }
        Serial1.println();
      #endif
  
      // Send data over LoRaWAN using port 1 on TTN
      sendLoRa(1);

      // Last reading of the cycle becomes the first of the next one
      pressures[0] = pressures[10];
    }

    // if there is no information in the first x reading or in the entire batch
    else{
      
      #ifdef DEBUG
        Serial1.println();
        Serial1.print("left shift by: ");
        Serial1.println(index);
      #endif
      
      // left shift contents of pressures by number of empty readings in the beginning of pressures
      for(int i=0; i<=(5-index); i++){
        pressures[i] = pressures[i+index];
      }
      
      // Continue cycle from last valid measure
      counter = 5 - index;
    }

    // update mode at the end, so that the correct one is being sent over lora
    updateMode(battery);
    
  }

  STM32L0.stop(); // end of cycle, go to sleep
}


void alarmMatch(){
  STM32L0.wakeup(); // wake up from sleep
  counter == 5 ? counter = 1 : counter++;  // increment counter
}


// determine interval of next batch based on voltage on supercap
void updateInterval(float battery){
  #ifndef DEBUG
    if(battery >= 4.32){ // more than 50% - reports every 5min - 1min frequency
      interval = 60;
    }else if(battery >= 3.7){ // more than 25% - reports every 25min - 5min frequency
      interval = 300;
    }else{ // less than 25% - reports every 50min - 10min frequency
      interval = 600;
    }
  #endif
}


// update operation mode based on voltage on supercap to let back-end know about frequency
void updateMode(float battery){
  #ifndef DEBUG
    if(battery >= 4.32){
      mode = 1;
    }else if(battery >= 3.7){
      mode = 2;
    }else{
      mode = 3;
    }
  #endif
}
