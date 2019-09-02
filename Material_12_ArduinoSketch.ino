/* 
               _ _      
 ___ _ __ ___ (_) | ___ 
/ __| '_ ` _ \| | |/ _ \
\__ \ | | | | | | |  __/
|___/_| |_| |_|_|_|\___|

Internet-Wetter-Lampe v1.0.6 - letzte Aenderung am 30. August 2019 - entwickelt an der Abteilung "Didaktik der Informatik" an der Universitaet in Oldenburg

*/


#include "FastLED.h"                      // Bibliothek einbinden, um LED ansteuern zu koennen

#include <SPI.h>                          // Bibliotheken einbinden, um das OLED Display ansteuern zu koennen
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "RestClient.h"                   // Bibliothek einbinden, um Get-Requests senden zu koennen
#include <ArduinoJson.h>                  // Bibliothek einbinden, um JSONs parsen zu koennen

#include <math.h>                         // Bibliothek einbinden, um Temperaturen runden zu koennen

#include <WiFiManager.h>                  // Bibliothek einbinden, um Uebergabe der WiFi Credentials ueber einen AP zu ermoeglichen
WiFiManager wifiManager;

RestClient client = RestClient("api.openweathermap.org");                         // RestClient der Openweathermap-API (Hinweis: Port mit Komma uebergeben)

#define OLED_RESET 0                       // "0" fuer ESP8266
#define SCREEN_WIDTH 128                   // OLED display width, in pixels
#define SCREEN_HEIGHT 64                   // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED_DATA_PIN D4                    // an welchem Pin liegt die LED an?

CRGB leds[1];                              // Instanziieren der LED




// ========================  hier deinen API-Key eintragen!!!  ============================================================================================================

const String api_key = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";    // dein Open-Weather-Map-API-Schluessel, kostenlos beziehbar ueber https://openweathermap.org/

// ========================================================================================================================================================================



int weatherID = 0;
int weatherID_shortened = weatherID / 100;
String weatherforecast_shortened = " ";
float temperature_Kelvin = 0.0;
float temperature_Celsius = temperature_Kelvin - 273;      // Hinweis: Celsiuswert + 273 = Kelvinwert
int temperature_Celsius_Int;
unsigned long systemtime = 0;                              // Zeit, die seit dem Start des Mikrocontrollers vergangen ist
unsigned long lastcheck = 0;                               // Zeitpunkt des letzten Checks



void setup() {
  Serial.begin(115200);                               // fuer die Ausgabe des seriellen Monitors

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);        // das Display initialisieren, ...
  display.clearDisplay();                           // ... den Inhalt löschen ...
  display.setTextColor(WHITE);                      // ... und "Verbindungsversuch" anzeigen
  display.setTextSize(1);
  display.startscrollleft(0x00, 0x0F);
  display.setCursor(0, 16);                         // Aufgrund der kleineren Displaygröße hier ein Offset von 16Pixeln        
  display.println("Verbinde dich mit");
  display.setCursor(0, 28);                        // 7 Pixel Buchstabenhöhe + 5 Pixel Abstand
  display.println("deineSmarteLampe");
  display.setCursor(0, 40);
  display.println("und \224ffne");
  display.setCursor(0, 52);
  display.println("192.168.4.1");
  display.display();                                // ... und die Änderungen anzeigen

  FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(leds, 1); // LED instanziieren
  FastLED.show();

  leds[0] = CRGB::Red;                              // LED zu Beginn rot setzen, um sie zu testen
  FastLED.setBrightness(255);
  FastLED.show();
  wifiManager.autoConnect("deineSmarteLampe");      // hier kann der Name des Hotspots deiner Lampe angepasst werden

  getCurrentWeatherConditions();                    // nach dem (Neu-)Start erstmalig das aktuelle Wetter laden
  updateDisplay();
}






