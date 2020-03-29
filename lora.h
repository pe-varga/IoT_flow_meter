#include "LoRaWAN.h"
#include "keys.h"


byte payload[12];


void initLoRa(){
  
  #ifdef DEBUG
    Serial1.println("Joining TTN");
  #endif
  
  LoRaWAN.begin(EU868);
  LoRaWAN.setADR(true);
  LoRaWAN.setSaveSession(true);
  LoRaWAN.joinOTAA(appEui, appKey, devEui);
}


void sendLoRa(int port){
  
  int err = 0;
  if (!LoRaWAN.busy()){
        if (!LoRaWAN.linkGateways()){
            #ifdef DEBUG
              Serial1.println("Attempting to rejoin TTN");
            #endif
            LoRaWAN.rejoinOTAA();
        }
        if (LoRaWAN.joined()){
          LoRaWAN.beginPacket(port);
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
