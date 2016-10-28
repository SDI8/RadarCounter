#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "FS.h"
#include <ArduinoJson.h>

#include "passwords.h"

uint32_t lastcheck = 0;

ESP8266WebServer server(80);

const int led = 13;

float toVoltage(int x) {
  return 29.8328+(-0.0762457+0.0000557642*x)*x;
}

void handleRoot() {
  digitalWrite(led, 1);
  File logfile = SPIFFS.open("/log.json", "r");
  if (logfile) {
    server.streamFile(logfile, "application/json");
  }
  logfile.close();
  digitalWrite(led, 0);
}

void handleDirect() {
  digitalWrite(led, 1);
  int raw = analogRead(A0);
  String message = String(raw);
  message += "\t";
  message += String(toVoltage(raw),4);
  message += "V";
  server.send(200, "text/plain", message);
  digitalWrite(led, 0);
}

bool saveState() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["time"] = String(millis() / 1000);
  json["value"] = (analogRead(A0) / 199.0f) *(-1) + 5.14;

  File logfile = SPIFFS.open("/log.json", "a");
  if (!logfile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(logfile);
  logfile.print(",\n");
  logfile.close();
  return true;
}

void handleNotFound(){
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void setup(void){
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");
  SPIFFS.begin();
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/direct", handleDirect);

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();

  if ((lastcheck + 60) < millis()/1000) {
    lastcheck = millis()/1000;
    saveState();
  }
}
