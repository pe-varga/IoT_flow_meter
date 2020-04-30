MPR* mpr = nullptr;
DS18B20* ds18b20 = nullptr;


float readBattery(){
  
  digitalWrite(BATTERY_SWITCH, HIGH);
  delay(50);  // leave time for capacitor to charge up
  float battery = 2 * STM32L0.getVDDA() * (float)analogRead(BATTERY_ADC) / 255.0;
  digitalWrite(BATTERY_SWITCH, LOW);

  #ifdef DEBUG
    Serial1.print("Battery: ");   Serial1.println(battery);
  #endif

  return battery;
}


float readPressure(int smoothing = 10){
  
  float sum = 0;
  for(int i=0; i<smoothing; i++){
    sum += mpr->readPascal();
  }
  float avePressure = sum / (float)smoothing;
  
  #ifdef DEBUG
    Serial1.print("Pressure: ");   Serial1.println(avePressure);
  #endif

  return avePressure;
}


float readTemp(){
  
  digitalWrite(DS18B20_VDD, HIGH); 
  delay(5);
  float temperature = ds18b20->readCelsius();
  digitalWrite(DS18B20_VDD, LOW);
  
  #ifdef DEBUG
    Serial1.print("Temperature ");   Serial1.println(temperature);
  #endif

  return temperature;
}
