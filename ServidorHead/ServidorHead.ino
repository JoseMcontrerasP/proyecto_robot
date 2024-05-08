#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"

// Set your access point network credentials
const char* ssid     = "ESP1";
const char* password = "Passwordsupersegura";

IPAddress remote1_IP(192,168,1,100);

String nomwifi;

int idmin;
int deploy  = 0;
int valorpotencia;

IPAddress local_IP(192, 168, 1, 101);

IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 255, 0);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

JsonDocument listamodulos;
bool confirmacion(JsonDocument estados){
  bool val;
  for(JsonPair kv : estados.as<JsonObject>()){
    int key = atoi(kv.key().c_str());
    int value = atoi(estados[String(key)]);
    if(value == deploy && deploy  > 0){
      val = true;
    }
    else{
      return false;
    }
  }
  listamodulos.remove(String(idmin));
  return val;
}

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

JsonDocument power(int id, JsonDocument estados){
  JsonDocument rssi;
  int valor=WiFi.RSSI();
  if(id !=  0){
    //Serial.println("no es el pc el que hace la peticion");
    idmin = id;
    //Serial.print("id del cliente: ");
    //Serial.println(id);
    for(JsonPair kv : estados.as<JsonObject>()){
      int key = atoi(kv.key().c_str());
      Serial.print("key: ");
      Serial.println(key);
      if(key  < idmin){
        idmin =  key;
        //Serial.print("nuevo Idmin: ");
        //Serial.println(idmin);
      }
    }
    Serial.print("id min:");
    Serial.println(idmin);
    if(valor<-85 && id == idmin){
      deploy++;
    }
  }
  /*else{
    Serial.println("es el pc el que hace la peticion");
  }*/

  rssi["despliegue"]  = deploy;
  rssi["rssi"]  = valor;
  valorpotencia = valor;
  //Serial.println(deploy);
  return rssi;
}

String agregar(String nom){       //codigo que genera el siguiente SSID al cual el dispositivo se tiene que conectar.
  String nombre = nom;            //recibe el SSID del wifi conectado en ese momento.
  for (int i = 2; i < 4; i++){   // se determinan todos los posibles modulos a los cuales se va a conectar en este caso 10, para más se debe modificar
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
  return nombre;
}

void setup(){
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.begin(ssid,password);
  WiFi.setSleep(false);
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
    char ip = request->client()->remoteIP().toString()[12];
    int ia = (ip - '0') - 1;
    if(request->client()->remoteIP()!= remote1_IP){
      //Serial.print("Id del cliente:");
      //Serial.println(ia);
      if(!listamodulos.containsKey(String(ia))){
        listamodulos[String(ia)] = "0";
      }
    }
    else{
      //Serial.println("es el PC");
      ia=0;
    }  
     serializeJsonPretty(power(ia,listamodulos), response);  
    request->send(200, "application/json", response);
  });
  server.on("/estadoModulos", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("llega un estado");
    String identificador = request->getParam(0)->value();
    String estado = request->getParam(1)->value();
    Serial.print("id entrante: ");
    Serial.print(identificador);
    Serial.print(" Estado: ");
    Serial.println(estado);
    listamodulos[identificador] = estado; 
    bool condicion  = confirmacion(listamodulos);
    if (  condicion ==  true ){
      delay(100);
      WiFi.disconnect();
    }  
    request->send(200);
});
DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  // Start server
  server.begin();
}
void loop(){
  if(WiFi.status()!= WL_CONNECTED /*&& valorpotencia<-70*/){
    String nel = agregar(nomwifi);
    WiFi.begin(nel.c_str(),password);
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
}