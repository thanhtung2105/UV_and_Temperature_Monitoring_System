//DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
//NRF
#include <SPI.h>
#include <RF24.h>

//----------Declare Variables-------------//
//DS18B20
const int oneWireBus = 0;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
//UV - ML8511
int UVOUT = A0;   //Output from the sensor
int REF_3V3 = 10; //3.3V power on the board - SD3
//NRF24
RF24 myRadio(2, 15); //CE - CSN pin
const byte address[6] = "00001";

//Other Variables
float value[2];

//----------Setup-------------//
void setup()
{
    //ESP8266
    Serial.begin(115200);
    SPI.begin();
    //UV - ML8511
    pinMode(UVOUT, INPUT);
    pinMode(REF_3V3, INPUT);
    //NRF24
    myRadio.begin();
    myRadio.setRetries(15, 15);
    myRadio.setPALevel(RF24_PA_MAX);
    myRadio.openWritingPipe(address);
}

//----------Main-------------//
void loop()
{
    sensors.requestTemperatures();
    float temperatureC = sensors.getTempCByIndex(0);
    value[0] = temperatureC;
    //    float temperatureF = sensors.getTempFByIndex(0);

    //----------Calculate for UV intensity-------------//
    int uvLevel = averageAnalogRead(UVOUT);
    int refLevel = averageAnalogRead(REF_3V3);
    //Use the 3.3V power pin as a reference to get a very accurate output value from sensor
    float outputVoltage = 3.3 / refLevel * uvLevel;
    float uvIntensity = mapfloat(outputVoltage, 0.99, 2.8, 0.0, 15.0); //Convert the voltage to a UV intensity level
    value[1] = uvIntensity;

    //Send data to Module - indoor:
    Serial.print("Outdoor temperature: ");
    Serial.print(value[0]);
    Serial.print("\t");
    Serial.print("UV intensity: ");
    Serial.print(value[1]);
    Serial.print("\t");
    Serial.println();

    myRadio.write(&value, sizeof(value));
    delay(500);
}

//----------Takes an average of readings on a given pin-------------//
//Returns the average
int averageAnalogRead(int pinToRead)
{
    byte numberOfReadings = 8;
    unsigned int runningValue = 0;

    for (int x = 0; x < numberOfReadings; x++)
        runningValue += analogRead(pinToRead);
    runningValue /= numberOfReadings;

    return (runningValue);
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
