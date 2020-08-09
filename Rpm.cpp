#include "Rpm.h"

#define MINIMUM_RPM_PULSE_LENGTH_US 2000  //
#define RPM_MIN_PULSE_PERIOD 1000000 //1000000us = 60RPM minimum value

#define apply_Q_RPM(x)  ((x) /4)




static uint32_t rpm_last_pulse_length=0;
static uint64_t rpm_cumulated_pulse_length=0;
static uint32_t rpm_valid=0;
static uint32_t rpm_pin=0;

void Rpm::Init(uint32_t pin)
{
  rpm_pin=pin;
}

uint32_t Rpm::FillMaxLengthPulse(void)
{
  rpm_cumulated_pulse_length=0;
  for(uint32_t i=0;i<100;i++)
  {
    rpm_cumulated_pulse_length -= apply_Q_RPM(rpm_cumulated_pulse_length);
    rpm_cumulated_pulse_length += RPM_MIN_PULSE_PERIOD;
  }
}

uint32_t Rpm::GetRpm(void)
{
  uint32_t temp=apply_Q_RPM(rpm_cumulated_pulse_length);
  if(0<temp && temp<=RPM_MIN_PULSE_PERIOD) return 60000000/temp;
  return 0;
}

void Rpm::Callback01s(void)
{
  rpm_cumulated_pulse_length -= apply_Q_RPM(rpm_cumulated_pulse_length);
  rpm_cumulated_pulse_length += rpm_last_pulse_length;
  if(rpm_valid)rpm_valid--; //rpm_valid is decremented every 100ms
  else rpm_cumulated_pulse_length=0;
}

void Rpm::AddNewPulse(void)
{
  static uint32_t last_micros=0;
  static uint32_t last_valid_micros=0;
  uint32_t current_micros=micros();

  if(current_micros-last_micros>MINIMUM_RPM_PULSE_LENGTH_US)
  {
    delayMicroseconds(50);
    if(digitalRead(rpm_pin)==0)
    {
      rpm_last_pulse_length = (current_micros-last_valid_micros);
      last_valid_micros=current_micros;
      if(rpm_valid==0)
      {
        FillMaxLengthPulse();
      }
      rpm_valid=20;    //rpm_valid flag is decremented every 100ms
    }  
  }
  last_micros=current_micros;
}
