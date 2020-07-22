#ifndef RPMaADC_h
#define RPMaADC_h

#include <Arduino.h>
#include <Smoothed.h> // For filtering ADC inputs



//Smoothed <float> ADCSensorJ;
//Smoothed <float> ADCSensorNTC;

#define BTN_STOP_ALARM 0
#define Jtemppin A6
#define NTCtemppin A7

/*enum RTCDateFormat {
  RTCdatetime,
  RTCcompleteDate,
  RTCcompleteTime,
};

enum RTCComponent {
  RTCyear,
  RTCmonth,
  RTCdate,
  RTChour,
  RTCmin,
  RTCsec
};*/

/*struct RPM {
    const uint8_t PIN;
    uint32_t numberCountPulses;
    bool pressed;
    uint32_t PeriodaUS;
    uint8_t checkRPM;
};*/

class RPMaADC
{
public:
  /*TimeRTCaGPS();
    void configure();
   // void process();
    String  process(RTCDateFormat menu);
    int partoftime(RTCComponent part);
    DateTime datetime();
    uint32_t GPSsynchronization();*/
  /*void IRAM_ATTR isr(void* arg);
	void IRAM_ATTR onTimer();
	void setup();*/
  double ReadVoltage(byte pin);
  byte ADCeval(float *temperature);
  float RPMeval(float RPMactual); //Hodnota RPM)
  void ADsetup();
  void TempSensorInit();
  /*void RPMeval(uint32_t numberCountPulses, int32_t PeriodaUS);
  uint32_t numberCountPulses;
  uint32_t PeriodaUS;*/
  //volatile uint32_t isrCounter = 0;
  //volatile uint32_t lastIsrAt = 0;
  /*uint16_t tempSensor1=0;
	uint16_t tempSensor2=0;
	uint16_t deltaTemp1=0;
	uint16_t deltaTemp2=0;
	uint32_t Teplota0=0;*/

private:
  /* void smartdelay(unsigned long ms);
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
    uint32_t GPSunixtime2;*/
    
};

#endif
