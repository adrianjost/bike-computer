/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Wir erzeugen einen Web-Server als Access Point, der den Inhalt einer Micro SD Card
  zuzm Download bereitstellt.
  Für die SD-Card werden die SPI -Pins benutzt
  D5 = GPIO14 als Clock
  D6 = GPIO12 als MISO
  D7 = GPIO13 als MOSI
  D8 = GPIO15 als Chip Select
  
  die verwendete SSID = SD_Server
  das Passwort = Micro
   
*/

//Wifi Bibliothek + Client + Web-Server
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//Bibliothek für SPI Bus
#include <SPI.h>
//Bibliothek für SD-Card Filesystem
#include <SdFat.h>

#include "sdios.h"

using namespace sdfat;

const uint8_t chipSelect = 15;

//Template für die HTML-Seite
const char HTML_HEADER[] =
    "<!DOCTYPE HTML>"
    "<html>"
    "<head>"
    "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
    "<title>SD-Card reader</title>"
    "<style>"
    "body { background-color: #d2f3eb; font-family: Arial, Helvetica, Sans-Serif; Color: #000000;font-size:12pt; }"
    "</style>"
    "</head>"
    "<body><div style='margin-left:30px;'>";
const char HTML_END[] =
    "</div></body>"
    "</html>";

//Globale Variablen
//Instanz der SdFat Bibliothek
SdFat sd;
//Globale Objekt Variablen zum Speichern von Files und Directories
SdFile file;
SdFile dirFile;
//Access Point
const char *ssid = "SD_Server";  //Name des WLAN
const char *password = "Micro";  //Passwort für das WLAN
//Flag für die SD-Card Initialisierung
bool sdinit = false;

ESP8266WebServer server(80);  //Web-Server starten auf Port 80

//Funktion zum Ermitteln des ContentTypes je nach Datei-Endung
String getContentType(String filename) {
  if (server.hasArg("download"))
    return "application/octet-stream";
  else if (filename.endsWith(".htm"))
    return "text/html";
  else if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".xml"))
    return "text/xml";
  else if (filename.endsWith(".pdf"))
    return "application/x-pdf";
  else if (filename.endsWith(".zip"))
    return "application/x-zip";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  return "text/plain";
}

//Funktion zum Senden einer Datei
//Parameter Pfad und Dateiname
bool sendFile(String path, String fn) {
  char cpath[512];
  uint32_t filesize;
  String contentType;
  char cname[256];
  sdfat::File myFile;

  //Erstellen des vollständigen Dateinamens
  //und umspeichern in ein Character-Array
  path = path + "/" + fn;
  path.toCharArray(cpath, 512);
  //Filenamen in Kleinbuchstaben umwandeln
  //zur Bestimmung des Dateityps
  fn.toLowerCase();
  contentType = getContentType(fn);
  //File auf der SD card öffnen
  myFile = sd.open(path);
  //und in den Web-Server streamen
  server.streamFile(myFile, contentType);
  //File schließen
  file.close();
  return true;
}

