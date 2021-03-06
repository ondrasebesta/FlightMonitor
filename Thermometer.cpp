#include "Thermometer.h"

#include "SparkFun_MCP9600.h"
MCP9600 tempSensor;

#define apply_Q_temperature(x)  ((x) /8)
float CumulatedTemperature=0;

void Thermometer::Init(void)
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


    float tmp_temp=0;
  while(0== ADCeval(&tmp_temp));

  Serial.print("Thermocouple INIT temperature: ");
  Serial.print(tmp_temp);
  Serial.print(" °C");
  Serial.println(); 

  for(uint8_t i=0;i<100;i++)
  {
     CumulatedTemperature -= apply_Q_temperature(CumulatedTemperature);
     CumulatedTemperature += tmp_temp;
  }
  
}

float Thermometer::GetTemperature(void)
{
  return apply_Q_temperature(CumulatedTemperature); //CumulatedTemperature se prubezne aktualizuje ve smycce
}

void Thermometer::Callback1s(void)
{
  float tmp_temp=0;
  if (ADCeval(&tmp_temp))
  {
    CumulatedTemperature -= apply_Q_temperature(CumulatedTemperature);
    CumulatedTemperature += tmp_temp;
  }
}

byte Thermometer::ADCeval(float *temperature)
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
}
