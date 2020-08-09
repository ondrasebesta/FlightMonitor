 #include <WiFi.h>
#include "Virtuino_ESP_WifiServer.h"
#include "WiFiVirtuino.h"
#include "TimeRTCaGPS.h"
#include "TFTdisplej.h"
#include "Thermometer.h"
#include <Arduino.h>
//#include <FS.h>
//#include "SD.h"
//#include "SPI.h"
#include "SDcard.h"
#include "ESP32FtpServer.h"
//#include "ESP8266FtpServer.h"


#define apply_Q_RPM(x)  ((x) /4)

//File myFile;

#define RED_LED 4
#define GREEN_LED 2
#define BEEPER 33
#define BUTTON_WIFI 12
#define BUTTON_RESET 27
#define BLINKINGTIME 10 // kolik sekund bude blikat červená ledka

#define MINIMUM_RPM_PULSE_LENGTH_US 2000  //
#define RPM_MIN_PULSE_PERIOD 1000000 //1000000us = 60RPM minimum value

#define BTN_STOP_ALARM 0

//#define THmin 130 //minimální teplota hlav motoru, při které může letadlo letět
//#define THmax 270 //maximální teplota hlav motoru, při které může letadlo letět
//#define TH170 170 //teplota, která odpovídá teplotě hlavy 150 stupňů
//#define TOmin 50 //minimální teplota oleje motoru, při které může letadlo letět
//#define TOmax 80 //maximální teplota oleje motoru, při které může letadlo letět
int THmin;// Minimální teplota hlavy, kdy se může letět
int deltaTmax;// Maximální povolená deltaT
int THcooling;// Maximální povolená teplota pro vzpnuti motoru
int THmax;// Maximální možná teplota motoru

uint32_t timeafterrestart=0;

bool WIFION;
String IP;
uint8_t state = 0;
uint32_t flytime = 0;
uint32_t staytime = 0;
float kmh = 0;
uint16_t temptable = 300;
uint16_t t120sec = 120;
uint16_t writetologtime = 0;
int laststate =0;
int FontColor = COLOR_RED;
int BackgroundColor = COLOR_BLACK;
String displayscreen="                                        ";
uint32_t RPMfromticks1sec;

float Tminus5=0;
float Tminus4=0;
float Tminus3=0;
float Tminus2=0;
float Tminus1=0;
float Tactual=0;
float deltaT=0;


struct RPM
{
  const uint8_t PIN;
  uint32_t numberCountPulses;
  bool pressed;
  uint32_t PeriodaUS;
  uint8_t checkRPM;
};

/*struct monitoring {
    float NTCtemp;
    int TFTicon;
    int Alarm;
    int AlarmsCount;
};*/

WiFiVirtuino WiFiVirtuino; //konstruktor
//DayOfWeek DayOfWeek; //konstruktor
TimeRTCaGPS TimeRTCaGPS; //konstruktor
Thermometer Thermometer;         //konstruktor
SDcard SDcard;         //konstruktor
FtpServer ftpSrv;   //set #define FTP_DEBUG in ESP32FtpServer.h to see ftp verbose on serial
TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);

//***RPM a ADC
RPM RPM1 = {32, 0, false}; //GPIO 32 jako RPM čidlo

hw_timer_t *timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

uint8_t interruptcounts = 0;
uint8_t interruptcounts1sec = 0;

float NTCtemp=0;
uint8_t ikona;
String ActualDate;
String ActualTime;
uint16_t SDerrorlogcount=0;
uint8_t blinking;


static uint32_t rpm_last_pulse_length=0;
static uint64_t rpm_cumulated_pulse_length=0;
static uint32_t rpm_valid=0;

uint32_t Rpm_FillMaxLengthPulse(void)
{
  rpm_cumulated_pulse_length=0;
  for(uint32_t i=0;i<100;i++)
  {
    rpm_cumulated_pulse_length -= apply_Q_RPM(rpm_cumulated_pulse_length);
    rpm_cumulated_pulse_length += RPM_MIN_PULSE_PERIOD;
  }
}

uint32_t Rpm_GetRpm(void)
{
  uint32_t temp=apply_Q_RPM(rpm_cumulated_pulse_length);
  if(0<temp && temp<=RPM_MIN_PULSE_PERIOD) return 60000000/temp;
  return 0;
}

void Rpm_Callback01s(void)
{
  rpm_cumulated_pulse_length -= apply_Q_RPM(rpm_cumulated_pulse_length);
  rpm_cumulated_pulse_length += rpm_last_pulse_length;
  if(rpm_valid)rpm_valid--; //rpm_valid is decremented every 100ms
  else rpm_cumulated_pulse_length=0;
}

void Rpm_AddNewPulse(void)
{
  static uint32_t last_micros=0;
  static uint32_t last_valid_micros=0;
  uint32_t current_micros=micros();

  if(current_micros-last_micros>MINIMUM_RPM_PULSE_LENGTH_US)
  {
    delayMicroseconds(50);
    if(digitalRead(RPM1.PIN)==0)
    {
      rpm_last_pulse_length = (current_micros-last_valid_micros);
      last_valid_micros=current_micros;
      if(rpm_valid==0)
      {
        Rpm_FillMaxLengthPulse();
      }
      rpm_valid=20;    //rpm_valid flag is decremented every 100ms
    }  
  }
  last_micros=current_micros;
}





//***Počítač pulzů z RPM v přerušení
void IRAM_ATTR isr(void *arg)
{
    Rpm_AddNewPulse();
}

