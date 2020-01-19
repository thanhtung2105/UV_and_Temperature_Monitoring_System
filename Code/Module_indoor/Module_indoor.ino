//ESP8266
#include <ESP8266WiFi.h>
//BLYNK
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
//DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>
//OLED
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2lib.h>
//NRF24
#include <RF24.h>

//----------Initiate OLED-------------//
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

//----------Token for Blynk-------------//
char auth[] = "IASph3ippkz65UwqYimcZOw1p-b1a9dF";
WidgetLCD lcd(V3);

//----------Setup Wifi Connect-------------//
char ssid[] = "VLTH-E205";
char pass[] = "vlthemotiv205";

//----------Declare Variables-------------//
//DS18B20
const int oneWireBus = 0;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
//NRF24
RF24 radio(2, 15); //ce,cs pin
const byte address[6] = "00001";
float value[2];

//Others Variables:
String inTemp, outTemp;
float temperature_outdoor = 0, uvIntensity = 0;
int draw_state = 0;
unsigned long previousMillis = 0;
long interval = 3000;

//----------Draw Temperature Icon-------------//
#define Temperature_20Icon_width 27
#define Temperature_20Icon_height 47
static const unsigned char Temperature_20Icon_bits[] U8X8_PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x80, 0x7f, 0x00, 0x00,
    0xc0, 0xe1, 0x00, 0x00, 0xe0, 0xc0, 0x01, 0x00, 0x60, 0x80, 0xf9, 0x03,
    0x60, 0x80, 0x01, 0x00, 0x60, 0x80, 0x01, 0x00, 0x60, 0x80, 0x79, 0x00,
    0x60, 0x80, 0x01, 0x00, 0x60, 0x80, 0x01, 0x00, 0x60, 0x80, 0xf9, 0x03,
    0x60, 0x80, 0x01, 0x00, 0x60, 0x80, 0x01, 0x00, 0x60, 0x8c, 0x79, 0x00,
    0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0xf9, 0x03,
    0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0x79, 0x00,
    0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0xf9, 0x03,
    0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0x01, 0x00,
    0x70, 0x9e, 0x03, 0x00, 0x38, 0x1e, 0x07, 0x00, 0x18, 0x3e, 0x0e, 0x00,
    0x1c, 0x3f, 0x0c, 0x00, 0x0c, 0x7f, 0x18, 0x00, 0x8c, 0xff, 0x18, 0x00,
    0x8e, 0xff, 0x38, 0x00, 0xc6, 0xff, 0x31, 0x00, 0xc6, 0xff, 0x31, 0x00,
    0xc6, 0xff, 0x31, 0x00, 0x8e, 0xff, 0x38, 0x00, 0x8c, 0xff, 0x18, 0x00,
    0x0c, 0x7f, 0x1c, 0x00, 0x3c, 0x1c, 0x0e, 0x00, 0x78, 0x00, 0x06, 0x00,
    0xe0, 0x80, 0x07, 0x00, 0xe0, 0xff, 0x03, 0x00, 0x80, 0xff, 0x00, 0x00,
    0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//----------Setup-------------//
void setup()
{
    //OLED
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //initialize with the I2C addr 0x3C (128x64)
    display.clearDisplay();
    u8g2.begin();
    //ESP8266
    Serial.begin(115200);
    WiFi.begin(ssid, pass);
    Serial.println("Connecting...");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("...");
        delay(500);
    }
    Serial.println("Connect successfully!!");
    //BLYNK
    Blynk.begin(auth, ssid, pass);
    //NRF24
    SPI.begin();
    radio.begin();
    radio.setRetries(15, 15);
    radio.setPALevel(RF24_PA_MAX);
    radio.openReadingPipe(1, address);
    radio.startListening();
}

BLYNK_READ(V0)
{
    Blynk.virtualWrite(0, millis() / 1000);
}
BLYNK_READ(V1)
{
    Blynk.virtualWrite(1, millis() / 1000);
}
BLYNK_READ(V2)
{
    Blynk.virtualWrite(2, millis() / 1000);
}

