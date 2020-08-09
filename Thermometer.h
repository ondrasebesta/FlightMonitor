#ifndef Thermometer_h
#define Thermometer_h

#include <Arduino.h>

class Thermometer
{
public:

  byte ADCeval(float *temperature);
  void TempSensorInit();
 

private:
    
};

#endif
