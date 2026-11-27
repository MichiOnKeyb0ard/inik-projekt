/*
  IniK-Box - Messwerte auf dem Display und auf einer eigenen Webseite
  Informatik im Kontext, WiSe 26/27

  Hardware: Carenuity C3 Mini auf der Platine IniK-Box v1.3
  In der Arduino IDE als Board "LOLIN C3 MINI" auswaehlen.

  Pin         Funktion
  ----------  ------------------------------------------
  GPIO 8      SDA (I2C) - Sensor und Display
  GPIO 10     SCL (I2C) - Sensor und Display

  Bauteile am I2C-Bus und ihre Adressen
  BMP280   0x77   Luftdruck und Temperatur
  AHT20    0x38   Luftfeuchte und Temperatur
  Display  0x3C   OLED, 64 x 48 Pixel

  Benoetigte Bibliotheken (Bibliotheksverwalter der Arduino IDE):
  U8g2, Adafruit AHTX0, Adafruit BMP280 Library
  WiFi und WebServer gehoeren zum ESP32-Boardpaket und muessen nicht
  installiert werden.

  WLAN-Name und Passwort stehen in der Datei zugangsdaten.h, nicht hier.
  Trag dort dein WLAN ein. Damit die Datei nicht in dein Repository kommt,
  traegst du zugangsdaten.h in die Datei .gitignore deines Repositorys ein.

  Aufbau nach dem EVA-Prinzip:
  messen()          Eingabe      - Werte vom Sensor holen
  zeigeAufDisplay() Ausgabe 1    - Werte auf das Display
  sendeSeite()      Ausgabe 2    - Werte als Webseite an den Browser

  Vorgegeben ist nur die Temperatur. Luftfeuchte und Luftdruck ergaenzt
  du selbst - an den drei Stellen, die mit AUFGABE markiert sind.

  Fuer die Zusatzaufgabe (Warnung per Blinken): Die RGB-LED sitzt an
  GPIO 7 und wird mit der Bibliothek Adafruit NeoPixel angesteuert -
  wie im Sketch inikBoxBlinkTaster aus Uebungszettel 05.
*/

#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
#include <WebServer.h>
#include "zugangsdaten.h"

U8G2_SSD1306_64X48_ER_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
Adafruit_AHTX0  aht;
Adafruit_BMP280 bmp;
WebServer server(80);        // 80 ist der uebliche Port fuer Webseiten

float temperatur = 0;        // Grad Celsius
float feuchte    = 0;        // Prozent relative Luftfeuchte - noch nicht gemessen
float druck      = 0;        // Hektopascal                  - noch nicht gemessen

unsigned long letzteMessung = 0;
const unsigned long MESS_ABSTAND = 2000;   // alle 2 Sekunden messen

// ---------- Eingabe ----------
void messen() {
  temperatur = bmp.readTemperature();

  // AUFGABE (Stelle 1 von 3): Luftfeuchte und Luftdruck messen
  // Die Luftfeuchte kommt vom AHT20 und wird anders abgeholt als die
  // Temperatur vom BMP280:
  //   sensors_event_t f, t;
  //   aht.getEvent(&f, &t);
  //   danach steht der Wert in f.relative_humidity
  // Den Luftdruck liefert bmp.readPressure() in Pascal.
  // Rechne ihn in Hektopascal um.
}

// ---------- Ausgabe 1: Display ----------
void zeigeAufDisplay() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.setCursor(0, 10);
  display.print(temperatur, 1);
  display.print(" C");

  // AUFGABE (Stelle 2 von 3): Luftfeuchte in Zeile 2 (y = 21) und Luftdruck in Zeile 3
  // (y = 32) anzeigen - nach demselben Muster wie die Temperatur.

  display.setFont(u8g2_font_4x6_tf);   // kleine Schrift, damit die Adresse passt
  display.setCursor(0, 46);
  if (WiFi.status() == WL_CONNECTED) {
    display.print(WiFi.localIP());
  } else {
    display.print("kein WLAN");
  }
  display.sendBuffer();
}

// ---------- Ausgabe 2: Webseite ----------
// Wird jedes Mal aufgerufen, wenn ein Browser die Adresse der Box oeffnet.
void sendeSeite() {
  String seite = "<!DOCTYPE html><html><head>";
  seite += "<meta charset='utf-8'>";
  seite += "<meta name='viewport' content='width=device-width'>";
  seite += "<meta http-equiv='refresh' content='10'>";   // alle 10 s neu laden
  seite += "<title>IniK-Box</title></head><body>";
  seite += "<h1>Meine IniK-Box</h1>";
  seite += "<p>Temperatur: " + String(temperatur, 1) + " &deg;C</p>";
  // AUFGABE (Stelle 3 von 3): je eine Zeile fuer Luftfeuchte und Luftdruck ergaenzen
  seite += "</body></html>";
  server.send(200, "text/html", seite);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 10);

  display.begin();
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.setCursor(0, 20);
  display.print("Verbinde");
  display.sendBuffer();

  if (!aht.begin())     Serial.println("AHT20 meldet sich nicht (0x38)");
  if (!bmp.begin(0x77)) Serial.println("BMP280 meldet sich nicht (0x77)");

  // ---------- ins WLAN ----------
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_NAME, WLAN_PASSWORT);
  // Verbindet sich deine Box nicht, entferne die beiden // vor der
  // naechsten Zeile. Sie senkt die Sendeleistung; bei manchen C3-Mini-
  // Platinen klappt die Verbindung erst dann.
  // WiFi.setTxPower(WIFI_POWER_8_5dBm);

  Serial.print("Verbinde mit ");
  Serial.println(WLAN_NAME);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Adresse: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Keine Verbindung - WLAN-Name und Passwort in zugangsdaten.h pruefen");
  }

  // ---------- Webserver starten ----------
  server.on("/", sendeSeite);   // Adresse "/" -> Funktion sendeSeite
  server.begin();

  messen();
  zeigeAufDisplay();
}

void loop() {
  server.handleClient();        // Hat ein Browser angefragt? Dann antworten.

  if (millis() - letzteMessung >= MESS_ABSTAND) {
    letzteMessung = millis();
    messen();
    zeigeAufDisplay();
  }
}