void setup()
{
  Serial.begin(921600);

  //****GPIO * LED * BUTTON * BEEPER
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BEEPER, OUTPUT);
  pinMode(BUTTON_WIFI, INPUT_PULLUP);
  pinMode(BUTTON_RESET, INPUT_PULLUP);
  digitalWrite(RED_LED,LOW);
  digitalWrite(GREEN_LED,LOW);
  digitalWrite(BEEPER,LOW);

  Thermometer.Init();
  
  //Inicializace SD karty
  SDcard.setup();
  delay(1000);
   if (!SDcard.existFile(SD, "/errorlog.csv"))
   {
    SDcard.appendFile(SD, "/errorlog.csv","Datum,Čas UTC,Teplota hlavy °C,Teplota oleje °C,RPM,Error\n");
   }
  SDerrorlogcount=SDcard.errorlogcount(SD);
  
  //Precteni teplotnich mezi z SD karty 
  if (!SDcard.existFile(SD, "/THmin.txt"))
   {
    SDcard.appendFile(SD, "/THmin.txt","150\n");
   }
  THmin = SDcard.THminread(SD);
  //***********************************************
  if (!SDcard.existFile(SD, "/deltaTmax.txt"))
   {
    SDcard.appendFile(SD, "/deltaTmax.txt","5\n");
   }
  deltaTmax = SDcard.THdeltamaxread(SD);
  //************************************************
  if (!SDcard.existFile(SD, "/THcooling.txt"))
   {
    SDcard.appendFile(SD, "/THcooling.txt","170\n");
   }
  THcooling = SDcard.THcoolingread(SD);
  //************************************************
  if (!SDcard.existFile(SD, "/THmax.txt"))
   {
    SDcard.appendFile(SD, "/THmax.txt","238\n");
   }
  THmax = SDcard.THmaxread(SD);
  //*********************************************************END precteni teplotnich mezi z SD karty 
  
  WIFION=!digitalRead(BUTTON_WIFI);//test jestli je stisknuto tlačítko WIFI při startu
  //WIFION=digitalRead(BUTTON_WIFI);//test jestli je stisknuto tlačítko WIFI při startu
  if(WIFION==true)
  {
      WiFiVirtuino.setupVirtuino();
      ftpSrv.begin("esp32","esp32");    //username, password for ftp.  set ports in ESP32FtpServer.h  (default 21, 50009 for PASV)
      Serial.println("WIFI_ON");
      IP=WiFiVirtuino.DisplayIPAddress();
  }
  else
  {
  WiFi.mode(WIFI_OFF);
  btStop();
   Serial.println("WIFI_OFF");
  }
//**** RTC a GPS
  TimeRTCaGPS.configure();
//****TFT displej
#if defined(ESP32)
  hspi.begin();
  tft.begin(hspi);
#else
  tft.begin();
#endif

  //***nastavení RPM a ADC
  pinMode(RPM1.PIN, INPUT_PULLUP);                          //RPM jako vstup
  attachInterruptArg(RPM1.PIN, isr, &RPM1, CHANGE); //RPM reaguje na sestupnou hranu a spouští přerušení
  
  // Set BTN_STOP_ALARM to input mode
  pinMode(BTN_STOP_ALARM, INPUT);
  
  float tmp_temp=Thermometer.GetTemperature();
    
  Tminus5=tmp_temp;
  Tminus4=tmp_temp;
  Tminus3=tmp_temp;
  Tminus2=tmp_temp;
  Tminus1=tmp_temp;
  Tactual=tmp_temp;

  deltaT=0;
  TFTdisplayAFTERRESTART();
  delay(3000);
  TFTdisplayconstants();
  delay(3000);
}

static uint32_t timer001sec; //10 ms
static uint32_t timer01sec; //100 ms
static uint32_t timer1sec; //1 s
static uint32_t timer10sec; //10 s
static int rst_btn_cnt=0;
#define SECONDS_TO_RESET 7

