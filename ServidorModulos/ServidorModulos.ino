#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"
#include "HTTPClient.h"

// Set your access point network credentials
const char* ssid     = "ESP1";
const char* password = "Passwordsupersegura";

const char* IPhead="http://192.168.1.101/RSSI.json";
int answer;
int requestinterval=5000;
bool flag = true;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

JsonDocument readpot() {
  JsonDocument sensores;
  
  int potenciometro1 = analogRead(35);
  
  Serial.println(potenciometro1);
  
  JsonArray data = sensores["sensor_1"].to<JsonArray>();
  JsonArray data2= sensores["sensor_2"].to<JsonArray>();
  data.add(potenciometro1);
  data.add(35);
  data2.add(44);
  data2.add(21);
  return sensores;
}

int getRequest(const char* servername){
  HTTPClient http;
  http.useHTTP10(true);
  http.begin(servername);
  int resultado;
  int httpResponseCode=http.GET();
  if(httpResponseCode > 0) {
    if(httpResponseCode == HTTP_CODE_OK) {
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, http.getStream());
      resultado = doc["despliegue"];
    }
    else{
      resultado = 0;
      Serial.print("error");
    }
  }
  http.end();
  return resultado;
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(12,OUTPUT);//servo union
  pinMode(14, OUTPUT);//switvh comunicacion(router)
  
  digitalWrite(12,HIGH);
  digitalWrite(14,LOW);
  
  WiFi.begin(ssid,password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  /*server.on("/sensores.json", HTTP_GET, [](AsyncWebServerRequest *request){
    String response; 
    serializeJsonPretty(readpot(), response);
    request->send(200, "application/json", response);
  });
  
  server.on("/ping",HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200,"text/plain","1");
  });


  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  // Start server
  server.begin();*/
}

void loop(){  
  //el if puede cambiarse por un while( answer <1) y haga lo del Jsondocument;
  if(answer >=  1){
    while(flag){
      Serial.print("desprendiendo modulo del robot");
      digitalWrite(12,LOW);// esto debería hace que el servo o stepper que haga de gancho se desenganche.
      digitalWrite(14, HIGH);//este pin lo que haria seria activar un relee o algo que haga switch que prenda el router o esp32 que hace de nodo de conexión;
      Serial.println("Prendiendo nuevo modulo de conexión");
      server.on("/sensores.json", HTTP_GET, [](AsyncWebServerRequest *request){
        String response; 
        serializeJsonPretty(readpot(), response);
        request->send(200, "application/json", response);
      });
      server.on("/ping",HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200,"text/plain","1");
      });
      DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
      // Start server
      server.begin();
      Serial.println("servidor configurandose");
      flag = false;
    }
    Serial.println("servidor funcionando");
  }
  else{
    Serial.println("motores haciendo cosas");
    JsonDocument doc; 
    answer  = getRequest(IPhead);
    delay(1000);
    Serial.print("valor enviado por el modulo cabeza: ");
    Serial.println(answer);
  }
}