//----------Main-------------//
void loop()
{
    Blynk.run();

    if (radio.available())
    {
        memset(&value, ' ', sizeof(value));
        radio.read(&value, sizeof(value));
        temperature_outdoor = value[0];
        uvIntensity = value[1];
    }
    sensors.requestTemperatures();
    float temperature_indoor = sensors.getTempCByIndex(0);
    //    float temperatureF = sensors.getTempFByIndex(0);

    //Send data to BLYNK:
    sendTempData(temperature_outdoor, temperature_indoor);
    sendUVData(uvIntensity);
    
    //Display virtual LCD - BLYNK:
    if (uvIntensity < 2.9)
    {
        lcd.clear();
        lcd.print(2, 0, "UV - Minimal");
        lcd.print(3, 1, "-> SAFE <-");
    }
    else if (uvIntensity > 2.9 && uvIntensity < 5.9)
    {
        lcd.clear();
        lcd.print(4, 0, "UV - Low");
        lcd.print(2, 1, "-> ITS OK <-");
    }
    else if (uvIntensity > 5.9 && uvIntensity < 7.9)
    {
        lcd.clear();
        lcd.print(1, 0, "UV - Moderate");
        lcd.print(2, 1, "-> UNSAFE <-");
    }
    else if (uvIntensity > 7.9 && uvIntensity < 10.9)
    {
        lcd.clear();
        lcd.print(3, 0, "UV - High");
        lcd.print(0, 1, "-> DANGEROUS! <-");
    }
    else
    {
        lcd.clear();
        lcd.print(1, 0, "UV - Very High");
        lcd.print(1, 1, "S.O.S!! S.O.S!!");
    }

    Serial.print(temperature_outdoor);
    Serial.print("ºC (outdoor) -  ");
    Serial.print(temperature_indoor);
    Serial.println("ºC (indoor) -  ");
    Serial.print("UV Intensity (mW/cm^2): ");
    Serial.print(uvIntensity);
    Serial.println();

    //Display data to OLED:
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis > interval)
    {
        previousMillis = currentMillis;
        u8g2.firstPage();
        do
        {
            switch (draw_state)
            {
            case 0:
                drawUVdata(uvIntensity);
                break;
            case 1:
                drawOutTemperature((int)temperature_outdoor);
                break;
            case 2:
                drawInTemperature((int)temperature_indoor);
                break;
            }
        } while (u8g2.nextPage());
        draw_state++;
        if (draw_state > 2)
        {
            draw_state = 0;
        }
    }
    
    display.clearDisplay();
    delay(1000);
}

//----------Necessary functions-------------//
void sendTempData(float c, float f)
{
    if (isnan(c) || isnan(f))
    {
        Serial.println("Failed to read data from DS18B20 sensor!");
        return;
    }
    else
    {
        Blynk.virtualWrite(V0, c); //V0 for virtual pin - oC
        Blynk.virtualWrite(V1, f); //V1 for virtual pin - oF
    }
}

void sendUVData(float UV)
{
    if (isnan(UV))
    {
        Serial.println("Failed to read data from DS18B20 sensor!");
        return;
    }
    else
    {
        Blynk.virtualWrite(V2, UV);
    }
}

void drawUVdata(float uv)
{
    display.setCursor(20, 0); //oled display
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.println("UV Ray Intensity");

    display.setCursor(20, 18); //oled display
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.println(uv);

    display.setCursor(20, 40); //oled display
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.println("mW/cm^2");
    display.display();
}

void drawInTemperature(int temp_indoor)
{
    inTemp = String(temp_indoor) + char(176) + "C";
    u8g2.setFont(u8g2_font_helvR14_tr);
    u8g2.setCursor(24, 15);
    u8g2.print("INDOOR");
    u8g2.setFont(u8g2_font_fub30_tf);
    u8g2.setCursor(36, 58);
    u8g2.print(inTemp);
    u8g2.drawXBMP(0, 17, Temperature_20Icon_width, Temperature_20Icon_height, Temperature_20Icon_bits);
}

void drawOutTemperature(int temp_outdoor)
{
    outTemp = String(temp_outdoor) + char(176) + "C";
    u8g2.setFont(u8g2_font_helvR14_tr);
    u8g2.setCursor(12, 15);
    u8g2.print("OUTDOOR");
    u8g2.setFont(u8g2_font_fub30_tf);
    u8g2.setCursor(36, 58);
    u8g2.print(outTemp);
    u8g2.drawXBMP(0, 17, Temperature_20Icon_width, Temperature_20Icon_height, Temperature_20Icon_bits);
}
