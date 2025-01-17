#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "Wire.h"
#include "WiFiUdp.h"
#include "ArduinoJson.h"
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

Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBY, 5000 ,8,1 );//SE definen los motores 
Motor motor2 = Motor(BIN1, BIN2, PWMB, offsetB, STBY, 5000 ,8,2 );//

const char* ssid = "ESP1";
const char* password = "Passwordsupersegura";
uint8_t broadcastAddress1[] = {0x30, 0xc9, 0x22, 0xd2, 0x0d, 0x6c};// dependiendo de cuantos modulos es cuantas direcciones mac agregar
uint8_t broadcastAddress2[] = {0xE8, 0x6b , 0xea, 0xf2, 0xbb, 0xd0};//actualmente está con solo 2 modulos

AsyncWebServer server(80);//se define el server http donde van a estar las partes web.

char packetBuffer[255];
WiFiUDP udp;//socket udp desde donde va a leer la entrada del control.
WiFiUDP udpcamara;//socket udp donde se va a enviar la señal a la camara para que se cambie de router.

unsigned int LocalPort = 4444;//puerto del control
unsigned int localPort3 = 4446;//puerto de la camara

IPAddress local_IP(192, 168, 1, 101);//ip local en la red LAN
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

IPAddress remote1_IP(192,168,1,100);//ip del computador

typedef struct movimiento {
  int rightX = 0;
  int rightY = 0;
  int leftX = 0;
  int leftY = 0;
  int brazostatus= 1;
  int LEFT= 0;
  int RIGHT= 0;
  int R1= 0;
  int L1=0;
  int UP=0;
  int DOWN=0;
  int dep=0;
} movimiento;

int deploy  = 0;// numero id a desplegar
int totalmodulos = 2;//total de modulos presentes en el robot
int valorpotencia;//variable para almacenar la medicion de la potencia
unsigned long anterior = 0;
unsigned long primerbajada;//desde donde va a comenzar a contar el tiempo de baja señal para el despliegue
unsigned long diferencia;//variable que almacena la cantidad de tiempo continua total que ha estado bajo el rango de potencia
unsigned long margen;//variable que almacena la cantidad de tiempo entre dos puntos de bajada de potencia para cambiar el balor de primerabajada.
int listo = 1;//indicador de que se puede desplegar siguiente modulo o no
int humidity; 
int tempC; 
int previo;

JsonDocument potencia;
Adafruit_MPU6050 mpu;
Adafruit_AHTX0 aht;
ScioSense_ENS160      ens160(ENS160_I2CADDR_1);

String nomwifi;

JsonDocument listamodulos;

movimiento control;

esp_now_peer_info_t peerInfo;

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
    Serial.print("Wi-Fi Channel: ");
    Serial.println(WiFi.channel()); 
}

void notify(movimiento control) {
  if(listo == 1){//solo se va a mover cuando este desplegado un modulo
    if (control.L1  ==  1) {
      forward(motor1, motor2, -255);         
      if(deploy == totalmodulos /*ESTe debe ser el ultimo modulo posible*/){
        if(control.LEFT == 1){
          left(motor1,motor2,-255);
        }
        else if(control.RIGHT ==  1){
          right(motor1,motor2,-255);
        }
      }
    } 
    else if (control.R1 == 1) {
      forward(motor1, motor2, 255);
      //movStatus=1;
      if(deploy == totalmodulos/*ESTe debe ser el ultimo modulo posible*/){
        
        if(control.LEFT == 1){
          left(motor1,motor2,255);
        }
        else if(control.RIGHT ==  1){
          right(motor1,motor2,255);
        }
      }
    }
    if (control.R1  ==  0 && control.L1 ==  0 || control.R1  ==  1 && control.L1 ==  1) {
      brake(motor1, motor2);
    }
  }
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

JsonDocument power(){
  JsonDocument rssi;
  int valor=WiFi.RSSI();
  rssi["rssi"]  = valor;
  if(valor<-77 && listo == 1){
    unsigned long actual  = millis();
    Serial.print("actuales milis: "); Serial.println(actual);
    if(anterior == 0){
      diferencia  = 0;
      primerbajada = actual;
    }else{
      diferencia  = actual  - primerbajada;
      margen      = actual  - anterior;
      if(margen>=100/*30 milisegundos pero puede ser ajustable*/){
        Serial.println("se superó el margen de tiempo");
        primerbajada=actual;
        diferencia  = actual  - primerbajada;
      }
      if(diferencia >= 5000 /*5 segundos pero puede ser ajustable*/){
        Serial.print("mandando señales de despliegue");
        deploy++;
        udpcamara.beginPacket();
        udpcamara.printf("1 \r");
        udpcamara.endPacket();
        diferencia=0;
        listo = 0;
      }
    }
    anterior  = actual;
    Serial.print("Valor deploy: ");
    Serial.println(deploy);
    
  }  
  rssi["deploy"]  = deploy;
  rssi["listo"]  = listo;
  control.dep  = deploy;
  return rssi;
}
String agregar(String nom){       //codigo que genera el siguiente SSID al cual el dispositivo se tiene que conectar.
  String nombre = nom;            //recibe el SSID del wifi conectado en ese momento.
  for (int i = 2; i < 10; i++){   // se determinan todos los posibles modulos a los cuales se va a conectar en este caso 9, para más se debe rehacer como identifica los numeros
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
      i = 11;                     // Hace i 11 para salir del bucle.
    }   
  }
  return nombre;
}  
// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  //Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  //Serial.print(macStr);
  //Serial.print(" send status:\t");
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void notFound(AsyncWebServerRequest *request) {
  Serial.println("llegó una petición inesperada");
  request->send(404, "text/plain", "Not found");
}
 