void loop() {
  // folgende if-Bedingung sorgt dafuer, dass nur alle 30 Minuten (also 1.800.000 ms) das aktuelle Wetter abgefragt wird; dies spart Strom und Web-Traffic
  if (millis() - lastcheck >= 1800000) {            // millis() gibt die Zeit aus, die seit dem Start des Mikrocontrollers verstrichen ist
    getCurrentWeatherConditions();
    lastcheck = millis();                           // lastcheck auf aktuelle Systemzeit setzen
    updateDisplay();
  }

  // unten werden nun je nach Wetterbedingung (die in der Variablen "weatherID_shortened" steckt) eine Funktion aufgerufen, die die Lampe unterschiedlich leuchten laesst
  if (weatherID == 800) LED_effect_clearSky();                  // nur wenn die "weatherID" 800 ist, ist es klar/heiter/sonnig...
  else {
    switch (weatherID_shortened) {                              // sonst ist es je nach Hunderterbereich unterschiedlich: Werte hierfuer entstammen aus https://openweathermap.org/weather-conditions
      case 2: LED_effect_thunder(); break;    // "Gewitter"
      case 3: LED_effect_drizzle(); break;    // "Nieselregen"
      case 5: LED_effect_rain(); break;       // "Regen"
      case 6: LED_effect_snow(); break;       // "Schnee"
      case 7: LED_effect_fog(); break;        // "Nebel"
      case 8: LED_effect_cloudy(); break;     // "Wolken"
    }
  }

  // und die loop-Schleife beginnt nun wieder von vorne ... :)

}



// ========================================================================================================================================================================
/* es folgen Funktionen, die von der Lampe benoetigt werden*/


void updateDisplay() {                                                                    // Funktion zum Aktualisieren des Inhalts auf dem Display
  display.clearDisplay();
  display.stopscroll();

  // obere Zeile
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(32, 16);                 // bei TextSize(1) funktioniert setCursor(32, 9) ganz gut
  display.println(weatherforecast_shortened);

  Serial.println(weatherforecast_shortened);
  Serial.println(weatherforecast_shortened.length());

  // untere Zeile
  if (weatherforecast_shortened.length() != 0) {                          // nur, wenn weatherforecast_shortened nicht leer ist (dann naemlich keine Server-Antwort)

    Serial.println(temperature_Celsius_Int);
    Serial.println(String(temperature_Celsius_Int,DEC));
    int digitsTemperature = String(temperature_Celsius_Int,DEC).length();  // wie lang (wie viele Ziffern) ist die Anzeige der Temperatur?
    display.setCursor(77 - 12 * digitsTemperature, 50);                   // bei textsize(2) ist eine Ziffer 12 Pixel breit; rechtsbuendig anzeigen, deswegen wird die x-Koord. des Cursors abhaengig davon gesetzt
    display.setTextSize(2);
    display.println(temperature_Celsius_Int);
    

    // Grad Celsius: C
    display.setCursor(86, 50);
    display.setTextSize(2);
    display.println("C");

    // Grad Celsius: Kreis
    display.drawCircle(80, 52, 2, WHITE);
  } else {
    display.stopscroll();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(32, 16);
    display.println("keine");
    display.setCursor(32, 28);
    display.println("Server-");
    display.setCursor(32, 40);
    display.println("antwort...");
  }
  display.display();
}


