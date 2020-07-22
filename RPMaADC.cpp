#include "RPMaADC.h"
//#include <Arduino.h>

Smoothed<float> ADCSensorJ;
Smoothed<float> ADCSensorNTC;
Smoothed<float> RPMSensor;

#include "SparkFun_MCP9600.h"
MCP9600 tempSensor;

void RPMaADC::TempSensorInit()
{
  Wire.begin();
  Wire.setClock(100000);
  tempSensor.begin();       // Uses the default address (0x60) for MCP9600
//check if the sensor is connected
  if(tempSensor.isConnected()){
      Serial.println("MCP9600 will acknowledge!");
  }
  else {
      Serial.println("MCP9600 did not acknowledge! Freezing.");
      while(1); //hang forever
  }

  //check if the Device ID is correct
  if(tempSensor.checkDeviceID()){
      Serial.println("MCP9600 ID is correct!");        
  }
  else {
      Serial.println("MCP9600 ID is not correct! Freezing.");
      while(1); //hang forever
  }

//set the resolution on the ambient (cold) junction
  tempSensor.setAmbientResolution(RES_ZERO_POINT_0625);
  tempSensor.setThermocoupleResolution(RES_18_BIT);

//change the thermocouple type being used
  Serial.println("Setting Thermocouple Type!");
  tempSensor.setThermocoupleType(TYPE_J);
  //make sure the type was set correctly!
  if(tempSensor.getThermocoupleType() == TYPE_J){
      Serial.println("Thermocouple Type set sucessfully!");
  }
  else{
      Serial.println("Setting Thermocouple Type failed!");
  }
  
uint8_t coefficient = 7;
//print the filter coefficient that's about to be set
    Serial.print("Setting Filter coefficient to ");
    Serial.print(coefficient);
    Serial.println("!");
    tempSensor.setFilterCoefficient(coefficient);
    //tell us if the coefficient was set sucessfully
    if(tempSensor.getFilterCoefficient() == coefficient){
        Serial.println("Filter Coefficient set sucessfully!");
    }
    else{
        Serial.println("Setting filter coefficient failed!");
        Serial.println("The value of the coefficient is: ");
        Serial.println(tempSensor.getFilterCoefficient(), BIN);
    }

  
  
}




/*RPM RPM1 = {32, 0, false}; //GPIO 32 jako RPM čidlo

hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void RPMaADC::IRAM_ATTR isr(void* arg) {
    RPM* s = static_cast<RPM*>(arg);
    s->numberCountPulses += 1;
    s->pressed = true;
    //Mereni periody
    static uint32_t lastMicros = 0;
    uint32_t newMicros = micros();
    s->PeriodaUS = newMicros - lastMicros;
    lastMicros = newMicros;
    //Počet měření, kdy zůstane zachováno poslední RPM pokud nepřišly nové pulzy
    s->checkRPM=2;
    
}


void RPMaADC::IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}*/

/*void RPMaADC::setup() {
    pinMode(RPM1.PIN, INPUT);
    attachInterruptArg(RPM1.PIN, isr, &RPM1, FALLING);

    // Set BTN_STOP_ALARM to input mode
    pinMode(BTN_STOP_ALARM, INPUT);

    // Create semaphore to inform us when the timer has fired
    timerSemaphore = xSemaphoreCreateBinary();

    // Use 1st timer of 4 (counted from zero).
    // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
    // info).
    timer = timerBegin(0, 80, true);

    // Attach onTimer function to our timer.
    timerAttachInterrupt(timer, &onTimer, true);

    // Set alarm to call onTimer function every second (value in microseconds).
    // Repeat the alarm (third parameter)
    timerAlarmWrite(timer, 100000, true);

    // Start an alarm
    timerAlarmEnable(timer);

   // nastavení parametrů ADC filtrace
   ADCSensorJ.begin(SMOOTHED_EXPONENTIAL, 10);
   ADCSensorNTC.begin(SMOOTHED_EXPONENTIAL, 10);
}*/
double RPMaADC::ReadVoltage(byte pin)
{
  double reading = analogRead(pin); // Reference voltage is 3v3 so maximum reading is 3v3 = 4095 in range 0 to 4095
  if (reading < 1 || reading > 4095)
    return 0;
  // return -0.000000000009824 * pow(reading,3) + 0.000000016557283 * pow(reading,2) + 0.000854596860691 * reading + 0.065440348345433;
  return -0.000000000000016 * pow(reading, 4) + 0.000000000118171 * pow(reading, 3) - 0.000000301211691 * pow(reading, 2) + 0.001109019271794 * reading + 0.034143524634089;
} // Added an improved polynomial, use either, comment out as required

