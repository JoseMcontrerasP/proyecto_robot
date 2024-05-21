#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"
#include "HTTPClient.h"
#include "ScioSense_ENS160.h"
#include <Adafruit_AHTX0.h>
#include <Wire.h>

const char* ssid     = "ESP1";
const char* password = "Passwordsupersegura";
const char* IPhead  = "http://192.168.1.101/RSSI.json";
const char* IPpost  = "http://192.168.1.101/estadoModulos";
int answer = 0;
int comprobador = 0;

int tempC; 
int humidity; 

Servo sbrazo;
Adafruit_AHTX0 aht;
ScioSense_ENS160      ens160(ENS160_I2CADDR_1);
//...

bool flag = true;
String nomwifi;
int id = 2;// HAY QUE CAMBIARLO PARA CADA MODULO partiendo desde 1, pq la cabeza no tiene id, con el id identificamos que modulo es el que debe prenderse.
// Create AsyncWebServer object on port 80
int status = 0;
IPAddress local_IP(192, 168, 1, 101+id);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
AsyncWebServer server(80);


JsonDocument readpot() {
  sensors_event_t humidity1, temp;
  aht.getEvent(&humidity1, &temp);
  tempC = (temp.temperature);
  humidity = (humidity1.relative_humidity);
  int MiCS = analogRead(34);
  Serial.print("Temperatura: "); Serial.print(tempC); Serial.print("°"); Serial.print("\t");
  Serial.print("Humedad: ");     Serial.print(humidity); Serial.print("% rH "); Serial.print("\t");
  Serial.print("MiCS5524: ");    Serial.print(MiCS); Serial.println("\t");
  if (ens160.available()) {
    ens160.set_envdata(tempC, humidity);
    ens160.measure(true);   ens160.measureRaw(true);
    Serial.print("AQI: ");  Serial.print(ens160.getAQI());Serial.print("\t");
    Serial.print("TVOC: "); Serial.print(ens160.getTVOC());Serial.print("ppb\t");
    Serial.print("eCO2: "); Serial.print(ens160.geteCO2());Serial.println("ppm\t");
  JsonDocument sensores;
  JsonArray data = sensores["sensor_1"].to<JsonArray>();
  JsonArray data2= sensores["sensor_2"].to<JsonArray>();
  JsonArray data3= sensores["sensor_3"].to<JsonArray>();
  JsonArray data4= sensores["sensor_4"].to<JsonArray>();
  data.add(ens160.getAQI());
  data.add(ens160.getTVOC());
  data.add(ens160.geteCO2());
  data2.add(MiCS);
  data3.add(tempC);
  data3.add(humidity);
  data4.add(1);
  data4.add(2);
  data4.add(3);
  return sensores;
}}

int getRequest(const char* servername){
  HTTPClient http;
  JsonDocument doc;
  http.useHTTP10(true);
  http.begin(servername);
  int resultado;
  int httpResponseCode=http.GET();
  while(WiFi.status() == WL_CONNECTED){
    if(httpResponseCode > 0) {
      if(httpResponseCode == HTTP_CODE_OK) {
        deserializeJson(doc, http.getStream());
        resultado = doc["despliegue"];
        http.end();
        return resultado;
      }
      else{
        resultado = -1;
        Serial.print("error");
        http.end();
        return resultado;
      }
    }
    else{
      Serial.println("intentando de nuevo");
      http.end();
      resultado = getRequest(servername);
      return resultado;
    }
  }  
  http.end();
  return resultado;

}

void postRequest(const char* servername, int identificador, int status){
  HTTPClient http;
  String estado = "?modulo="  + String(identificador)  + "&status=" + String(status);
  http.begin(servername);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST(estado);
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
  return nombre;
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  
  //Configuracion de sensores...
  ens160.begin();
  Serial.println(ens160.available() ? "done." : "failed!");
  if (ens160.available()) {
    Serial.print("\tRev: "); Serial.print(ens160.getMajorRev());
    Serial.print("."); Serial.print(ens160.getMinorRev());
    Serial.print("."); Serial.println(ens160.getBuild());
    Serial.println(ens160.setMode(ENS160_OPMODE_STD) ? "done." : "failed!");
  }
  if (! aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    while (1) delay(10);
  }
  Serial.println("AHT20 no detectado");
  //...

  pinMode(32,OUTPUT);//servo union
  pinMode(33, OUTPUT);//switch comunicacion(router)
  digitalWrite(32,HIGH);
  digitalWrite(33,LOW);

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

}

void loop(){  
  if (answer == id){
    while(flag){
      Serial.println("Desconectando modulo de la cola");
      digitalWrite(32,  LOW);// esto debería hace que el servo o stepper que haga de gancho se desenganche.
      delay(3000);
      
      Serial.println("Prendiendo nuevo modulo de conexión");
      digitalWrite(33,  HIGH);//este pin lo que haria seria activar un relee o algo que haga switch que prenda el router o esp32 que hace de nodo de conexión;
      delay(3000);
      status = answer;
      postRequest(IPpost, id, status);
      WiFi.disconnect();
      WiFi.begin(agregar(nomwifi).c_str(),"Passwordsupersegura");
      while (WiFi.status()  !=  WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("");
      Serial.print("WiFi connected: ");
      nomwifi = WiFi.SSID();
      Serial.println(nomwifi);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());

      Serial.println("servidor configurandose");
      server.on("/sensores.json", HTTP_GET, [](AsyncWebServerRequest *request){
        String response; 
        serializeJsonPretty(readpot(), response);
        request->send(200, "application/json", response);
      });
      server.on("/ping",HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200,"text/plain", "1");
      });
      DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
      // Start server
      server.begin();
      Serial.println("Servidor configurado");
      flag = false;
    }
    Serial.println("Servidor funcionando");
    delay(1000);
  }
  else{
    answer  = getRequest(IPhead);
    Serial.println(answer); 
    if(answer < id && answer != comprobador){
      Serial.println("se tiene que conectar al ultimo modulo disponible");
      status  = answer;
      postRequest(IPpost, id,  status);
      delay(5000);
      WiFi.disconnect();
      WiFi.begin(agregar(nomwifi).c_str(),password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("");
      Serial.print("WiFi connected: ");
      nomwifi =  WiFi.SSID();
      Serial.println(nomwifi);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      delay(4000);
    }
    //aqui colocar el codigo del movimiento.
    comprobador = answer;
    delay(200);  
  }
}