void loop()
{
  uint32_t current_millis = millis();

  if ((current_millis - timer001sec) >= 10) {
    timer001sec = current_millis;
  }
  
  if ((current_millis - timer01sec) >= 100) {
    Rpm_Callback01s();
    if(BSP_IsResetBTNPressed())//restart pokud je stisknuto tlačítko RESET déle než 7 sekund
    {
      if(rst_btn_cnt>=SECONDS_TO_RESET*10) SystemRestart();
      rst_btn_cnt++;
    }
    else
    {
      rst_btn_cnt=0;
    }
    timer01sec = current_millis;
  }
  
  if ((current_millis - timer1sec) >= 1000) {

    Serial.print("RPM: ");
    Serial.println(Rpm_GetRpm());

    Thermometer.Callback1s();

      //Vyhodnocení deltaT
      Tminus5 = Tminus4;
      Tminus4 = Tminus3;
      Tminus3 = Tminus2;
      Tminus2 = Tminus1;
      Tminus1 = Tactual;
      Tactual = Thermometer.GetTemperature();
/*      Serial.print(" THmin: ");
      Serial.println(THmin);*/
      
      //Zjištění poslední synchronizace a hodnot z GPS
      uint32_t LastSynchronizationFromGPSBefore = TimeRTCaGPS.GPSsynchronization();
      deltaT = (Tminus5 - Tactual)*2; //deltaTeploty za 10 sekund v desetinách stupně
      kmh = TimeRTCaGPS.kmphcheck();
      //Vyhodnocení RPM      
      ikona = CheckStatus(Rpm_GetRpm(), Thermometer.GetTemperature(), deltaT);

      DateTime now = TimeRTCaGPS.datetime();
      ActualDate=TimeRTCaGPS.process(RTCcompleteDate);
      ActualTime=TimeRTCaGPS.process(RTCcompleteTime);
      if(WIFION==true)
      {
        WiFiVirtuino.loopVirtuino(now);//Poslání času a data do Virtuina
        WiFiVirtuino.WriteInt(1, Thermometer.GetTemperature());//TO do Virtuina
        WiFiVirtuino.WriteInt(2, deltaT);//TO do Virtuina
        WiFiVirtuino.WriteInt(3, Rpm_GetRpm());//TO do Virtuina
        WiFiVirtuino.WriteInt(4, kmh);//TO do Virtuina
        WiFiVirtuino.WriteInt(5, TimeRTCaGPS.GPS_OK());//status GPSky
        //*******************************************************
        int virtuinoswitch = WiFiVirtuino.Switch();
        int virtuinovariable = WiFiVirtuino.variable();
        int virtuinovariablewrite = WiFiVirtuino.variablewrite();
        
        if (virtuinoswitch==1)
        {
           if ((virtuinovariable != THmin) && (virtuinovariable > 50) && virtuinovariablewrite > 0)
          {
            SDcard.THminwrite(virtuinovariable);
            THmin = virtuinovariable;
            Serial.print(" THmin zapsano do SD ");
            TFTdisplayconstants();
            delay(2000);
          }
        }
        //*******************************************************
        if (virtuinoswitch==2)
          {
          if ((virtuinovariable != THmax) && (virtuinovariable > 50)&& virtuinovariablewrite > 0)
          {
           SDcard.THmaxwrite(virtuinovariable);
           THmax = virtuinovariable;
           Serial.print(" THmaxzapsano do SD ");
           TFTdisplayconstants();
           delay(2000);
          }

        }
        //*******************************************************
        if (virtuinoswitch==3)
          {
          if ((virtuinovariable != deltaTmax) && (virtuinovariable > 0)&& virtuinovariablewrite > 0)
          {
            SDcard.THdeltamaxwrite(virtuinovariable);
            deltaTmax = virtuinovariable;
            Serial.print(" deltaTmax zapsano do SD ");
            TFTdisplayconstants();
            delay(2000);
          }

        }
        //*******************************************************
        if (virtuinoswitch==4)
          {
            if ((virtuinovariable != THcooling) && (virtuinovariable > 100)&& virtuinovariablewrite > 0)
            {
            SDcard.THcoolingwrite(virtuinovariable);
            THcooling = virtuinovariable;
            Serial.print(" THcooling zapsano do SD ");
            TFTdisplayconstants();
            delay(2000);
            }
          }
      }
      timeafterrestart++;
      //DateTime now = TimeRTCaGPS.datetime();
      //String ActualDate=TimeRTCaGPS.process(RTCcompleteDate);
      //String ActualTime=TimeRTCaGPS.process(RTCcompleteTime);
      //char charActualDate[16];
      //ActualDate.toCharArray(charActualDate, 16);
      //char charActualDate[]=ActualDate;
      //SDcard.writetemp( Thermometer.GetTemperature(), NTCtemp,Rpm_GetRpm(), ActualDate, ActualTime);
      //SDcard.errorlogcount(SD);
/*      Serial.print("Ikona: ");
      Serial.print(ikona); //Serial.print("\t");
      Serial.print("   Jtemp: ");
      Serial.println(Thermometer.GetTemperature()); //Serial.print("\t");*/

      //Blikání červené ledky při porušení pravidel a pípání pípáku
      if(blinking >=1)
      {
        if ( blinking % 2 == 0)
        {
          digitalWrite(RED_LED,HIGH);//liché číslo
          digitalWrite(BEEPER,HIGH);//liché číslo
          //digitalWrite(GREEN_LED,digitalRead(BUTTON_WIFI));//liché číslo
          Serial.print("liché číslo");
        }
        else
        {
          digitalWrite(RED_LED,LOW);//sudé číslo
          digitalWrite(BEEPER,LOW);//sudé číslo
          //digitalWrite(GREEN_LED,LOW);//sudé číslo
          Serial.print("sudé číslo");
        }
       
      blinking-=1;
      }


    timer1sec = current_millis;
  }

  if ((current_millis - timer10sec) >= 10000) {

    SDcard.writetemp( Thermometer.GetTemperature(), deltaT,Rpm_GetRpm(), ActualDate, ActualTime, flytime, kmh);
    timer10sec = current_millis;
  }

   if(WIFION==true)
 {
  ftpSrv.handleFTP();        //make sure in loop you call handleFTP()!!
 }
}

bool BSP_IsResetBTNPressed(void)
{
  if(digitalRead(BUTTON_RESET)==0) return 1;
  return 0;
}

void SystemRestart()
{
  Serial.print("RESET!!!");
  digitalWrite(BEEPER,HIGH);
  digitalWrite(RED_LED,HIGH);
  digitalWrite(GREEN_LED,HIGH);
  ESP.restart();
}