void getCurrentWeatherConditions() {                                                      // Funktion zum Abrufen der Wetterdaten von der Openweathermap-API
  
  String address = "/data/2.5/weather?q=Oldenburg,DE&APPID=" + api_key;
  char address2[100];
  address.toCharArray(address2, 100);
  Serial.println(address2);
  String response = "";
  int statusCode = client.get(address2, &response);                                       // Wetter von heute über die openweathermap-API
  // int statusCode = client.get("/weather?ort=Oldenburg&wann=heute", &response);         // Wetter von heute über den Server der Uni Oldenburg
  Serial.print("Status code from server: "); Serial.println(statusCode);
  Serial.print("Response body from server: "); Serial.println(response);

  //an dieser Stelle wird die Antwort vom Server zurechtgeschnitten (geparsed); weitere Hinweise hierzu unter arduinojson.org/assistant
  StaticJsonDocument<1000> doc;
  char json[1000]; // Antwort in char umwandeln; Groesse ueber den arduinojson.org/assistant berechnen
  response.toCharArray(json, 1000);
  DeserializationError error = deserializeJson(doc, json);
  JsonObject root = doc.as<JsonObject>();
  JsonObject weather = root["weather"][0];
  JsonObject weatherdaten = root["main"];

  weatherID = weather["id"];
  temperature_Kelvin = weatherdaten["temp"];
  Serial.println(temperature_Kelvin);
  weatherforecast_shortened = "";
  temperature_Celsius = temperature_Kelvin - 273;                                // Hinweis: Celsiuswert + 273 = Kelvinwert

  temperature_Celsius_Int = (int)temperature_Celsius;
  
  weatherID_shortened = weatherID / 100;
  switch (weatherID_shortened) {                                                 // Werte hierfuer stammen aus https://openweathermap.org/weather-conditions
    case 2: weatherforecast_shortened = "Gewitter"; break;
    case 3: weatherforecast_shortened = "Nieselreg."; break;
    case 5: weatherforecast_shortened = "Regen"; break;
    case 6: weatherforecast_shortened = "Schnee"; break;
    case 7: weatherforecast_shortened = "Nebel"; break;
    case 8: weatherforecast_shortened = "Wolken"; break;
    default: weatherforecast_shortened = ""; break;                              // wenn kein anderer Wert passt (z.B. weil Server nicht antwortet), ist die weatherlage ungewiss
  } if (weatherID == 800) weatherforecast_shortened = "klar";                    // nur fuer den Fall, dass die weather-ID genau 800 ist, ist es "klar"
}


// ========================================================================================================================================================================
// folgende Methode erleichtert das Modellieren von Farbverlaeufen von einem RGB-Wert

void fade(int led_position, uint16_t duration, uint16_t delay_val, uint16_t startR, uint16_t startG, uint16_t startB, uint16_t endR, uint16_t endG, uint16_t endB) {
    int16_t redDiff = endR - startR;
    int16_t greenDiff = endG - startG;
    int16_t blueDiff = endB - startB;
    int16_t steps = duration*1000 / delay_val;
    int16_t redValue, greenValue, blueValue;
    for (int16_t i = 0 ; i < steps - 1 ; ++i) {
        redValue = (int16_t)startR + (redDiff * i / steps);
        greenValue = (int16_t)startG + (greenDiff * i / steps);
        blueValue = (int16_t)startB + (blueDiff * i / steps);
        leds[0]=CRGB(redValue, greenValue, blueValue);
        FastLED.show();
        delay(delay_val);
    }
    leds[led_position]=CRGB(endR, endG, endB);
 }


/*
                            
###### #    # #####  ###### 
#      #    # #    # #      
#####  #    # #    # #####  
#      #    # #####  #      
#      #    # #   #  #      
######  ####  #    # ###### 
                            
#######                                          
#       ###### ###### ###### #    # ##### ###### 
#       #      #      #      #   #    #   #      
#####   #####  #####  #####  ####     #   #####  
#       #      #      #      #  #     #   #      
#       #      #      #      #   #    #   #      
####### #      #      ###### #    #   #   ###### 
                                                 
                                                          
###### # #    # ###### #    # ######  ####  ###### #    # 
#      # ##   # #      #    # #      #    # #      ##   # 
#####  # # #  # #####  #    # #####  #      #####  # #  # 
#      # #  # # #      #    # #      #  ### #      #  # # 
#      # #   ## #      #    # #      #    # #      #   ## 
###### # #    # #       ####  ######  ####  ###### #    # 



          ########
          ########
          ########
          ########
          ########
          ########
          ########
          ########
          ########
          ########
     ##################
       ##############
         ##########
           ######
             ## 
*/

void LED_effect_clearSky() { // Effekt, der angezeigt wird, wenn der Himmel klar ist
  FastLED.setBrightness(255);
  leds[0] = CRGB::Yellow;
  FastLED.show();
  delay(500);
  leds[0] = CRGB::Black;
  FastLED.show();
  delay(500);
}


void LED_effect_thunder() {

}

void LED_effect_drizzle() {

}

void LED_effect_rain() {

}

void LED_effect_snow() {

}

void LED_effect_fog() {

}

void LED_effect_cloudy() {

}