void setup() {
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

  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  WiFi.setSleep(false);
  Serial.println("");
  Serial.print("WiFi connected: ");
  nomwifi =  WiFi.SSID();
  Serial.println(nomwifi);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel()); 
 
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);
   
  // register peer
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  // register first peer
  // se tienen que agregar las direcciones mac de todos los modulos a utilizar ahora hay 2
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // register second peer  
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  server.on("/ping",HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200,"text/plain","1");
  });
  server.on("/sensores.json", HTTP_GET, [](AsyncWebServerRequest *request){
    String response; 
    serializeJsonPretty(leerSensores(), response);
    request->send(200, "application/json", response);
  });
  server.on("/estadoModulos", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("llega un estado");
    String identificador = request->getParam(0)->value();
    Serial.print("id entrante: ");
    Serial.print(identificador);
    if(identificador=="1"){// en caso de ser mas modulos agregarlos
      esp_err_t recv= esp_now_del_peer(broadcastAddress1);
      if(recv == ESP_OK ){
      Serial.println("se elimino el modulo ");
      }
      else{
        Serial.println("fallo");
      }
    }
     if(identificador=="2"){
      esp_err_t recv2= esp_now_del_peer(broadcastAddress2);
      if(recv2 == ESP_OK ){
      Serial.println("se elimino el modulo ");
      }
      else{
        Serial.println("fallo");
      }
    }
    request->send(200);
    WiFi.disconnect();  
    listo=1;
  });
  server.on("/RSSI.json", HTTP_GET, [](AsyncWebServerRequest *request){
    String response;
    char ip = request->client()->remoteIP().toString()[12];
    int ia = (ip - '0');
    if(request->client()->remoteIP() != remote1_IP){
      //Serial.print("1 Id del cliente :");
      //Serial.println(ia-1);
      if(!listamodulos.containsKey(String(ia-1))){
        listamodulos[String(ia-1)] = "0";
      }
    }
    else{
      //Serial.println("es el PC");
      ia=0;
    }  
    serializeJsonPretty(potencia, response);  
    request->send(200, "application/json", response);
  }); 
  server.onNotFound(notFound);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();
  
  udp.begin(LocalPort);
  udpcamara.begin(localPort3);
}

void loop() {
  if(WiFi.status()!= WL_CONNECTED /*&& valorpotencia<-70*/){
    String nel = agregar(nomwifi);
    WiFi.begin(nel.c_str(),password);
    conectaWifi();
    Serial.println("Se conectó");
  }
  potencia=power();
  int desdecontrol = udp.parsePacket();
  if (desdecontrol) {
    JsonDocument control1;
    int len = udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len - 1] = 0;
    deserializeJson(control1,packetBuffer);
    control.rightX = control1["rightX"]; control.rightY = control1["rightY"];
    control.leftX = control1["leftX"]; control.leftY = control1["leftY"];
    control.R1 = control1["R1"]; control.L1 = control1["L1"];
    control.UP = control1["UP"]; control.DOWN = control1["DOWN"]; control.RIGHT = control1["RIGHT"];control.LEFT = control1["LEFT"];
    control.brazostatus =control1["brazostatus"];
  }else{
    control.R1= 0;
    control.L1= 0;
  }
  notify(control);  
  if(listo == 0){
    control.R1= 0;
    control.L1= 0;
  }  
  esp_err_t result = esp_now_send(0, (uint8_t *) &control, sizeof(control));
  /*if (result == ESP_OK) {
    Serial.println("Sent with success");
  }    
  else {
    Serial.println("Error sending the data");
   }*/
  delay(10);
}
