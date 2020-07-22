//****Program zpracovává data z GPS a ukládá je do RTC pokud nejsou starší 1200 ms (age) a zároveň nebylo zapsáno dříve jak před hodinou
//****Po spuštění programu je hodnota rozdílu poslední synchronizace rovna UNIXtime, takže po spuštění je vždy RTC synchronizováno ihned po prvním správném přijetí času z GPS. Další synchronizace až zase za hodinu.

#include "TimeRTCaGPS.h"

#define RX1 16
#define TX1 17

float kmph;
int GPSOK=0;

const char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

TimeRTCaGPS::TimeRTCaGPS() : hwSerial_1(1),
                             lat(28.5458),
                             lon(77.1703)
{
}

void TimeRTCaGPS::configure()
{

#ifndef ESP8266
  while (!Serial)
    ; // for Leonardo/Micro/Zero
#endif

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }

  //if (rtc.lostPower()) {
  //Serial.println("RTC lost power, lets set the time!");
  // following line sets the RTC to the date &amp; time this sketch was compiled
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //rtc.adjust(DateTime(F(10 26 2009), F(12:34:56)));
  //rtc.adjust(175890);
  // This line sets the RTC with an explicit date &amp; time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  //}
  //Serial2.begin(9600, SERIAL_8N1, RX1, TX1);
  hwSerial_1.begin(9600, SERIAL_8N1, RX1, TX1);
}

int TimeRTCaGPS::partoftime(RTCComponent part)
{
  DateTime now = rtc.now();
  //sprintf(RTCdatetimeString, "%02d/%02d/%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  switch (part)
  {
  case RTCyear:
    return now.year();
  case RTCmonth:
    return now.month();
  case RTCdate:
    return now.day();
  case RTChour:
    return now.hour();
  case RTCmin:
    return now.minute();
  case RTCsec:
    return now.second();
  default:
    return 0;
  }
}

//void TimeRTCaGPS::process() {
String TimeRTCaGPS::process(RTCDateFormat menu)
{
  char RTCdatetimeString[32];
  char RTCdateString[16];
  char RTCtimeString[16];
  DateTime now = rtc.now();
  sprintf(RTCdatetimeString, "%02d.%02d.%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  sprintf(RTCdateString, "%02d_%02d_%02d", now.year(), now.month(), now.day());
  sprintf(RTCtimeString, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  
 // Serial.print("RTC date time: ");
  /*Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);*/
 // Serial.println(RTCdatetimeString);
  

  /*Serial.print(" since midnight 1/1/1970 = ");
    Serial.print(now.unixtime());
    Serial.print("s = ");
    Serial.print(now.unixtime() / 86400L);
    Serial.println("d");*/
  //Serial.println();
  //print_date(gps);
  //Serial.println();
  //delay(1000);
  //return rtcsz;

  if (menu == RTCdatetime)
  {
    return RTCdatetimeString;
  }
  else if (menu == RTCcompleteDate)
  {
    return RTCdateString;
  }
  else
  {
    return RTCtimeString;
  }
}

uint32_t TimeRTCaGPS::GPSsynchronization()
{
  uint32_t prenos = print_date(gps);
  return prenos;
}

DateTime TimeRTCaGPS::datetime()
{
  return rtc.now();
}

void TimeRTCaGPS::smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (hwSerial_1.available())
      gps.encode(hwSerial_1.read());
  } while (millis() - start < ms);
}

void TimeRTCaGPS::print_float(float val, float invalid, int len, int prec)
{
  if (val == invalid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i = flen; i < len; ++i)
      Serial.print(' ');
  }
  smartdelay(0);
}

void TimeRTCaGPS::print_int(unsigned long val, unsigned long invalid, int len)
{
  char sz[32];
  if (val == invalid)
    strcpy(sz, "*******");
  else
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i = strlen(sz); i < len; ++i)
    sz[i] = ' ';
  if (len > 0)
    sz[len - 1] = ' ';
  Serial.print(sz);
  smartdelay(0);
}

