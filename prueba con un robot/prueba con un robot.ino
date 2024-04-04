#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"

// Set your access point network credentials
const char* ssid     = "ESP1";
const char* password = "Passwordsupersegura";

int deploy  = 0;
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

JsonDocument readpot() {
  JsonDocument sensores;
  
  int potenciometro1 = analogRead(35);
  
  Serial.print("valor del potenciometro: ");
  Serial.println(potenciometro1);
  
  JsonArray data = sensores["sensor_1"].to<JsonArray>();
  JsonArray data2= sensores["sensor_2"].to<JsonArray>();
  data.add(potenciometro1);
  data.add(35);
  data2.add(44);
  data2.add(21);
  return sensores;
}

JsonDocument power(){
  JsonDocument rssi;
  int valor=WiFi.RSSI();
  if (valor<-70){
    deploy++;
  }
  rssi["rssi"]=valor;
  rssi["despliegue"]=deploy;
  Serial.print("la señal de despliegue es: ");
  Serial.println(deploy);
  return rssi;
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  WiFi.begin(ssid,password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/ping",HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200,"text/plain","1");
  });
  
  server.on("/sensores.json", HTTP_GET, [](AsyncWebServerRequest *request){
    String response; 
    serializeJsonPretty(readpot(), response);
    request->send(200, "application/json", response);
  });

  server.on("/RSSI.json", HTTP_GET, [](AsyncWebServerRequest *request){
    String response; 
    serializeJsonPretty(power(), response);
    request->send(200, "application/json", response);
  });
  
  

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  // Start server
  server.begin();
}
 
void loop(){ 
}