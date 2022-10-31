#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

volatile int pulses = 0; //variable that holds the number of pulses from flow sensor

const int flow_pin = 21; //pin used by flow sensor
const int temp_pin = 32; //pin used by flow sensor

const char *SSID = "";
const char *PWD = "";

IPAddress local_IP(192, 168, 47, 168);
IPAddress gateway(192, 168, 47, 76);
IPAddress subnet(255, 255, 255, 0);

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

void refreshScreen(float flow, float temperature, int pls) { //refresh the screen
  if (flow>0) digitalWrite(27, HIGH); //if flow exists, turn on the screen
    else digitalWrite(27, LOW);
  int val = 5;
  display.clearDisplay();
  //show flow
  display.setTextSize(0);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.write("Flow: ");
  String s = String(flow, 3) + " L";
  char c[10];
  s.toCharArray(c, 16);
  display.write(c);

  //show temp
  display.setCursor(0,10);
  display.write("Temp: ");
  String s2 = String(temperature, 1) + "  C";
  char c2[10];
  s2.toCharArray(c2, 16);
  display.write(c2); 
  for(int i = 1; i<84;++i) {
    flowARR[i -1] = flowARR[i];
  }
  flowARR[83] = pls;
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
  float amount_difference = (float)pulses/660;
  int pls;
  if (pulses == pulses)
    pls = pulses;
  else pls = 0;
  //amount_difference = random(100);
  
  float read = analogRead(temp_pin);
  float r = (208896000/read)-51000;
  float temperature = (3828.21168/(log10(r/0.000145475)))-423.80602;
  add_json_object("amount_difference", amount_difference);
  //add_json_object("amount_difference", pulses);
  add_json_object("temperature", temperature);
  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
  refreshScreen((float)amount_difference/((millis()-lastTime)/1000), temperature, pls);
  //refreshScreen(pulses, temperature);
  lastTime = millis();
  pulses = 0;
}


void setupRouting() {
  server.on("/data", getData);
  server.begin();
}

void pulseCounter() {
  pulses++;
  Serial.println("puls");
}

void IRAM_ATTR ISR() {
    pulses++;
}
void setup() {
  // put your setup code here, to run once:
  
  Serial.begin (9600);
  pinMode(27, OUTPUT);
  pinMode(flow_pin, INPUT);
  attachInterrupt(flow_pin, ISR, CHANGE);
  displayInit();
  connectToWiFi();
  setupRouting();
}

void loop() {
  server.handleClient();
}