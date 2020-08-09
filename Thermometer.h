#ifndef Thermometer_h
#define Thermometer_h

#include <Arduino.h>

class Thermometer
{
public:
  void Init(void);
  void Callback1s(void);
  float GetTemperature(void);
private:
  byte ADCeval(float *temperature);
};

#endif