uint8_t CheckStatus(uint32_t RPM, float TH, float deltaT)
{
  //ENGINE OVERHEATING
  //Serial.print("Writetologtime: ");
  //Serial.println(writetologtime);
  if (kmh < 6)
      {
        staytime++;          
      }
  else 
  {
    staytime=0;
  }
      
  if (Thermometer.GetTemperature() > THmax && timeafterrestart > 5) // 238 stupnu odpovida 260 stupnum, je zde korekce + 10 stupnu pri zpracovani a +22 stupnu na vysokych teplotach / sonda ukayuje o 32 stupnu mene ney tam je
  {
    //static
    //LCD displej ENGINE OVERHEATING
    TFTdisplayENGINEOVERHEATING();
    //displayscreen="TFTdisplayENGINEOVERHEATING";
    //LCD displej ENGINE OVERHEATING
    flytime++;
    state=state;
    return state;
  }
  //ENGINE SHOCK COOLING
  if (deltaT > deltaTmax && timeafterrestart > 5)
  {
    //static
    //ENGINE SHOCK COOLING
    TFTdisplaySHOCKCOOLING();
    //displayscreen="TFTdisplaySHOCKCOOLING";
    //ENGINE SHOCK COOLING
    flytime++;
    state=state;
    return state;
  }
  if (RPM > 2750 && timeafterrestart > 5)//Logovani prekroceni RPM max
  {
    //static
    //LCD displej ENGINE OVERHEATING
    TFTdisplayRPMTOHIGH();
    //displayscreen="TFTdisplayRPMTOHIGH";
    //LCD displej ENGINE OVERHEATING
    flytime++;
    state=state;
    return state;
  }
  if (Thermometer.GetTemperature() > THcooling && RPM < 100)
  {
    //static
    //LCD displej ENGINE OVERHEATING
    TFTdisplayENGINESTOPCOOLING();
    //displayscreen="TFTdisplayENGINESTOPCOOLING";
    //LCD displej ENGINE OVERHEATING
    flytime++;
    state=state;
    return state;
  }
  
  //Po nahozeni motoru
  if (state == 0)
  { 
    if (Thermometer.GetTemperature() >= 65) temptable = 0; //vyhodnocení teploty motoru před spuštěním/po restartu monitoru
    else if (Thermometer.GetTemperature() >= 31) temptable = 30;
    else if (Thermometer.GetTemperature() >= 20) temptable = 60;
    else if (Thermometer.GetTemperature() >= 16) temptable = 90;
    else if (Thermometer.GetTemperature() >= 11) temptable = 120;//nastaveny polovicni hodnoty
    else temptable = 150;
    //LCD displej START ENGINE
    //TFTdisplaySTARTENGINE();
    //LCD displej START ENGINE
    state = 1;
    return state;
  }
  
  //START ENGINE, RPM = 0
  if (state == 1 && RPM <= 60)
  { 
    //LCD displej START ENGINE
    //TFTdisplaySTARTENGINE();
    TFTdisplaySTARTENGINE2();
    //displayscreen="TFTdisplaySTARTENGINE";
    //LCD displej START ENGINE
    flytime=0;
    return state;
  }
  
  //WARM UP ENGINE 1000 RPM stav 2, RPM < 1000 po dobu 2 minut
  if (state <= 2 && RPM >= 100)
  { 
    //LCD displej WARM UP ENGINE 1000 RPM
    TFTdisplayWARMUPENGINE1000(RPM);
    //displayscreen="TFTdisplayWARMUPENGINE1000";
    //LCD displej WARM UP ENGINE 1000 RPM
    staytime=0; 
    flytime++;
    if (flytime>=t120sec) //120
      {
        state=3;
        return state; 
      }   
  }
  
  //WARM UP ENGINE 1100 RPM stav 3, RPM < 1100 po dobu minut dle tabulky
  if (state == 3)
  { 
    //LCD displej WARM UP ENGINE 1100 RPM
    TFTdisplayWARMUPENGINE1100(RPM);
    //displayscreen="TFTdisplayWARMUPENGINE1100";
    //LCD displej WARM UP ENGINE 1100 RPM
    staytime=0; 
    flytime++;
    if (flytime>=t120sec+temptable) //120 + template
      {
        state=4;
        return state; 
      }   
  }
  
  //WARM UP ENGINE 1200 RPM stav 4, RPM < 1200 po dobu než je na Jtemp správná teplota
  if (state == 4)
  { 
    //LCD displej WARM UP ENGINE 1200 RPM
    TFTdisplayWARMUPENGINE1200(RPM);
    //displayscreen="TFTdisplayWARMUPENGINE1200";
    //LCD displej WARM UP ENGINE 1200 RPM
    staytime=0; 
    flytime++;
    if (TH >= THmin)
      {
        state=5;
        return state; 
      }   
  }

  //READY TO FLY stav 5
  if (state == 5)
  { 
    //LCD displej READY TO FLY
    TFTdisplayREADYTOFLY();
    //displayscreen="TFTdisplayREADYTOFLY";
    //LCD displej READY TO FLY
    flytime++;
    if (staytime >= 10 && RPM <= 1200)
      {
        state=9; 
        
      }  
      return state;  
  }

  //ENGINE COOLING 1200RPM OR FLY stav 9
  /*if (state == 9 && Thermometer.GetTemperature() >= THmin)
  { 
    //LCD displej ENGINE COOLING 1200RPM OR FLY
    TFTdisplayENGINECOOLING1200RPMORFLY();
    //displayscreen="TFTdisplayENGINECOOLING1200RPMORFLY";
    //LCD displej ENGINE COOLING 1200RPM OR FLY
    flytime++;
    //Serial.print("kmh*****: ");
    //Serial.println(kmh);
    if (kmh >= 5)
      {
        state=5;
        staytime=0; 
         
      }   
      return state;
  }*/

  //ENGINE COOLING 1200RPM OR FLY stav 9
  if (state == 9 && RPM <= 1200)
  { 
    if (RPM <= 1200)
    {
      staytime++;
    }
    if (RPM <= 100)
    {
      
      TFTdisplayENGINESTOPCOOLING();
      state =9;  
      return state;
    }
    
    //LCD displej ENGINE COOLING 1200RPM OR FLY
    TFTdisplayENGINECOOLING1200RPMORFLY();
    //displayscreen="TFTdisplayENGINECOOLING1200RPMORFLY";
    //LCD displej ENGINE COOLING 1200RPM OR FLY
    flytime++;
    //Serial.print("kmh*****: ");
    //Serial.println(kmh);
    if(staytime > 120)
    {
      state = 10;
      return state;
    }
    
    if (kmh >= 5)
      {
        state=5;
        staytime=0; 
        return state;
         
      } 
      state =9;  
      return state;
  }

  //ENGINE WARM UP OR OF zrušeno protože se nedochladí nikdy
  /*if (state == 9 && Thermometer.GetTemperature() < THmin)
  { 
    //LCD displej ENGINE WARM UP OR OF
    TFTdisplayENGINEWARMUPOROFF();
    //displayscreen="TFTdisplayENGINEWARMUPOROFF";
    //LCD displej ENGINE WARM UP OR OF
    flytime++;
    //state =9;
    return state;
  }*/
  //ENGINE FLY OR OFF
  if (state == 10)
  { 
    //LCD displej TFTdisplay ENGINE FLY OR OFF
    TFTdisplayENGINEFLYOROFF();
    //displayscreen="TFTdisplayENGINEWARMUPOROFF";
    //LCD displej TFTdisplay ENGINE FLY OR OFF
    flytime++;
    //state =9;
    if (kmh >= 5)
      {
        state=5;
        staytime=0; 
        return state;
      }   
     state = 10;
    return state;
  }

  
  //KONEC
  //Vyhodnocení Stavu
/*  Serial.print("Stav: ");
  Serial.print(state);
  Serial.print("   Flytime: ");
  Serial.println(flytime);*/

  return state;
}