/*void RPMaADC::loop() {
    //***********************Vyhodnocení zda došlo k přerušení, které je každých 100 milisekund*********************************
    if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE){
      uint32_t isrCount = 0, isrTime = 0;
      // Read the interrupt count and time
      portENTER_CRITICAL(&timerMux);
      isrCount = isrCounter;
      isrTime = lastIsrAt;
      portEXIT_CRITICAL(&timerMux);
      //***********************Vyhodnocení otáček motoru RPM*********************************
      if(RPM1.checkRPM>=1)//Kontrola aby při nízkých otáčkách kdy nepřijde žádný pulz nekleslo RPM na nulu, ale pamatovalo si předchozí hodnotu několik měření (měření je 10x za sekundu)
        {
        RPM1.checkRPM--;
        }
        else
          {
          RPM1.PeriodaUS=0;
          }
        if (RPM1.PeriodaUS<10000)
          {
            RPM1.PeriodaUS=0; //Kontrola, jestli nedošlo k zákmitu, aby RPM nebylo vyšší než 6000 RPM
          }
       Serial.printf("Freq = %u Hz, ", RPM1.numberCountPulses);
       Serial.printf("Perioda = %u us, ", RPM1.PeriodaUS);
       uint32_t Frequency=0;
       if (RPM1.PeriodaUS>=1)
         {
           Frequency = 1000000000/RPM1.PeriodaUS; // Kontrola, kdy nejsou pulzy, aby nebylo děleno nulou
         }
       Serial.printf("Frekvence = %u Hz, ", Frequency);
       Serial.printf("RPM = %u Ot/min, ", Frequency*60/1000);
       RPM1.numberCountPulses=0;

       //***********************Vyhodnocení teplotních čidel spolu s filtrací hodnot*********************************
       //J - Teplotní čidlo hlav motoru, provozní teplota 110 - 270 st.c, teplota = Uadc * 200
       //NTC - Teplotní čidlo oleje, provozní teplota 50 - 80 st.c
       
       float ADCcurrentSensorValueJ = ReadVoltage(A6); //Vyhodnocení čidla J na pinu 34, vrátí kalibrovanou hodnotu napětí, napětí pod 0,3 V je nelineární
       float ADCcurrentSensorValueNTC = ReadVoltage(A7); //Vyhodnocení čidla NTC na pinu 35, vrátí kalibrovanou hodnotu napětí, napětí pod 0,3 V je nelineární
  
       // Add the new value to both sensor value stores
       ADCSensorJ.add(ADCcurrentSensorValueJ);
       ADCSensorNTC.add(ADCcurrentSensorValueNTC);   
  
       // Get the smoothed values
       float JsmoothedSensorValueExp = ADCSensorJ.get();
       float NTCsmoothedSensorValueExp = ADCSensorNTC.get();    
  
       // Output the smoothed values to the serial stream. Open the Arduino IDE Serial plotter to see the effects of the smoothing methods.
       Serial.print(JsmoothedSensorValueExp); Serial.print("\t"); Serial.println(NTCsmoothedSensorValueExp);
    
    }
 
}*/

