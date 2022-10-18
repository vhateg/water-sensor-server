#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

volatile int pulses = 0; //variable that holds the number of pulses from flow sensor

const int flow_pin = 2; //pin used by flow sensor

const char *SSID = "";
const char *PWD = "";

const float a = 1; //multiply raw analog value by this to obtain the temperature
const float pulse_ratio = 550;

int flowARR[84];
unsigned long lastTime = 0;

WebServer server(80); //start server on port 80

Adafruit_PCD8544 display = Adafruit_PCD8544(18, 23, 19, 5, 14);

void displayInit() { //initialize the display
  display.begin();
  display.setContrast(50);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(BLACK);
  
}

void refreshScreen(float flow, float temperature) { //refresh the screen
  if (flow>0) digitalWrite(27, HIGH); //if flow exists, turn on the screen
    else digitalWrite(27, LOW);
  int val = 5;
  display.clearDisplay();
  //show flow
  display.setTextSize(0);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.write("Flow: ");
  String s = String(flow, 3);
  char c[10];
  s.toCharArray(c, 16);
  display.write(c);

  //show temp
  display.setCursor(0,10);
  display.write("Temp: ");
  String s2 = String(temperature);
  char c2[10];
  s2.toCharArray(c2, 16);
  display.write(c2); 
  for(int i = 1; i<84;++i) {
    flowARR[i -1] = flowARR[i];
  }
  flowARR[83] = flow;
  float max = 0;
  for (int i = 0; i<84; ++i) {
    if (flowARR[i] > max) max = flowARR[i];
  }
  Serial.println(max);
  for (int i = 0; i<84; ++i) {
    val = floor(flowARR[i]/max*22);
    for (int j = 0; j<23; ++j) {
      if (val>j) {
        display.drawPixel(i, 47-j, BLACK);
      } else {
        display.drawPixel(i, 47-j, WHITE);
      }
  }
  }
  display.display();
}

void connectToWiFi() { 
  Serial.print("Connecting to ");
  Serial.println(SSID);
  
  WiFi.begin(SSID, PWD);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
 
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}

StaticJsonDocument<250> jsonDocument;
char buffer[250];
void create_json(String tag, float value) {  
  jsonDocument.clear();
  char* tagChar;
  tag.toCharArray(tagChar, 10);
  jsonDocument["type"] = tagChar;
  jsonDocument["value"] = value;
  serializeJson(jsonDocument, buffer);  
}
 
void add_json_object(String tag, float value) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["type"] = tag;
  obj["value"] = value;
}

void getData() {
  Serial.println("Sending data");
  jsonDocument.clear(); // Clear json buffer
  float amount_difference = pulses /550;
  //amount_difference = random(100);
  pulses = 0;
  float temperature = analogRead(32)*a;
  //temperature = 44.356;
  add_json_object("amount_difference", amount_difference);
  add_json_object("temperature", temperature);
  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
  refreshScreen(amount_difference/((millis()-lastTime)/1000), temperature);
  lastTime = millis();
}


void setupRouting() {
  server.on("/data", getData);
  server.begin();
}

void pulseCounter() {
  pulses++;
}

void setup() {
  // put your setup code here, to run once:
  
  Serial.begin (9600);
  pinMode(27, OUTPUT);
  attachInterrupt(flow_pin, pulseCounter, CHANGE);
  displayInit();
  connectToWiFi();
  randomSeed(212);
  setupRouting();
}

void loop() {
  server.handleClient();
}