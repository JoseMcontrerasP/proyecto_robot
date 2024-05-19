#include "WiFi.h"
#include "ArduinoJson.h"
#include "HTTPClient.h"
#include <Ps3Controller.h>
#include <Wire.h>

#define Ledbluetooth 23

const char* ssid     = "ESP1";
const char* password = "Passwordsupersegura";

const char* IPheadcontrol  = "http://192.168.1.101/control";

IPAddress local_IP(192, 168, 1, 140);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

String nomwifi;

int rightX = 0;
int rightY = 0;
int leftX;
int leftY;

void notify() {
  leftX  = (Ps3.data.analog.stick.lx);
  leftY  = (Ps3.data.analog.stick.ly);
  rightX = (Ps3.data.analog.stick.rx);
  rightY = (Ps3.data.analog.stick.ry);
}

void postRequest(const char* servername){
  HTTPClient http;
  JsonDocument control;
  String mensaje;  

  control["rightX"]=rightX;  
  control["rightY"]=rightY;
  serializeJsonPretty(control,mensaje);
  Serial.print(mensaje);
  http.begin(servername);
  http.addHeader("Content-Type", "application/json");
  http.POST(mensaje);
}

void onConnect() {
  digitalWrite(Ledbluetooth, HIGH);
}

void setup(){
    Serial.begin(115200);
    Serial.println("llega acá");
    Ps3.attach(notify);
    Ps3.attachOnConnect(onConnect);
    Ps3.begin("00:00:00:00:00:01");
    //primero el movimiento despues incia el wifi y bluetooth
    WiFi.mode(WIFI_STA);
    if (!WiFi.config(local_IP, gateway, subnet)) {
        Serial.println("STA Failed to configure");
    }
    WiFi.setSleep(true);
    WiFi.begin(ssid,password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("WiFi connected: ");
    nomwifi=WiFi.SSID();
    Serial.println(nomwifi);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}
void loop(){
    if (Ps3.isConnected()) { 
      postRequest(IPheadcontrol);
    }
    delay(100);
}