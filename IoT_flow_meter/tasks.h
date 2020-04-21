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

// Determine interval of next batch based on voltage on supercap
void updatePowerScheme(){
  
  #ifndef DEBUG // do not update power scheme if debugging is in process
    if(battery >= 3.94){ // more than 50%
      mode = 1; interval = 6;
    }else if(battery >= 3.2){ // more than 25%
      mode = 2; interval = 30;
    }else{ // less than 25%
      mode = 3; interval = 60;
    }
  #endif
}


// Update operation mode based on voltage on supercap to let back-end know about frequency
int getInterval(){
  
  #ifdef DEBUG // do not update interval if debugging is in process
    return interval;
  #endif
  
  if(battery >= 3.94){
    return 6;
  }else if(battery >= 3.2){
    return 30;
  }else{
    return 60;
  }
}


// Calculate slope of values in array from start until end position using linear regression
float linReg(float y[], int start, int end){

  // generate x values and get their mean
  int ySize = end - start + 1;
  int x[ySize];
  for(int i=0; i<ySize; i++){
    x[i] = i+1;
  }
  float xMean = (1 + ySize) / 2.0;

  // get mean of y values
  float yMean = 0;
  for(int i=start; i<=end; i++){
    yMean += y[i];
  }
  yMean = yMean / (float)ySize;

  // calculate slope (b1) of y values using lin reg
  float sum1 = 0;
  float sum2 = 0;
  for(int i=0; i<ySize; i++){
    sum1 += (x[i] - xMean)*(y[start+i] - yMean);
    sum2 += pow(x[i] - xMean, 2);
  }

  return sum1/sum2;
}


// discard measurements of flushes and piece together a slope from segments of valid data
float getSlope(){
  
  int start = 0;  // first index of a valid segment in pressures
  int end = 0;  // last index of a valid segment in pressures
  int arrays = 0; // number of valid segments in pressures
  float sum = 0;  // sum of the slopes of valid segments

  // locate valid segments
  while(end < 11){
    start = end;
    while(end < 11){
      if(!flush[end]){
        if(end == 10){
            break;
        }
        end++;
      }else{
        end--;
        break;
      }
    }
    // calculate slope of valid segments
    if(start < end){
      sum += linReg(pressures, start, end);
      arrays++;
    }
    end+=2;
  }

  // get the average of the slope of valid segments
  if(arrays){
    return 10 * (sum/(float)arrays);
  }else{
    return 0.0;
  }

}


// updates global bool array 'flush', and flags the index of flushed measurements
void checkFlush(){

  // evaluation of the last measurement from the precious cycle becomes the first one
  flush[0] = flush[10];
  
  for(int i=0; i<10; i++){
    // it is considered a flush if there is a sudden drop of at least 15 pascals (one full flush is aprx -430 pascals based on experiments)
    if((pressures[i+1] - pressures[i]) < -15){
      flush[i+1] = true;
      
      #ifdef DEBUG
        Serial1.print("Flush detected at counter: "); Serial1.println(i+1);
      #endif
      
    }else{
      flush[i+1] = false;
    }
  }
}
