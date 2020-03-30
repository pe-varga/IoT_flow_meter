/*  Power Scheme
 *  Always 5 flow per lora
 *  Always 10(+1) readings per flow
 *  Always 25 sample per reading
 *   
 *          LoRa          Pressure ave    Pressure reading    Smoothing
 *  50%<    every 5 min   every 1 min     every 6 sec         25 readings
 *  25%<    every 25 min  every 5 min     every 30 sec        25 readings
 *  <25%    every 50 min  every 10 min    every min           25 readings
 */ 

// determine interval of next batch based on voltage on supercap
void updateInterval(float battery){
  #ifndef DEBUG
    if(battery >= 4.32){ // more than 50%
      interval = 5;
    }else if(battery >= 3.7){ // more than 25%
      interval = 15;
    }else{ // less than 25%
      interval = 50;
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


// Pack payload in 12 bytes
void packPayload(float temperature){
  
  payload[0] = ((mode << 6) & 0xC0) + (((int)round(100 * (temperature + 55)) >> 8) & 0x3F); // mode - 2 bits, temp - 6 bits
  payload[1] = (int)round(100 * (temperature + 55)) & 0xFF; // temp - 1 byte
  for(int i=2; i<12; i+=2){ // flow rates - 5 * 2 byte
    payload[i] = (flows[i/2-1] >> 8) & 0xFF;
    payload[i+1] = flows[i/2-1] & 0xFF;
  }
  
  #ifdef DEBUG
    for(int i=0; i<12; i++){
      Serial1.print(payload[i], HEX);
      Serial1.print(" ");
    }
    Serial1.println();
  #endif
}
