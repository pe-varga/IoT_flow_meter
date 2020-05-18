#define DEBUG
//#define TEST

#ifdef TEST
  float test[5][11] = {{20.0, 21.0, 22.0, 23.0, 24.0, 25.0, 26.0, 27.0, 28.0, 29.0, 30.0},
                      {30.50, 31.0, 31.50, 32.0, 32.5, 33.0, 33.5, 34.0, 34.5, 35.0, 35.5},
                      {35.5, 35.6, 35.7, 35.8, 35.9, 36.0, 36.1, 36.2, 36.3, 36.4, 46.5},
                      {36.5, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0},
                      {19.0, 19.1, 19.2, 19.3, 19.5, 19.7, 20.0, 20.0, 20.0, 20.0, 20.0}};
#endif

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
int heartbeat = 0;

// Battery management
int interval = 6;
int mode = 1;
float battery;

// Measurements
float temperature;
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
    STM32L0.stop(200);
    digitalWrite(WHITE_LED, LOW);
  
    // Debug over Serial
    Serial1.begin(9600);
    while(!Serial1);
  
    Serial1.println("Start Initialisation");

    // Overwrite interval for faster debugging
    interval = 3;

    // Simulate sending battery reading quicker
    heartbeat = 11;
  #endif
  
  // init voltage reading on supercap
  analogReadResolution(8);
  pinMode(BATTERY_ADC, INPUT);
  pinMode(BATTERY_SWITCH, OUTPUT);
  digitalWrite(BATTERY_SWITCH, LOW);

  // read battery, and hang if it is below the undervoltage threshold
  battery = readBattery();
  #ifndef DEBUG
    if(battery < 2.245){
      STM32L0.stop(30 * 60 * 1000);
      STM32L0.reset();
    }
  #endif
  
  // set power scheme for first ever cycle
  updatePowerScheme(); 

  // join TTN over LoRaWAN
  initLoRa();

  // init pressure sensor, discard first 15 readings
  mpr = new MPR(MPR_CHIP_SELECT);
  for(int i=0; i<15; i++){
    mpr->readPascal();
    STM32L0.stop(1000);
  }

  // init temperature sensor
  pinMode(DS18B20_VDD, OUTPUT);
  digitalWrite(DS18B20_VDD, LOW); 
  ds18b20 = new DS18B20(DS18B20_ONEWIRE);

  // Read pressure for 0th data point
  pressures[counter] = readPressure(25);
  STM32L0.stop(interval * 1000);
  counter++;
  
  // start clock
  RTC.setEpoch(0);
}


void loop() {
  
  #ifdef DEBUG
    Serial1.println();
    Serial1.print("Cycle: "); Serial1.print(cycle); Serial1.print(" Counter: "); Serial1.println(counter);
  #endif

  // Get battery life to determine power scheme for the next batch
  if((cycle == 5) && (counter == 10)){
    battery = readBattery();
  }

  // Schedule next alarm
  RTC.setAlarmEpoch(RTC.getEpoch() + getInterval());
  RTC.enableAlarm(RTC.MATCH_YYMMDDHHMMSS);
  RTC.attachInterrupt(alarmMatch);
    
  // Read pressure
  pressures[counter] = readPressure(25);

  if(counter == 10){  // end of cycle

    #ifdef TEST
      // overwrite readings with test values
      for (int i=0; i<11; i++){
        pressures[i] = test[cycle-1][i];
      }
    #endif

    // find flushed values
    checkFlush();

    // calculate slope of measurements discarding flushed ones
    float slope = getSlope();
    #ifdef DEBUG
      Serial1.print("Slope (Pa/Cycle): ");  Serial1.println(slope * 10);
      Serial1.print("Flow (mm/h): ");  Serial1.println((3600 / interval) * slope / (4 * 9.80665));
    #endif

    // convert to 100 * mm/h
    flows[cycle-1] = (int)round(100 * (3600 / interval) * slope / (4 * 9.80665));

    // if the flow is less than two mm/h equivalent, it is likely to be noise, therefore it is discarded  
    if(flows[cycle-1] < 200){
      flows[cycle-1] = 0;

    // find index of first average to be valid based on the above
    }else if(valid == 5){
      valid = cycle-1;
    }
    
    // last reading of the cycle becomes the first of the next one
    pressures[0] = pressures[10];

    if(cycle == 5){  // end of batch

      // if there is information at the first index, send the batch along with mode and temperature
      if(valid == 0){
      
        // Read temperature
        temperature = readTemp();
    
        // Send flow readings to TTN
        sendFlows();

        // reset index and heartbeat
        valid = 5; 
        heartbeat = 0;
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
        }else{
          heartbeat++;
          // If nothing was sent for 12 consecutive batches(1, 5, 10 hours), send battery life to TTN
          if(heartbeat >= 12){
            sendBattery();
            heartbeat = 0;
          }
        }
      }
  
      // update power scheme at the end, so that correct mode is being sent over lora
      updatePowerScheme();
    }
  }

  STM32L0.stop(); // go to sleep
}


void alarmMatch(){
  
  STM32L0.wakeup(); // wake up from sleep
  
  if(counter == 10){
    cycle == 5 ? cycle = 1 : cycle++;  // increment cycle
    counter = 0; 
  }
  counter++;  // increment counter
}
