/*
  DS18B20 datasheet:
  https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf

  Library re-written based on:
  https://controllerstech.blogspot.com/2019/04/how-to-interface-ds18b20-with-stm32.html
  
  IoT Flow Meter v2.1
  3-Way Screw Terminal
  GND - Black
  D4 - Red - Power On
  D5 - Yellow - One Wire
 */


class DS18B20{
  
  private:
    int oneWire;
  
  public:
    DS18B20(int oneWire);
    uint8_t check();
    void writeData (uint8_t data);
    uint8_t readData ();
    float readCelsius();
};


DS18B20::DS18B20(int oneWire){
  
  this->oneWire = oneWire;
}

uint8_t DS18B20::check(){
  
  pinMode(oneWire, OUTPUT);
  digitalWrite(oneWire, LOW);
  delayMicroseconds(480); // delay according to datasheet

  pinMode(oneWire, INPUT);
  delayMicroseconds(80);  // delay according to datasheet

  if (!digitalRead(oneWire))  // if the pin is low i.e the presence pulse is there
  {
    delayMicroseconds(400);
    return 0;
  }
  else
  {
    delayMicroseconds(400);
    return 1;
  }
}

void DS18B20::writeData(uint8_t data){
  
  pinMode(oneWire, OUTPUT);

  for (int i=0; i<8; i++)
  {
    if ((data & (1<<i))!=0)  // if the bit is high
    {
      // write 1
      pinMode(oneWire, OUTPUT);
      digitalWrite(oneWire, LOW);
      delayMicroseconds(1);

      pinMode(oneWire, INPUT);
      delayMicroseconds(60);
    }
    else  // if the bit is low
    {
      // write 0
      pinMode(oneWire, OUTPUT);
      digitalWrite(oneWire, LOW);
      delayMicroseconds(60);

      pinMode(oneWire, INPUT);
    }
  }
}

uint8_t DS18B20::readData(){
  
  uint8_t value=0;
  pinMode(oneWire, INPUT);

  for (int i=0;i<8;i++)
  {
    pinMode(oneWire, OUTPUT);

    digitalWrite(oneWire, LOW);
    delayMicroseconds(2);

    pinMode(oneWire, INPUT);
    if (digitalRead(oneWire))
    {
      value |= 1<<i;  // read = 1
    }
    delayMicroseconds(60);
  }
  
  return value;
}

float DS18B20::readCelsius(){
  
  check();
  writeData(0xCC);  // skip ROM
  writeData(0x44);  // convert reading

  STM32L0.stop(500);
  delay(250);

  check();
  writeData(0xCC);  // skip ROM
  writeData(0xBE);  // Read Scratchpad

  uint16_t temp_l = readData();
  uint16_t temp_h = readData();
  uint16_t temp = (temp_h<<8)|temp_l;
  
  return (float)temp/16;
}