void RPMaADC::ADsetup()
{
  // nastavení parametrů ADC filtrace
  ADCSensorJ.begin(SMOOTHED_EXPONENTIAL, 10);
  ADCSensorNTC.begin(SMOOTHED_EXPONENTIAL, 10);
  RPMSensor.begin(SMOOTHED_AVERAGE, 4);
}

//***********************Vyhodnocení teplotních čidel spolu s filtrací hodnot*********************************
//J - Teplotní čidlo hlav motoru, provozní teplota 110 - 270 st.c, teplota = Uadc * 200
//NTC - Teplotní čidlo oleje, provozní teplota 50 - 80 st.c
byte RPMaADC::ADCeval(float *temperature)
{

static float tmp_temperature=0;
 
  if(tempSensor.available()){
    tmp_temperature=tempSensor.getThermocoupleTemp();
   /* Serial.print("Thermocouple temperature: ");
    Serial.print(tmp_temperature);
    Serial.print(" °C");
    Serial.println(); */
    if(tmp_temperature==16.0625)  //MCP9600 občas vrací hodnotu 16.0625 (0b0000000100000001)
    {
      Serial.println("Thermocouple temperature: error");
      return 0;
    }
    *temperature=tmp_temperature;
    return 1;
    }
    else
    {
      //Serial.println("Thermocouple temperature: not ready");
      return 0;
    }


uint8_t pin;

  if (pin == Jtemppin)
  {
    float ADCcurrentSensorValueJ = ReadVoltage(pin); //Vyhodnocení čidla J na pinu 34, vrátí kalibrovanou hodnotu napětí, napětí pod 0,3 V je nelineární
    ADCSensorJ.add(ADCcurrentSensorValueJ);
    float JsmoothedSensorValueExp = ADCSensorJ.get();
    //Serial.print(JsmoothedSensorValueExp); //Serial.print("\t");
    //Serial.print("     ");
    return JsmoothedSensorValueExp * 200 * 1.0465 + 1.1628 + 10; //Přepočet z napětí na teplotu t=5mV/C
  }
  else
  {
    float ADCcurrentSensorValueNTC = ReadVoltage(pin); //Vyhodnocení čidla NTC na pinu 35, vrátí kalibrovanou hodnotu napětí, napětí pod 0,3 V je nelineární
    ADCSensorNTC.add(ADCcurrentSensorValueNTC);
    float NTCsmoothedSensorValueExp = ADCSensorNTC.get();
    //return NTCsmoothedSensorValueExp ; //y = -419,84x3 + 992,57x2 - 843,64x + 309,92
    //float NTCtemp = NTCsmoothedSensorValueExp;//- 419.84 * pow(NTCsmoothedSensorValueExp, 3) + 992.57 * pow(NTCsmoothedSensorValueExp, 2) - 843.64 * NTCsmoothedSensorValueExp + 309.92;
    //y = -58,84ln(x) + 42,109
    float NTCtemp = -58.84 * log(NTCsmoothedSensorValueExp) + 42.109;
    //Serial.println(NTCsmoothedSensorValueExp);
    //Serial.print("     ");
    return NTCtemp;
  }
  //Output the smoothed values to the serial stream. Open the Arduino IDE Serial plotter to see the effects of the smoothing methods.
}

float RPMaADC::RPMeval(float RPMactual) //Hodnota RPM)
{
    float RPMSensorActual = RPMactual; //Vyhodnocení čidla J na pinu 34, vrátí kalibrovanou hodnotu napětí, napětí pod 0,3 V je nelineární
    RPMSensor.add(RPMSensorActual);
    float RPMsmoothedSensorValueExp = RPMSensor.get();
    //Serial.print(RPMsmoothedSensorValueExp); //Serial.print("\t");
    //Serial.println("     ");
    return RPMsmoothedSensorValueExp; //Zprůměrovaná hodnota RPM 
}
