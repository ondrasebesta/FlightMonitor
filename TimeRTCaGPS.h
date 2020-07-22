#ifndef TimeRTCaGPS_h
#define TimeRTCaGPS_h

#include "Wire.h"
#include "RTClib.h"
#include <TinyGPS.h>

enum RTCDateFormat
{
  RTCdatetime,
  RTCcompleteDate,
  RTCcompleteTime,
};

enum RTCComponent
{
  RTCyear,
  RTCmonth,
  RTCdate,
  RTChour,
  RTCmin,
  RTCsec
};

class TimeRTCaGPS
{
public:
  TimeRTCaGPS();
  void configure();
  // void process();
  String process(RTCDateFormat menu);
  int partoftime(RTCComponent part);
  DateTime datetime();
  uint32_t GPSsynchronization();
  float kmphcheck(void);
  int GPS_OK(void);


private:
  void smartdelay(unsigned long ms);
  void print_float(float val, float invalid, int len, int prec);
  void print_int(unsigned long val, unsigned long invalid, int len);
  uint16_t date2days(uint16_t y, uint8_t m, uint8_t d);
  long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s);
  uint32_t print_date(TinyGPS &gps);
  void print_str(const char *str, int len);

  TinyGPS gps;
  HardwareSerial hwSerial_1;
  float lat;
  float lon; // create variable for latitude and longitude object

  RTC_DS3231 rtc;
  uint32_t GpsToRtcLastSynchro;
  uint32_t GPSunixtime2;
};

#endif
