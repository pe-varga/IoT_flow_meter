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
bool flush[11] = {false};
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

    // Overwrite interval for faster debugging
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


  if(counter == 10){  // end of cycle
    
    // find flushed values
    checkFlush();

    // calculate slope of measurements discarding flushed ones
    float slope = getSlope();
    #ifdef DEBUG
      Serial1.print("Slope (Pa/Cycle): ");  Serial1.println(slope * 10);
      Serial1.print("Flow (mm/h): ");  Serial1.println((3600 / interval) * slope / (4 * 9.80665));
    #endif

    // if the slope is greater than half a pascal (it is not noise), convert to 100 * mm/h
    if(slope > 0.5){
      flows[cycle-1] = (int)round(100 * (3600 / interval) * slope / (4 * 9.80665));

      // find index of first average to be greater than 1 pascal/interval
      if(valid == 5){
        valid = cycle-1;
      }

    // otherwise the flow rate is very small and likely to be noise, therefore discarded
    }else{
      flows[cycle-1] = 0;
    }
    
    // last reading of the cycle becomes the first of the next one
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

  STM32L0.stop(); // go to sleep
}


void alarmMatch(){
  
  STM32L0.wakeup(); // wake up from sleep
  
  if(counter == 10){
    cycle == 5 ? cycle = 1 : cycle++;  // increment cycle 
  }
  counter == 10 ? counter = 1 : counter++;  // increment counter
}
