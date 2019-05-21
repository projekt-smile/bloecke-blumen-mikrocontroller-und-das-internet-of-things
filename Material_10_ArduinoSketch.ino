/* smile Internet-Wetter-Lampe v1.0.3 - letzte Aenderung am 22. Januar 2019 - entwickelt an der Abteilung "Didaktik der Informatik" an der Universitaet in Oldenburg */

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
Adafruit_SSD1306 display(OLED_RESET);

#define LED_DATA_PIN D3                    // an welchem Pin liegt die LED an?

CRGB leds[1];                              // Instanziieren der LED




// ========================  hier eure Werte eintragen  ===================================================================================================================

const String api_key = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";    // dein Open-Weather-Map-API-Schluessel, kostenlos beziehbar ueber https://openweathermap.org/

// ========================================================================================================================================================================



int weatherID = 0;
int weatherID_shortened = weatherID / 100;
String weatherforecast_shortened = " ";
float temperature_Kelvin = 0.0;
float temperature_Celsius = temperature_Kelvin - 273;      // Hinweis: Celsiuswert + 273 = Kelvinwert
unsigned long systemtime = 0;                              // Zeit, die seit dem Start des Mikrocontrollers vergangen ist
unsigned long lastcheck = 0;                               // Zeitpunkt des letzten Checks



void setup() {
  Serial.begin(115200);                               // fuer die Ausgabe des seriellen Monitors

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);        // das Display initialisieren, ...
  display.clearDisplay();                           // ... den Inhalt löschen ...
  display.setTextColor(WHITE);                      // ... und "Verbindungsversuch" anzeigen
  display.setTextSize(1);
  display.startscrollleft(0x00, 0x0F);
  display.setCursor(0, 9);
  display.println("Verbinde dich mit");
  display.setCursor(0, 17);
  display.println("deineSmarteLampe und");
  display.setCursor(0, 25);
  display.println("\224ffne 192.168.4.1");
  display.display();                                // ... und die Änderungen anzeigen

  FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(leds, 1); // LED instanziieren
  FastLED.show();

  leds[0] = CRGB::Red;                              // LED zu Beginn rot setzen, um sie zu testen
  FastLED.setBrightness(255);
  FastLED.show();
  wifiManager.autoConnect("deineSmarteLampe");

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
  display.setCursor(32, 9);                 // bei TextSize(1) funktioniert setCursor(32, 9) ganz gut
  display.println(weatherforecast_shortened);

  Serial.println(weatherforecast_shortened);
  Serial.println(weatherforecast_shortened.length());

  // untere Zeile
  if (weatherforecast_shortened.length() != 0) {                          // nur, wenn weatherforecast_shortened nicht leer ist (dann naemlich keine Server-Antwort)
    int digitsTemperature = String(round(temperature_Celsius)).length();  // wie lang (wie viele Ziffern) ist die Anzeige der Temperatur?
    display.setCursor(77 - 12 * digitsTemperature, 18);                   // bei textsize(2) ist eine Ziffer 12 Pixel breit; rechtsbuendig anzeigen, deswegen wird die x-Koord. des Cursors abhaengig davon gesetzt
    display.setTextSize(2);
    display.println(String(round(temperature_Celsius)));

    // Grad Celsius: C
    display.setCursor(86, 18);
    display.setTextSize(2);
    display.println("C");

    // Grad Celsius: Kreis
    display.drawCircle(80, 20, 2, WHITE);
  } else {
    display.stopscroll();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(32, 9);
    display.println("keine");
    display.setCursor(32, 17);
    display.println("Server-");
    display.setCursor(32, 25);
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
  weatherforecast_shortened = "";
  temperature_Celsius = temperature_Kelvin - 273;                                  // Hinweis: Celsiuswert + 273 = Kelvinwert
  weatherID_shortened = weatherID / 100;
  switch (weatherID_shortened) {                                                   // Werte hierfuer entstammen aus https://openweathermap.org/weather-conditions
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

