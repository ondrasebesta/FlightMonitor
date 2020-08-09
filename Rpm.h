#ifndef Rpm_h
#define Rpm_h

#include <Arduino.h>

struct RPM
{
  const uint8_t PIN;
  uint32_t numberCountPulses;
  bool pressed;
  uint32_t PeriodaUS;
  uint8_t checkRPM;
};

class Rpm
{
public:
void Init(uint32_t pin);
uint32_t GetRpm(void);
void Callback01s(void);
void AddNewPulse(void);
private:
uint32_t FillMaxLengthPulse(void);

};

#endif
