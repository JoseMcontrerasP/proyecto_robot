#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"
#include "WiFiMulti.h"

// Set your access point network credentials
const char* ssid     = "ESP1";
const char* password = "Passwordsupersegura";

WiFiMulti WifiMulti;
String nomwifi;
int deploy  = 0;
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

JsonDocument readpot() {
  JsonDocument sensores;
  
  int potenciometro1 = analogRead(35);
  
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
  //Serial.print("la señal de despliegue es: ");
  //Serial.println(deploy);
  return rssi;
}

String agregar(String nom){       //codigo que genera el siguiente SSID al cual el dispositivo se tiene que conectar.
  String nombre = nom;            //recibe el SSID del wifi conectado en ese momento.
  for (int i = 2; i < 10; i++){   // se determinan todos los posibles modulos a los cuales se va a conectar en este caso 10, para más se debe modificar
                                  // y parte el for desde el 2 ya que el valor 1 es el AP original. 
    if(isDigit(nombre[3])){       // compara si el caracter numero 4 del ssid es numerico, aunque diga 3 es 4 ya que la cuenta parte de 0.
      String id;                  // es así ya que se va a trabajar con redes del tipo ESPX donde X es un numero de 1 a 9. 
      id = nombre[3];             // guarda el valor numerico del ssid en una variable string.
      int val = id.toInt();       // transforma el valor del string en su valor numerico int.
      while(i < val+1){           // mientras el valor de i sea menor al valor ya medido para que despues de desplegar distintos modulos  
        i++;                      // no se repitan los nombres.
      }
      nombre += String(i);        // reutiliza la variable nombre donde se encuentra el nombre actual y le agrega el siguiente valor por ejemplo ESP12 
      nombre.remove(3,1);         // elimina el valor del valor actual siguiendo el anterior ejemplo queda ESP2.
      i = 11;                     // Hace i 11 para salir del bucle. creo que no es correcto como lo hice pero eso se arregla despues, mientras funcione.
    }   
  }
  Serial.print("buscando el siguiente AP: ");
  Serial.println(nombre);
  return nombre;
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setAutoReconnect(false);
  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  nomwifi=WiFi.SSID();

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
    JsonDocument rssi =  power();
    int valorpotencia = rssi["rssi"];
    Serial.println(valorpotencia);
    serializeJsonPretty(rssi, response);
    request->send(200, "application/json", response);
    /*if (valorpotencia <-70){
      nomwifi = WiFi.SSID();
      if(WifiMulti.run() == WL_CONNECTED) {
        Serial.println("Aqui para ver el ultimo valor de la potencia");
        WifiMulti.addAP(agregar(nomwifi).c_str(),"Passwordsupersegura");
        WiFi.disconnect();
      }
    }*/
  });
  
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  // Start server
  server.begin();
}
 
void loop(){ 
  delay(1);
  /*if(WifiMulti.run() != WL_CONNECTED) {
    delay(5000);
    Serial.println("desconectado");
    WifiMulti.run();
  }*/
}