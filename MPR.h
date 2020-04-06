/*
  MPRLS0001PG0000SA MicroPressure Board Mount Pressure Sensor
  https://sensing.honeywell.com/honeywell-sensing-micropressure-board-mount-pressure-mpr-series-datasheet-32332628.pdf

  STM32L0 Arduino Core SPI Reference
  https://github.com/GrumpyOldPizza/ArduinoCore-stm32l0/tree/master/libraries/SPI
  
  IoT Flow Meter v2.1
  D3 - Chip Select Pin
 */


#include <SPI.h>

#define START 0xAA
#define DATA 0x00
#define NOP 0xF0


class MPR{
  
  private:
    int chipSelectPin;
  
  public:
    MPR(int chipSelectPin);
    float readPascal();
};


MPR::MPR(int chipSelectPin){
  
  SPI.begin();
  this->chipSelectPin = chipSelectPin;
  pinMode(this->chipSelectPin, OUTPUT);
}

float MPR::readPascal(){
  
  //SPI bus - 800KHz, MSB, polarity0, phase0
  SPI.beginTransaction(SPISettings(800000, MSBFIRST, SPI_MODE0));
  digitalWrite(chipSelectPin, LOW);

  byte sensorStatus = SPI.transfer(START);
  byte data0 = SPI.transfer(DATA);
  byte data1 = SPI.transfer(DATA);
  byte data2 = SPI.transfer(DATA);
  int data = ((data0 << 16) | (data1 << 8)) | data2 ;

  digitalWrite(chipSelectPin, HIGH);
  SPI.endTransaction();

  // (((count - max_count) * psi_range(1)) / count_range + min_psi(0)) * psi_to_pascal
  return(((float(data) - float(1677722)) / (float(15099494) - float(1677722)))*6894.76);
}