void TFTdisplaySTARTENGINE(void)
{
  
  statusOK();
  digitalWrite(RED_LED,LOW);
  digitalWrite(RED_LED,LOW);
  int16_t x=0, y=0, w, h;
  tft.setOrientation(1);
  if(displayscreen!="TFTdisplaySTARTENGINE")
  {
    tft.clear();
    tft.setGFXFont(&FreeSansBold18pt7b); // Set current font
    tft.getGFXTextExtent(String("START"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+48, y+5, String("START"), COLOR_GREEN); // Print string
    tft.getGFXTextExtent(String(" ENGINE"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+24, y+42+5, String(" ENGINE"), COLOR_GREEN); // Print string  
    tft.getGFXTextExtent(String(" 1000 RPM"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x, y+42+42+5, String(" 1000 RPM"), COLOR_GREEN); // Print string 
    displayscreen="TFTdisplaySTARTENGINE";
  }
}

void TFTdisplaySTARTENGINE2(void)
{
  
  statusOK();
  digitalWrite(RED_LED,LOW);
  digitalWrite(RED_LED,LOW);
  int16_t x=0, y=0, w, h;
  tft.setOrientation(1);
  if(displayscreen!="TFTdisplaySTARTENGINE")
  {
    tft.clear();
    tft.setGFXFont(&FreeSansBold18pt7b); // Set current font
    tft.getGFXTextExtent(String("START"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+48, y+5, String("START"), COLOR_GREEN); // Print string
    tft.getGFXTextExtent(String(" ENGINE"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+24, y+42+5, String(" ENGINE"), COLOR_GREEN); // Print string  
    tft.getGFXTextExtent(String(" 1000 RPM"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x, y+42+42+5, String(" 1000 RPM"), COLOR_GREEN); // Print string 

    tft.getGFXTextExtent(String("teplota: "), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x, y+42+42+42+5, String("teplota: "), COLOR_GREEN); // Print string 
    displayscreen="TFTdisplaySTARTENGINE";
  }

        tft.fillRectangle(140, 28+42+42+42+5,240,28+42+42+5, COLOR_BLACK); // Print string 
        uint16_t temp=(Thermometer.GetTemperature()+0.5);//zaokrouhleni
        tft.drawGFXText(140, 28+42+42+42+5, String(temp), COLOR_RED); // Print string 
  
}

void TFTdisplayWARMUPENGINE1000(uint32_t RPM)
{
  if(RPM <= 1100)
  {
    statusOK();
    writetologtime=0;
    int16_t x=0, y=0, w, h;
    tft.setOrientation(1);
    if(displayscreen!="TFTdisplayWARMUPENGINE1000")
    {
      tft.clear();
      tft.setGFXFont(&FreeSansBold18pt7b); // Set current font
      tft.getGFXTextExtent(String("  WARM UP"), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x, y+5, String("  WARM UP"), COLOR_GREEN); // Print string
      tft.getGFXTextExtent(String(" ENGINE"), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x+24, y+42+5, String(" ENGINE"), COLOR_GREEN); // Print string  
      tft.getGFXTextExtent(String("  1000 RPM"), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x, y+42+42+5, String("  1000 RPM"), COLOR_GREEN); // Print string
      displayscreen="TFTdisplayWARMUPENGINE1000";
    }
    if (flytime<=t120sec)
    { 
      tft.fillRectangle(60, 130, 160, 160, COLOR_BLACK);
      tft.getGFXTextExtent(String(t120sec-flytime), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x+42+42, y+42+42+42+5, String(t120sec-flytime), COLOR_GREEN); // Print string  
    }
  }
    else  if (timeafterrestart > 5)
  {
    TFTdisplayRPMTOHIGH();
    //displayscreen="TFTdisplayRPMTOHIGH";
  }
  
}

void TFTdisplayWARMUPENGINE1100(uint32_t RPM)
{

  if(RPM <= 1200)
  {
    statusOK();
    writetologtime=0;
    int16_t x=0, y=0, w, h;
    tft.setOrientation(1);
    if(displayscreen!="TFTdisplayWARMUPENGINE1100")
    {
      tft.clear();
      tft.setGFXFont(&FreeSansBold18pt7b); // Set current font
      tft.getGFXTextExtent(String("  WARM UP"), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x, y+5, String("  WARM UP"), COLOR_GREEN); // Print string
      tft.getGFXTextExtent(String(" ENGINE"), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x+24, y+42+5, String(" ENGINE"), COLOR_GREEN); // Print string  
      tft.getGFXTextExtent(String("  1100 RPM"), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x, y+42+42+5, String("  1100 RPM"), COLOR_GREEN); // Print string
      displayscreen="TFTdisplayWARMUPENGINE1100";
    }
    if (flytime <= (t120sec + temptable))
    { 
      tft.fillRectangle(60, 130, 180, 160, COLOR_BLACK);
      tft.getGFXTextExtent(String((t120sec+temptable)-flytime), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x+42+42, y+42+42+42+5, String((t120sec+temptable)-flytime), COLOR_GREEN); // Print string  
    }
  }
  else if (timeafterrestart > 5)
  {
    TFTdisplayRPMTOHIGH();
    //displayscreen="TFTdisplayRPMTOHIGH";
  }
  

}

void TFTdisplayWARMUPENGINE1200(uint32_t RPM)
{

  if(RPM <= 1300)
  {
    statusOK();
    writetologtime=0;
    int16_t x=0, y=0, w, h;
    tft.setOrientation(1);
    if(displayscreen!="TFTdisplayWARMUPENGINE1200")
    {
      tft.clear();
      tft.setGFXFont(&FreeSansBold18pt7b); // Set current font
      tft.getGFXTextExtent(String("  WARM UP"), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x, y+5, String("  WARM UP"), COLOR_GREEN); // Print string
      tft.getGFXTextExtent(String(" ENGINE"), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x+24, y+42+5, String(" ENGINE"), COLOR_GREEN); // Print string  
      tft.getGFXTextExtent(String("  1200 RPM"), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x, y+42+42+5, String("  1200 RPM"), COLOR_GREEN); // Print string
      displayscreen="TFTdisplayWARMUPENGINE1200";
    }
    int Jtempcelsius=Thermometer.GetTemperature();
    char JtempDISP[10];
    sprintf(JtempDISP, "T=%02d C", Jtempcelsius);
    tft.fillRectangle(40, 130, 200, 160, COLOR_BLACK);
    tft.getGFXTextExtent(String(JtempDISP), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+42, y+42+42+42+5, String(JtempDISP), COLOR_GREEN); // Print string  
  }
  else
  {
    TFTdisplayRPMTOHIGH();
    //displayscreen="TFTdisplayRPMTOHIGH";
  }

}

void TFTdisplayREADYTOFLY(void)
{
  statusOK();
  writetologtime=0;
  int16_t x=0, y=0, w, h;
  tft.setOrientation(1);
  if(displayscreen!="TFTdisplayREADYTOFLY")
  {
    tft.clear();
    tft.setGFXFont(&FreeSansBold18pt7b); // Set current font
    tft.getGFXTextExtent(String("   READY"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x, y+21, String("    READY"), COLOR_GREEN); // Print string
    tft.getGFXTextExtent(String("    TO"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+24, y+42+21, String("    TO"), COLOR_GREEN); // Print string  
    tft.getGFXTextExtent(String("FLY"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+42+21, y+42+42+21, String("FLY"), COLOR_GREEN); // Print string
    displayscreen="TFTdisplayREADYTOFLY";
  }
}

void TFTdisplayENGINECOOLING1200RPMORFLY(void)
{
  statusOK();
  writetologtime=0;
  int16_t x=0, y=0, w, h;
  tft.setOrientation(1);
  if(displayscreen!="TFTdisplayENGINECOOLING1200RPMORFLY")
  {
    tft.clear();
    tft.setGFXFont(&FreeSansBold18pt7b); // Set current font
    tft.getGFXTextExtent(String("ENGINE"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+42, y+5, String("ENGINE"), COLOR_GREEN); // Print string
    tft.getGFXTextExtent(String("COOLING"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+24, y+42+5, String("COOLING"), COLOR_GREEN); // Print string  
    tft.getGFXTextExtent(String("  1200 RPM"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x, y+42+42+5, String("  1200 RPM"), COLOR_GREEN); // Print string
    tft.getGFXTextExtent(String("OR FLY"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+42, y+42+42+42+5, String("OR FLY"), COLOR_GREEN); // Print string
    displayscreen="TFTdisplayENGINECOOLING1200RPMORFLY";
    //tft.fillRectangle(60, 130, 160, 160, COLOR_BLACK);
    /*tft.getGFXTextExtent(String(staytime), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+42+42, y+42+42+42+5, String(staytime), COLOR_GREEN); // Print string */
  }
}

void TFTdisplayENGINEFLYOROFF(void)//*********přijde smazat
{
  statusOK();
  writetologtime=0;
  int16_t x=0, y=0, w, h;
  tft.setOrientation(1);
  if(displayscreen!="TFTdisplayENGINEFLYOROFF")
  {
    tft.clear();
    tft.setGFXFont(&FreeSansBold18pt7b); // Set current font
    tft.getGFXTextExtent(String("ENGINE"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+42, y+5, String("ENGINE"), COLOR_GREEN); // Print string
    tft.getGFXTextExtent(String("OFF"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+42+42, y+42+5, String("OFF"), COLOR_GREEN); // Print string  
    tft.getGFXTextExtent(String("OR"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+42+42, y+42+42+5, String("OR"), COLOR_GREEN); // Print string
    tft.getGFXTextExtent(String("FLY"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+42+42, y+42+42+42+5, String("FLY"), COLOR_GREEN); // Print string
    displayscreen="TFTdisplayENGINEFLYOROFF";
  }
}

void TFTdisplayENGINEOVERHEATING(void)
{
  statusERR();
  int16_t x=0, y=0, w, h;
  tft.setOrientation(1);
  if(displayscreen!="TFTdisplayENGINEOVERHEATING")
  {
    tft.clear();
    displayscreen="TFTdisplayENGINEOVERHEATING";  
  }
  tft.setGFXFont(&FreeSansBold18pt7b); // Set current font
  tft.getGFXTextExtent(String("ENGINE"), x, y, &w, &h); // Get string extents
  y = h; // Set y position to string height
  tft.drawGFXText(x+42, y+21, String("ENGINE"), FontColor); // Print string
  tft.getGFXTextExtent(String("OVER"), x, y, &w, &h); // Get string extents
  y = h; // Set y position to string height
  tft.drawGFXText(x+42+24, y+42+21, String("OVER"), FontColor); // Print string  
  tft.getGFXTextExtent(String("HEATING"), x, y, &w, &h); // Get string extents
  y = h; // Set y position to string height
  tft.drawGFXText(x+31, y+42+42+21, String("HEATING"), FontColor); // Print string
  if (writetologtime==5)
  {
    SDcard.writeerrorlog( Thermometer.GetTemperature(), deltaT,Rpm_GetRpm(), ActualDate, ActualTime, flytime, kmh, "ENGINE OVER HEATING");
  }
  writetologtime++;

}

void TFTdisplayRPMTOHIGH(void)
{
    statusERR();
    int16_t x=0, y=0, w, h;
    tft.setOrientation(1);
    if(displayscreen!="TFTdisplayRPMTOHIGH")
    {
      tft.clear();
      displayscreen="TFTdisplayRPMTOHIGH";
    }
    tft.setGFXFont(&FreeSansBold18pt7b); // Set current font
    tft.getGFXTextExtent(String("RPM"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+42+31, y+21, String("RPM"), FontColor); // Print string
    tft.getGFXTextExtent(String("OVER"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+42+21, y+42+21, String("OVER"), FontColor); // Print string  
    tft.getGFXTextExtent(String("TO HIGH"), x, y, &w, &h); // Get string extents
    y = h; // Set y position to string height
    tft.drawGFXText(x+31, y+42+42+21, String("TO HIGH"), FontColor); // Print string
    if (writetologtime==5)
    {
      SDcard.writeerrorlog( Thermometer.GetTemperature(), deltaT,Rpm_GetRpm(), ActualDate, ActualTime, flytime, kmh, "RPM TO HIGH");
    }
    writetologtime++;
}

void TFTdisplayENGINESTOPCOOLING(void)
{
  statusERR();
  int16_t x=0, y=0, w, h;
  tft.setOrientation(1);
  if(displayscreen!="TFTdisplayENGINESTOPCOOLING")
  {
    tft.clear();
    displayscreen="TFTdisplayENGINESTOPCOOLING";
  }
  tft.setGFXFont(&FreeSansBold18pt7b); // Set current font
  tft.getGFXTextExtent(String("ENGINE"), x, y, &w, &h); // Get string extents
  y = h; // Set y position to string height
  tft.drawGFXText(x+42, y+21, String("ENGINE"), FontColor); // Print string
  tft.getGFXTextExtent(String("STOP"), x, y, &w, &h); // Get string extents
  y = h; // Set y position to string height
  tft.drawGFXText(x+42+24, y+42+21, String("STOP"), FontColor); // Print string  
  tft.getGFXTextExtent(String("COOLING"), x, y, &w, &h); // Get string extents
  y = h; // Set y position to string height
  tft.drawGFXText(x+31, y+42+42+21, String("COOLING"), FontColor); // Print string
  if (writetologtime==5)
  {
    SDcard.writeerrorlog( Thermometer.GetTemperature(), deltaT,Rpm_GetRpm(), ActualDate, ActualTime, flytime, kmh, "ENGINE STOP COOLING");
  }
  writetologtime++;

}


void TFTdisplaySHOCKCOOLING(void)
{
  statusERR();
  int16_t x=0, y=0, w, h;
  tft.setOrientation(1);
  if(displayscreen!="TFTdisplaySHOCKCOOLING")
  {
    tft.clear();
    displayscreen="TFTdisplaySHOCKCOOLING";
  }
  tft.setGFXFont(&FreeSansBold18pt7b); // Set current font
  tft.getGFXTextExtent(String("SHOCK"), x, y, &w, &h); // Get string extents
  y = h; // Set y position to string height
  tft.drawGFXText(x+42, y+42+21, String("SHOCK"), FontColor); // Print string  
  tft.getGFXTextExtent(String("COOLING"), x, y, &w, &h); // Get string extents
  y = h; // Set y position to string height
  tft.drawGFXText(x+31, y+42+42+21, String("COOLING"), FontColor); // Print string
  if (writetologtime==5)
  {
    SDcard.writeerrorlog( Thermometer.GetTemperature(), deltaT,Rpm_GetRpm(), ActualDate, ActualTime, flytime, kmh, "ENGINE SHOCK COOLING");
  }
  writetologtime++;

}

void TFTdisplayTHminwritetoSD(void)
{
  int16_t x=0, y=0, w, h;
  tft.setOrientation(1);
  tft.clear();
  displayscreen="TFTdisplaySHOCKCOOLING";
  tft.setGFXFont(&FreeSansBold18pt7b); // Set current font
  tft.getGFXTextExtent(String("CHTmin"), x, y, &w, &h); // Get string extents
  y = h; // Set y position to string height
  tft.drawGFXText(x+42, y+42+21, String("CHTmin"), COLOR_BLUE); // Print string  
  tft.getGFXTextExtent(String(THmin), x, y, &w, &h); // Get string extents
  y = h; // Set y position to string height
  tft.drawGFXText(x+31, y+42+42+21, String(THmin), COLOR_BLUE); // Print string
  delay(2000);
}



  void statusOK(void)
  {
    digitalWrite(GREEN_LED,HIGH);
    digitalWrite(RED_LED,LOW);
    digitalWrite(BEEPER,LOW);
  }

  void statusERR(void)
  {
    digitalWrite(GREEN_LED,LOW);
    if (laststate==0)
    { 
      digitalWrite(RED_LED,HIGH);
      digitalWrite(BEEPER,HIGH);
      FontColor=COLOR_RED;
      //BackgroundColor=COLOR_BLACK;
      laststate=1;
    }
    else
    { 
      digitalWrite(RED_LED,LOW);
      digitalWrite(BEEPER,LOW);
      FontColor=COLOR_WHITE;
      //BackgroundColor=COLOR_RED;
      laststate=0;
    }
  }


      void TFTdisplayAFTERRESTART(void)
      {
      DateTime now = TimeRTCaGPS.datetime();
      ActualDate=TimeRTCaGPS.process(RTCcompleteDate);
      ActualTime=TimeRTCaGPS.process(RTCcompleteTime);
      int16_t x=0, y=0, w, h;
      tft.setOrientation(1);
      tft.clear();
      tft.setGFXFont(&FreeSans12pt7b); // Set current font    
      //Zobrazení dat na displeji před startem
      //Zobrazí datum
      tft.getGFXTextExtent(ActualDate, x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x+20, y+5, ActualDate, COLOR_WHITE); // Print string 
      //Zobrazí čas
      tft.getGFXTextExtent(ActualTime, x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x+20, y+5+24, ActualTime, COLOR_WHITE); // Print string 
      //Zobrazí IP adresu
      // WiFiVirtuino.DisplayIPAddress();
      if(WIFION==true)
      {  
        tft.getGFXTextExtent(IP, x, y, &w, &h); // Get string extents
        y = h; // Set y position to string height
        tft.drawGFXText(x+20, y+5+24+24+48+48, IP, COLOR_YELLOW); // Print string 
      }    
      //Zobrazí nápis počet errorů
      tft.getGFXTextExtent("Logs count:", x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x+20, y+5+24+24+10, "Logs count:", COLOR_RED); // Print string 
      //Zobrazí počet errorů
      tft.setGFXFont(&FreeSansBold18pt7b); // Set current font
      tft.getGFXTextExtent(String(SDerrorlogcount), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x+20+50, y+5+24+24+48, String(SDerrorlogcount), COLOR_RED); // Print string 
      //Serial.println(WiFi.softAPIP());
      }
     
      void TFTdisplayconstants(void)
      {
      int16_t x=0, y=0, w, h;
      displayscreen="TFTdisplayconstants";
      tft.setOrientation(1);
      tft.clear();
      tft.setGFXFont(&FreeSans12pt7b); // Set current font    
      //Zobrazí THmin
      tft.getGFXTextExtent("CHTmin=" + String(THmin), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x+20, y+5,"CHTmin=" + String(THmin), COLOR_BLUE); // Print string 
      //Zobrazí THmax
      tft.getGFXTextExtent("CHTmax=" + String(THmax), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x+20, y+5+24,"CHTmax=" + String(THmax), COLOR_BLUE); // Print string  
      //Zobrazí deltaTmax
      tft.getGFXTextExtent("dTmax=" + String(deltaTmax), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x+20, y+5+24+24,"dTmax=" + String(deltaTmax), COLOR_BLUE); // Print string 
      //Zobrazí THmax
      tft.getGFXTextExtent("CHTcool=" + String(THcooling), x, y, &w, &h); // Get string extents
      y = h; // Set y position to string height
      tft.drawGFXText(x+20, y+5+24+24+24,"CHTcool=" + String(THcooling), COLOR_BLUE); // Print string 
      }


//******Možné stavy vyhodnocení
//***1*Ikonka že došlo k přehřátí motoru TH>270 a nebo TO>80 ***červený teploměr,                                               zápis do logu ihned
//***2*Ikonka porušení chlazení motoru po přistání RPM=0, TH>150***červený ventilátor,                                          zápis do logu ihned
//***3*Ikonka že došlo k dřívějšímu vzletu než byl TO nebo TH zahřátý RPM>=1000, TH<110 a nebo TO<50 ***červený symbol letadla, zápis do logu ihned
//***4*Před startem, po zapnutí elektřiny - alarmové slovo, kolik bylo napočítáno alarmů
//***5*Ikonka zahřívání motoru, RPM>0, RPM < 1000, TH<110 a nebo TO<50 ***modrý teploměr
//***6*Ikonka připraveno k letu RPM>0, RPM < 1000, TH>=110 a zároveň TO>=50 *** zelený teploměr
//***7*Ikonka že je vše OK a letí se RPM>=1000, TH>=110, TH<=270, TO>=50, TO<=80 *** zelený smbol letadla
//***8*Ikonka chlazení motoru po přistání RPM>0, RPM < 1000, TH>150***zelený ventilátor
//THmin 110 //minimální teplota hlav motoru, při které může letadlo letět
//THmax 270 //maximální teplota hlav motoru, při které může letadlo letět
//TOmin 50 //minimální teplota oleje motoru, při které může letadlo letět
//TOmax 80 //maximální teplota oleje motoru, při které může letadlo letět
