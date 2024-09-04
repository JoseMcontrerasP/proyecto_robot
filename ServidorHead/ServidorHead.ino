#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "Wire.h"
#include "ScioSense_ENS160.h"
#include <Adafruit_AHTX0.h>
#include "Adafruit_MPU6050.h"
#include "Adafruit_Sensor.h"
#include <TB6612_ESP32.h>

#define SERVO_PINA 2

#define AIN1 19 // ESP32 Pin D13 to TB6612FNG Pin AIN1
#define BIN1 5 // ESP32 Pin D12 to TB6612FNG Pin BIN1
#define AIN2 18 // ESP32 Pin D14 to TB6612FNG Pin AIN2
#define BIN2 17 // ESP32 Pin D27 to TB6612FNG Pin BIN2
#define PWMA 23 // ESP32 Pin D26 to TB6612FNG Pin PWMA
#define PWMB 16 // ESP32 Pin D25 to TB6612FNG Pin PWMB
#define STBY 33 // ESP32 Pin D33 to TB6612FNG Pin STBY

const int offsetA = 1;
const int offsetB = 1;

Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBY, 5000 ,8,1 );
Motor motor2 = Motor(BIN1, BIN2, PWMB, offsetB, STBY, 5000 ,8,2 );

int brazoStatus = 1;
int movStatus = -1;
int humidity; 
int tempC; 
int timeout = 5;
int previo;
Adafruit_MPU6050 mpu;
Adafruit_AHTX0 aht;
ScioSense_ENS160      ens160(ENS160_I2CADDR_1);
// Set your access point network credentials
const char* ssid     = "ESP1";
const char* password = "Passwordsupersegura";

IPAddress remote1_IP(192,168,1,100);
IPAddress remote2_IP(192,168,1,170);
String nomwifi;

int rightX; int rightY;
int UP; int DOWN;
int R1; int L1;

int idmin;
int idmax;
int deploy  = 0;
int valorpotencia;

IPAddress local_IP(192, 168, 1, 101);

IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 255, 0);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

JsonDocument listamodulos;
JsonDocument control;

