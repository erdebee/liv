
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>


#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK  "your-password"
#endif

const char* host = "esp8266-webupdate";
const char* ssid = STASSID;
const char* password = STAPSK;

const String playlistPath = "";

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;



void handleRoot() {
  char temp[2000];
  
  if (server.hasArg("mode")) {
    handleSubmit();
  }

  server.send ( 200, "text/html", temp );
}

void handleSubmit() {
  char mode = server.arg("mode")[0];
  char effect = server.arg("effect")[0];

  EEPROM.write(0, mode);
  EEPROM.write(1, effect);
  EEPROM.commit();
}

String buildPlaylist() {
  Dir dir = SPIFFS.openDir(playlistPath);
  String out = "[";
  int i = 0;
  while (dir.next()) {
    char temp[100];
    File entry = dir.openFile("r");
    out = out + ",\"" + entry.name()+"\"";
  }
   out = out.substring(1) + "]";
  return out + ;
}

void handleNotFound() {
  digitalWrite ( led, 1 );
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
  digitalWrite ( led, 0 );
}

void setupWifi() {
  Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 10; i < 42; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 42; i < 106; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  if ( esid.length() > 1 ) {
    WiFi.begin(esid.c_str(), epass.c_str());
    if (testWifi()) {
      syncTime();
      launchWeb(0);
      return;
    }
  }
  setupAP();
}

bool testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    delay(500);
    Serial.print(WiFi.status());
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void launchWeb(int webtype) {
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer(webtype);
  // Start the server
  server.begin();
  Serial.println("Server started");
}

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<option value=''>-- Choose network</option>";
  char temp[200];
  for (int i = 0; i < n; ++i)
  {
    String ssid = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);
    String enc = (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "  " : " *";

    int len = ssid.length() * 2 + 36;

    snprintf ( temp, len, "<option value=\"%s\">%s (%i) %s</option>", ssid.c_str(), ssid.c_str(), rssi, enc.c_str());
    st += temp;
  }

  delay(100);
  WiFi.softAP(ssid, passphrase, 6);
  Serial.println("softap");
  launchWeb(1);
  Serial.println("over");
}

void createWebServer(int webtype)
{
  if ( webtype == 1 ) {
    server.on("/", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>There is no Wifi connection. <br/>Configure wifi below.<br/>The current IP: ";
      content += ipStr;
      content += "<p>";
      content += "</p><form method='post' action='setting'><label>SSID: </label><select name='ssid'>";
      content += st;
      content += "</select><input name='pass' placeholder='password' length=64><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing wifi eeprom");
        for (int i = 10; i < 106; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(10 + i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(42 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();
        content = "Settings saved. System will reboot and try to connect...";
        statusCode = 200;
        server.send(statusCode, "application/json", content);
        resetFunc();
      } else {
        content = "404, page not found";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.send(statusCode, "application/json", content);
    });
  } else if (webtype == 0) {
    server.on ( "/", handleRoot );
    server.on("/reset", []() {
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Reset done. System is rebooting.</p></html>";
      server.send(200, "text/html", content);
      Serial.println("clearing eeprom");
      for (int i = 0; i < 106; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      resetFunc();
    });
  }
  server.onNotFound ( handleNotFound );
}
