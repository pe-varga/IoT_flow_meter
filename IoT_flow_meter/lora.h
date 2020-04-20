#include "LoRaWAN.h"
#include "keys.h"

void initLoRa(){
  
  #ifdef DEBUG
    Serial1.println("Joining TTN");
  #endif
  
  LoRaWAN.begin(EU868);

  // Adaptive data-rate does not work in practice, gives ~1.1s airtime
  LoRaWAN.setADR(false);
  // With the following settings airtime is reduced to ~80ms
  LoRaWAN.setDataRate(4);
  LoRaWAN.setTxPower(14);

  // Enable session saving so that there is no need to join again upon reboot
  LoRaWAN.setSaveSession(true);
  
  LoRaWAN.joinOTAA(appEui, appKey, devEui);
}


void sendFlows(){

  byte payload[12];
  payload[0] = ((mode << 6) & 0xC0) + (((int)round(100 * (temperature + 55)) >> 8) & 0x3F); // mode - 2 bits, temp - 6 bits
  payload[1] = (int)round(100 * (temperature + 55)) & 0xFF; // temp - 1 byte
  for(int i=2; i<12; i+=2){ // flow rates - 5 * 2 byte
    payload[i] = (flows[i/2-1] >> 8) & 0xFF;  // high byte
    payload[i+1] = flows[i/2-1] & 0xFF; // low byte
  }
  
  #ifdef DEBUG
    for(int i=0; i<12; i++){
      Serial1.print(payload[i], HEX);
      Serial1.print(" ");
    }
    Serial1.println();
  #endif

  int err = 0;
  if (!LoRaWAN.busy()){
        if (!LoRaWAN.linkGateways()){
            #ifdef DEBUG
              Serial1.println("Attempting to rejoin TTN");
            #endif
            LoRaWAN.rejoinOTAA();
        }
        if (LoRaWAN.joined()){
          LoRaWAN.beginPacket(6); // port 6
          LoRaWAN.write(payload, 12);
          err = LoRaWAN.endPacket();
        }
  }
  
  #ifdef DEBUG
    if (err > 0) {
      Serial1.println("Payload sent!");
      digitalWrite(WHITE_LED, HIGH); 
      delay(200);
      digitalWrite(WHITE_LED, LOW);
    } else {
      Serial1.println("Error sending payload");
    }  
  #endif
}


void sendBattery(){
  int percent = 0;
  if(battery >= 2.245){
    percent = (int)round((pow(battery, 2) - 5.04) / 20.97 * 10000);
  }
  
  byte payload[2];
  payload[0] = (percent >> 8) & 0xFF;
  payload[1] = percent & 0xFF;

  #ifdef DEBUG
    Serial1.print(payload[0], HEX);   Serial1.println(payload[1], HEX);
    Serial1.println();
  #endif

    int err = 0;
  if (!LoRaWAN.busy()){
        if (!LoRaWAN.linkGateways()){
            #ifdef DEBUG
              Serial1.println("Attempting to rejoin TTN");
            #endif
            LoRaWAN.rejoinOTAA();
        }
        if (LoRaWAN.joined()){
          LoRaWAN.beginPacket(7); // port 7
          LoRaWAN.write(payload, 2);
          err = LoRaWAN.endPacket();
        }
  }
  
  #ifdef DEBUG
    if (err > 0) {
      Serial1.println("Payload sent!");
      digitalWrite(WHITE_LED, HIGH); 
      delay(200);
      digitalWrite(WHITE_LED, LOW);
    } else {
      Serial1.println("Error sending payload");
    }  
  #endif
}