void conectaWifi(){
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
void notify(int R1,int L1) {
  while (L1  ==  1) {
    back(motor1, motor2, -255);         // Reverse Motor 1 and Motor 2 for 1 seconds at full speed
    movStatus=1;
  } 
  while (R1 == 1) {
    forward(motor1, motor2, 255);
    movStatus=0;
    Serial.print("Avanzaln");
  }
  while (R1  ==  0 && L1 ==  0) {
    brake(motor1, motor2);
    movStatus=-1;
  }
  while(R1  ==  1 && L1 ==  1) {
    brake(motor1, motor2);
  }
} 


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
JsonDocument leerSensores() {
  sensors_event_t humidity1, temp;
  sensors_event_t a, g;
  JsonDocument  sensores;
  JsonArray data = sensores["sensor_1"].to<JsonArray>();
  JsonArray data2= sensores["sensor_2"].to<JsonArray>();
  JsonArray data3= sensores["sensor_3"].to<JsonArray>();
  JsonArray data4= sensores["sensor_4"].to<JsonArray>();
  
  aht.getEvent(&humidity1, &temp);
  tempC = (temp.temperature);
  humidity = (humidity1.relative_humidity);
  int MiCS = analogRead(34);
  
  //AHT2X
  Serial.print("Temperatura: "); Serial.print(tempC); Serial.print("°"); Serial.print("\t");
  Serial.print("Humedad: ");     Serial.print(humidity); Serial.print("% rH "); Serial.print("\t");
  //MiCS5524
  Serial.print("MiCS5524: ");    Serial.print(MiCS); Serial.println("\t");
  //ens160
  if (ens160.available()) {
    ens160.set_envdata(tempC, humidity);
    ens160.measure(true);   ens160.measureRaw(true);
    Serial.print("AQI: ");  Serial.print(ens160.getAQI());Serial.print("\t");
    Serial.print("TVOC: "); Serial.print(ens160.getTVOC());Serial.print("ppb\t");
    Serial.print("eCO2: "); Serial.print(ens160.geteCO2());Serial.println("ppm\t");
  }
  //MPU6050
  mpu.getEvent(&a, &g, &temp);
  Serial.print("Aceleración: ");
  Serial.print("  X: ");Serial.print(a.acceleration.x);
  Serial.print(", Y: "); Serial.print(a.acceleration.y);
  Serial.print(", Z: "); Serial.print(a.acceleration.z); 
  Serial.println(" [m/s^2]");
  Serial.print("Rotación: ");
  Serial.print("  X: "); Serial.print(g.gyro.x);
  Serial.print(", Y: "); Serial.print(g.gyro.y);
  Serial.print(", Z: "); Serial.print(g.gyro.z);
  Serial.println(" [rad/s]");
  Serial.print("Temperatura: "); Serial.print(temp.temperature); Serial.println(" [C]");
  //Documento Json
  data.add(ens160.getAQI());
  data.add(ens160.getTVOC());
  data.add(ens160.geteCO2());
  data2.add(MiCS);
  data3.add(temp.temperature);
  data3.add(humidity);
  data4.add(g.gyro.x);
  data4.add(g.gyro.y);
  data4.add(g.gyro.z);
  return sensores;
}
JsonDocument power(int id, JsonDocument estados){
  JsonDocument rssi;
  int valor=WiFi.RSSI();

  if(id !=  0){
    //Serial.println("no es el pc el que hace la peticion");

    idmin = id;
    idmax = id;
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
      if (key >idmax){
        idmax=key;
      }
    }
    Serial.print("id min:");
    Serial.println(idmin);
    if(valor<-80){
      previo++;
      if(id == idmin && previo >= timeout){
        previo  = 0;
        deploy++;
    
      }
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
void notFound(AsyncWebServerRequest *request) {
  Serial.println("llegó una petición inesperada");
  request->send(404, "text/plain", "Not found");
}
AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/control", [](AsyncWebServerRequest *request, JsonVariant &json) {
  JsonObject jsonObj = json.as<JsonObject>();
  Serial.println("llega señal del control");
  request->send(200,"text/plain", "ok");

  int leftX; int leftY;
  int R1; int L1; int R2; int L2;
  int UP; int DOWN; int RIGHT; int LEFT;
  int X; int O; int T; int S;

  rightX = jsonObj["rightX"]; rightY = jsonObj["rightY"];
  leftX = jsonObj["leftX"]; leftY = jsonObj["leftY"];
  R1 = jsonObj["R1"]; L1 = jsonObj["L1"]; R2 = jsonObj["R2"]; L2 = jsonObj["L2"];
  UP = jsonObj["UP"];DOWN = jsonObj["DOWN"];RIGHT = jsonObj["RIGHT"];LEFT = jsonObj["LEFT"];
  X = jsonObj["X"];O = jsonObj["O"];T = jsonObj["T"];S = jsonObj["S"];
  if(brazoStatus>=idmin && brazoStatus<=idmax){
    if(X == 1){
    brazoStatus--;
    }
    else if(T ==  1){
      brazoStatus++;
    }
  }
  notify(R1,L1);
});

void setup(){
  Serial.begin(115200);
  ens160.begin();
  Serial.println(ens160.available() ? "done." : "failed!");
  if (ens160.available()) {
    Serial.print("\tRev: "); Serial.print(ens160.getMajorRev());
    Serial.print("."); Serial.print(ens160.getMinorRev());
    Serial.print("."); Serial.println(ens160.getBuild());
    Serial.println(ens160.setMode(ENS160_OPMODE_STD) ? "done." : "failed!");
  }
  if (!aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    while (1) delay(10);
    Serial.println("AHT20 no detectado");
  }
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
 
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
    case MPU6050_RANGE_2_G:
      Serial.println("+-2G");
      break;
    case MPU6050_RANGE_4_G:
      Serial.println("+-4G");
      break;
    case MPU6050_RANGE_8_G:
      Serial.println("+-8G");
      break;
    case MPU6050_RANGE_16_G:
      Serial.println("+-16G");
      break;
  }
  mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
    case MPU6050_RANGE_250_DEG:
      Serial.println("+- 250 deg/s");
      break;
    case MPU6050_RANGE_500_DEG:
      Serial.println("+- 500 deg/s");
      break;
    case MPU6050_RANGE_1000_DEG:
      Serial.println("+- 1000 deg/s");
      break;
    case MPU6050_RANGE_2000_DEG:
      Serial.println("+- 2000 deg/s");
      break;
  }
 
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
    case MPU6050_BAND_260_HZ:
      Serial.println("260 Hz");
      break;
    case MPU6050_BAND_184_HZ:
      Serial.println("184 Hz");
      break;
    case MPU6050_BAND_94_HZ:
      Serial.println("94 Hz");
      break;
    case MPU6050_BAND_44_HZ:
      Serial.println("44 Hz");
      break;
    case MPU6050_BAND_21_HZ:
      Serial.println("21 Hz");
      break;
    case MPU6050_BAND_10_HZ:
      Serial.println("10 Hz");
      break;
    case MPU6050_BAND_5_HZ:
      Serial.println("5 Hz");
      break;
  }

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
  nomwifi =  WiFi.SSID();
  Serial.println(nomwifi);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  server.addHandler(handler);
  server.on("/ping",HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200,"text/plain","1");
  });
  server.on("/sensores.json", HTTP_GET, [](AsyncWebServerRequest *request){
    String response; 
    serializeJsonPretty(leerSensores(), response);
    request->send(200, "application/json", response);
  });
  server.on("/RSSI.json", HTTP_GET, [](AsyncWebServerRequest *request){
    String response;
    char ip = request->client()->remoteIP().toString()[12];
    int ia = (ip - '0') - 1;
    if(request->client()->remoteIP()!= remote1_IP || request->client()->remoteIP()!= remote2_IP){
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
    request->send(200);
    if (  condicion ==  true ){
      delay(100);
      WiFi.disconnect();
    }  
  });
  server.on("/movstatus", HTTP_GET, [](AsyncWebServerRequest *request){
    String response;
    response = String(movStatus);
    request->send(200, "text/plain", response);
  });
  server.on("/brazostatus", HTTP_GET, [](AsyncWebServerRequest *request){
    String response;
    JsonDocument datosBrazo;
    int variableLocal1;//Que union se selecciona |||UNA VEZ LISTO HAY QUE CAMBIARLE LOS NOMBRES|||
    int variableLocal2;//posicion del servo eje x osea la señal rightX
    int variableLocal3;//posicion del servo eje x osea la señal rightY
    int variableLocal4;//posicion del servo eje z osea la señal UP
    int variableLocal5;//posicion del servo eje z osea la señal DOWN
    variableLocal1  = brazoStatus;//esto probablemente se pueda borrar pero lo dejó asi porque primero que sea funcional
    variableLocal2  = rightX;
    variableLocal3  = rightY;
    variableLocal4  = UP;
    variableLocal5  = DOWN;
    datosBrazo["brazostatus"] = variableLocal1;
    datosBrazo["X"] = variableLocal2;
    datosBrazo["Y"] = variableLocal3;
    datosBrazo["Zu"] = variableLocal4;
    datosBrazo["Zd"] = variableLocal5;
    serializeJsonPretty(datosBrazo,response);
    request->send(200, "application/json", response);
  });
  server.onNotFound(notFound);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  // Start server
  server.begin();
}
void loop(){
  if(WiFi.status()!= WL_CONNECTED /*&& valorpotencia<-70*/){
    String nel = agregar(nomwifi);
    WiFi.begin(nel.c_str(),password);
    conectaWifi();
  }
}