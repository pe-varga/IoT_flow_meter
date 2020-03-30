//#define DEBUG

#ifdef DEBUG
  #define WHITE_LED 10
#endif

#define BATTERY_ADC A0
#define BATTERY_SWITCH 2
#define MPR_CHIP_SELECT 3
#define DS18B20_VDD 4
#define DS18B20_ONEWIRE 5

// Program counters
int cycle = 1;  // 1 - 5
int counter = 0;  //  1 - 10 

// Battery management
int interval = 5;
int mode = 1;
float battery;

// Measurements
float pressures[11];
int flows[5];
int valid = 5;

#include "RTC.h"
#include "STM32L0.h"
#include "lora.h"
#include "MPR.h"
#include "DS18B20.h"
#include "sensors.h"
#include "tasks.h"

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

    interval = 2;
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

  // Read pressure for 0th data point
  pressures[counter] = readPressure(25);
  counter++;

  // start clock
  RTC.setEpoch(0);
}


void loop() {
  
  #ifdef DEBUG
    Serial1.println();
    Serial1.print("Cycle: "); Serial1.print(cycle); Serial1.print(" Counter: "); Serial1.println(counter);
  #endif

  // Determine power scheme for next batch based on battery life
  if((cycle == 5) && (counter == 10)){
    battery = readBattery();
    updateInterval(battery); 
  }

  // Schedule next alarm
  RTC.setAlarmEpoch(RTC.getEpoch() + interval);
  RTC.enableAlarm(RTC.MATCH_YYMMDDHHMMSS);
  RTC.attachInterrupt(alarmMatch);
    
  // Read pressure
  pressures[counter] = readPressure(25);

  // take average of slope, find index of first average to be greater than 1 pascal/interval, convert to 100 * mm/h
  if(counter == 10){
    float slope = linReg(pressures, 11);
    if(slope > 1){
      flows[cycle-1] = (int)round(100 * (3600 / (interval* 10)) * slope / (4 * 9.80665));
      if(valid == 5){
        valid = cycle-1;
      }
    }else{
      flows[cycle-1] = 0;
    }
    
    // Last reading of the cycle becomes the first of the next one
    pressures[0] = pressures[10];

    if(cycle == 5){  // end of batch
      
      // if there is information at the first index, send the batch along with mode and temperature
      if(valid == 0){
      
        // Read temperature
        float temperature = readTemp();
  
        // Pack payload in 12 bytes
        packPayload(temperature);
    
        // Send data over LoRaWAN using port 1 on TTN
        sendLoRa(1);

        // reset index
        valid = 5; 
      }
  
      // if there is no information in the first x reading or in the entire batch
      else{
        
        #ifdef DEBUG
          Serial1.print("left shift by: ");
          Serial1.println(valid);
        #endif
        
        // left shift contents of flows according to the index of the first information in the queue
        for(int i=0; i<(5-valid); i++){
         flows[i] = flows[i+valid];
        }
        
        // Continue cycle from last valid measure or start next batch without sending anything
        cycle = 5 - valid;

        // If information was left-shifted, send data once the batch is completed
        if(valid != 5){
          valid = 0;
        }
      }
  
      // update mode at the end, so that the correct one is being sent over lora
      updateMode(battery);
    }
  }

  STM32L0.stop(); // end of cycle, go to sleep
}


void alarmMatch(){
  
  STM32L0.wakeup(); // wake up from sleep
  
  if(counter == 10){
    cycle == 5 ? cycle = 1 : cycle++;  // increment cycle 
  }
  counter == 10 ? counter = 1 : counter++;  // increment counter
}