////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

const uint8_t daysInMonth[] PROGMEM = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// number of days since 2000/01/01, valid for 2001..2099
uint16_t TimeRTCaGPS::date2days(uint16_t y, uint8_t m, uint8_t d)
{
  if (y >= 2000)
    y -= 2000;
  uint16_t days = d;
  for (uint8_t i = 1; i < m; ++i)
    days += pgm_read_byte(daysInMonth + i - 1);
  if (m > 2 && y % 4 == 0)
    ++days;
  return days + 365 * y + (y + 3) / 4 - 1;
}

long TimeRTCaGPS::time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s)
{
  return ((days * 24L + h) * 60 + m) * 60 + s;
}

////////////////////////////////////////////////////////////////////////////////

uint32_t TimeRTCaGPS::print_date(TinyGPS &gps)
{
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  kmph= gps.f_speed_kmph();
 /* Serial.print("Speed: ");
  Serial.print(kmph);
  Serial.println(" km/h");*/
  
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age == TinyGPS::GPS_INVALID_AGE)
  {
 //   Serial.println("GPS without data ");
    GPSOK=0;
  }
  else
  {
    GPSOK=1;
    char sz[32];
 //   Serial.print("GPS date time: ");
 //   sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    DateTime now = (second + (minute * 60) + (hour * 60 * 60) + (day * 60 * 60 * 24) + (month * 60 * 60 * 24 * 12) + ((year)*60 * 60 * 24 * 365) + 946684800);
  //  Serial.println(sz);
    //Serial.println(now.unixtime());
    //Serial.println(age);
    //Serial.println(age/1000L);
    //Serial.print("date2days");
    //static uint32_t pokus=(SECONDS_PER_DAY*date2days(year, month, day))+SECONDS_FROM_1970_TO_2000+(3600*hour+60*minute+second-(age/1000));
    uint32_t GPSunixtime = (SECONDS_FROM_1970_TO_2000 + (date2days(year, month, day) * SECONDS_PER_DAY) + ((hour * 3600) + (minute * 60) + second));
    GPSunixtime2 = GPSunixtime;
    //long pokus2=time2long(date2days(year, month, day)+SECONDS_FROM_1970_TO_2000/SECONDS_PER_DAY, hour, minute, second);
    //Serial.println(GPSunixtime);
    //Serial.println(pokus2);

    // Kdy bylo naposledy přeneseno z GPS do RTC
    static uint32_t GPSlastUNIXTIME = 0;
    uint32_t GPSnewUNIXTIME = GPSunixtime;
    GpsToRtcLastSynchro = GPSnewUNIXTIME - GPSlastUNIXTIME;

    Serial.print("GPS to RTC syncro before: ");
    Serial.print(GpsToRtcLastSynchro);
    Serial.println(" seconds");
    if (age < 1100 && GpsToRtcLastSynchro > 3600) //pokud je časový údaj z GPS mladší než 1200 ms, zapíše se jeho hodnota do RTC s korekcí cca +- 1 sekundy, zapisuje se jen pokud je synchronizace starší jak 1 hodina
    {
      Serial.println("GPS time to RTC saved");
      rtc.adjust(DateTime(GPSunixtime));
      GPSlastUNIXTIME = GPSnewUNIXTIME;
    }
  }
  //print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
  smartdelay(0);
  return GpsToRtcLastSynchro;
}

float TimeRTCaGPS::kmphcheck(void)
{
  float kmphspeed=kmph;
  return kmphspeed;
}

void TimeRTCaGPS::print_str(const char *str, int len)
{
  int slen = strlen(str);
  for (int i = 0; i < len; ++i)
    Serial.print(i < slen ? str[i] : ' ');
  smartdelay(0);
}

int TimeRTCaGPS::GPS_OK(void)
{
  return GPSOK;
}
