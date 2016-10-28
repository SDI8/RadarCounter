#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "passwords.h"

#define FREQ_PIN 5
#define LED_PIN 2

boolean state = false;
volatile uint32_t cycle_start_time;
volatile uint32_t cycle_low_time;
uint32_t cycle_high_time;
uint32_t cycle_time;

volatile float hilo_ratio_err;
float avg_hilo_ratio_err;
volatile float freq;
float avg_freq;

ESP8266WebServer server(80);

void freqCounterRisingISR( void ) {
  cycle_low_time = abs( micros() - cycle_start_time );
}

void freqCounterFallingISR( void ) {
  cycle_high_time = abs(micros() - cycle_start_time) - cycle_low_time;
  cycle_start_time = micros();

  cycle_time = cycle_low_time + cycle_high_time;

  if (cycle_time > 0) {
    hilo_ratio_err = abs( cycle_low_time - cycle_high_time ) / (cycle_time / 100);
    freq = 1000000 / cycle_time;
  }
        
}

void freqCounterISR( void ) {
  if (digitalRead(FREQ_PIN)) {
    freqCounterRisingISR();
    digitalWrite(LED_PIN, HIGH);
  } else {
    freqCounterFallingISR();
    digitalWrite(LED_PIN, LOW);
  }
}

void handleSimpleRequest( void ) {

  String message = String( avg_hilo_ratio_err );
  message += "\t";
  message += String( avg_freq );
  server.send(200, "text/plain", message);

}

float toVoltage(int x) {
  return 29.8328+(-0.0762457+0.0000557642*x)*x;
}

void handleState() {
  int raw = analogRead(A0);
  String message = String(raw);
  message += "\t";
  message += String(toVoltage(raw),4);
  message += "V";
  server.send(200, "text/plain", message);
}

void setup() {
  // put your setup code here, to run once:
  pinMode( 16, LOW );
  pinMode( FREQ_PIN, INPUT);
  pinMode( LED_PIN, OUTPUT );
  attachInterrupt(digitalPinToInterrupt(FREQ_PIN), freqCounterISR, CHANGE);
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  
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

  if (MDNS.begin("radarsensor")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", handleSimpleRequest);
  server.on("/state", handleState);
  server.onNotFound([](){server.send(404,"text/plain","404");});
  server.begin();

}

void loop() {
  // put your main code here, to run repeatedly:

  noInterrupts();
  if (freq < 1000) {
    avg_freq = avg_freq * .9 + freq * .1;  
  }
  if (hilo_ratio_err <= 100) {
    avg_hilo_ratio_err = avg_hilo_ratio_err * .9 + hilo_ratio_err * .1;
  }
  interrupts();
/*
  Serial.print( avg_hilo_ratio_err );
  Serial.print("\t");
  Serial.println( avg_freq );
*/
  delay(20);
  server.handleClient();
}