//Ein Dateiverzeichnis senden
bool sendDirectory(String path) {
  char cpath[512];
  uint16_t n = 0;
  char cname[256];
  String subdir;
  String parent;
  String name;

  path.toCharArray(cpath, 512);
  //Wir versuchen den Pfad zu öffnen
  if (dirFile.open(cpath, O_READ)) {
    //ist die Aktion erfolgreich setzen wir die Contentlänge auf unbekannt
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    //und senden den Header
    server.send(200, "text/html", HTML_HEADER);
    WiFiClient client = server.client();
    if (path != "/") {
      //wenn der Pfad nicht auf das Rootverzeichnis zeigt fügen wir eine
      //Zeile mit ".." ein um eine Ebene zurück gehen zu können
      parent = path;
      //wir benötigen das übergeordnete Verzeichnis
      parent.remove(parent.lastIndexOf("/"));
      //und bilde daraus einen Link, den wir an den Client senden
      server.sendContent("<a href='http://192.168.4.1/?DIR=");
      server.sendContent(parent);
      server.sendContent("'>..</a><br>");
    }
    //nun folgen die Zeilen für die Verzeichniseinträge
    while (file.openNext(&dirFile, O_RDONLY)) {
      //am Anfang des Links setzen wir den Pfad
      server.sendContent("<a href='http://192.168.4.1/?DIR=");
      server.sendContent(path);
      //Filename lesen
      file.getName(cname, 255);
      name = String(cname);
      if (file.isDir()) {
        //wenn der Eintrag ein Unterverzeichnis ist
        //hängen wir dieses an den Pfad an und schließen den Link
        subdir = "/" + name;
        server.sendContent(subdir);
        server.sendContent("'>");
      } else {
        //ist es ein einfaches File fügen wir den Filenamen an
        server.sendContent("&FN=");
        server.sendContent(name);
        //zum Link addieren wir noch "target=''" damit das File
        //in einem eigenen Fenster geöffnet wird.
        //Dann schließen wir den Link
        server.sendContent("' target=''>");
      }
      //zum Schluss folgt der Name als Label für den Link
      server.sendContent(name);
      //und das Ende vom Link sowie ein Zeilenvorschub
      server.sendContent("</a><br>");
      //File schließen
      file.close();
    }
    //Nachdem alle Einträge gesendet wurden, wird das Direktory geschlossen
    dirFile.close();
    //Das Contentende dem Client mitteilen und die Verbindung beenden
    server.sendContent(HTML_END);
    client.stop();
    return true;
  } else {
    return false;  //Fehler der Pfad konnte nicht geöffnet werden
  }
}
//Diese Funktion wird aufgerufen wenn der Request an den Web-Server = "/" ist
void handleRoot() {
  //Filename und Pfad mit Defaultwerten füllen
  String path = "/";
  String fn = "";
  //Falls der Request entsprechende Argumente enthält
  //Pfad und Filename mit den Daten des Requests füllen
  if (server.hasArg("DIR")) path = server.arg("DIR");
  if (server.hasArg("FN")) fn = server.arg("FN");
  Serial.print("Path ");
  Serial.print(path);
  Serial.print(" File: ");
  Serial.println(fn);
  String name;
  //Wenn die SD Card noch nicht initialisiert wurde diese initialisieren
  if (!sdinit) sdinit = sd.begin(chipSelect, SD_SCK_MHZ(50));
  if (sdinit) {
    //War die Initialisierung erfolgreich, lesen wir Daten von der Karte
    if (fn == "") {
      //Wenn kein Filename angegeben wurde versuchen wir ein Verzeichnis zu senden
      if (!sendDirectory(path)) {
        //Konnte das Verzeichnis nicht gesendet werden setzen wir sdinit auf false
        //Da offenbar keine Karte im Reader ist. Wir senden an den Client eine entsprechende Warnung
        sdinit = false;
        server.send(200, "text/html", "Keine SD Karte");
      }
    } else {
      //Ansonsten versuchen wir das angegebene File zu senden
      if (!sendFile(path, fn)) {
        //Konnte das Verzeichnis nicht gesendet werden setzen wir sdinit auf false
        //Da offenbar keine Karte im Reader ist. Wir senden an den Client eine entsprechende Warnung
        sdinit = false;
        server.send(200, "text/html", "Keine SD Karte");
      }
    }
  } else {
    //Initialisierung nicht erfolgreich
    server.send(200, "text/html", "Keine SD Karte");
  }
}

//Prozessor vorbereiten
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");

  //Access Point einrichten
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  //ip ist immer 192.168.4.1
  //Definition der Antwortfunktionen
  server.on("/", handleRoot);
  //Web Server starten
  server.begin();
  Serial.println("HTTP server started");
}

//Hauptschleife
void loop() {
  //Auf Request prüfen
  server.handleClient();
}
