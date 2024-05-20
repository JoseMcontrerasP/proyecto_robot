#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "ESP32Servo.h"
#include "Wire.h"

#define enA 21
#define in1 19
#define in2 18
#define in3 26
#define in4 27
#define enB 14
#define SERVO_PINA 2
#define Ledbluetooth 23

const int motorFreq = 1000;
const int motorResolution = 8;
const int motorAChannel = 3;
const int motorBChannel = 4;
int motorAPWM = 0;
int motorBPWM = 0;
bool motorDir = true;
int servoPos = 90;
int rightX = 0;
int rightY = 0;
int leftX;
int leftY;

// Set your access point network credentials
const char* ssid     = "ESP1";
const char* password = "Passwordsupersegura";

IPAddress remote1_IP(192,168,1,100);
IPAddress remote2_IP(192,168,1,170);
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
JsonDocument control;

Servo sbrazo;
void notify() {
  int speedX = (abs(rightX) * 2);
  int speedY = (abs(rightY) * 2);
  //Levantamiento del brazo
  if (leftY < -100) {
    servoPos = 90;
    sbrazo.write(servoPos);
    delay(10);
  } else {
      if (leftX < -10 && servoPos < 180) {
        servoPos++;
        sbrazo.write(servoPos);
        delay(10);
        }
      if (leftX > 10 && servoPos > 0) {
        servoPos--;
        sbrazo.write(servoPos);
        delay(10);
        }
    }
  if (rightY < 0) {
    motorDir = true;
  } else {
    motorDir = false;
    }

  if (rightX < -10) {
    motorAPWM = speedY - speedX;
    motorBPWM = speedY + speedX;
  } else if (rightX > 10) {
      motorAPWM = speedY + speedX;
      motorBPWM = speedY - speedX;
    } else {
      motorAPWM = speedY;
      motorBPWM = speedY;
      }
  motorAPWM = constrain(motorAPWM, 0, 255);
  motorBPWM = constrain(motorBPWM, 0, 255);
  moveMotors(motorAPWM, motorBPWM, motorDir);

  Serial.print("X = "); Serial.print(rightX);
  Serial.print("Y = "); Serial.print(rightY);
  Serial.print("Motor A = "); Serial.print(motorAPWM); 
  Serial.print("Motor B = "); Serial.println(motorBPWM);
}

void moveMotors(int mtrAspeed, int mtrBspeed, bool mtrdir) {
  if (!mtrdir) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);

  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
  }
  ledcWrite(motorAChannel, mtrAspeed);
  ledcWrite(motorBChannel, mtrBspeed);
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
    if(valor<-80 && id == idmin){
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

void notFound(AsyncWebServerRequest *request) {
  Serial.println("llegó una petición inesperada");
  request->send(404, "text/plain", "Not found");
}

AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/control", [](AsyncWebServerRequest *request, JsonVariant &json) {
  JsonObject jsonObj = json.as<JsonObject>();
  Serial.println("llega señal del control");
  request->send(200,"text/plain", "ok");
  rightX = jsonObj["rightX"];
  Serial.print("rightX: ");
  Serial.println(rightX);
  rightY = jsonObj["rightY"];
  Serial.print("rightY: ");
  Serial.println(rightY);
  notify();
  // ...
});

void setup(){
  Serial.begin(115200);
  sbrazo.attach(SERVO_PINA);
  sbrazo.write(servoPos);
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(Ledbluetooth, OUTPUT);
  ledcSetup(motorAChannel, motorFreq, motorResolution);
  ledcSetup(motorBChannel, motorFreq, motorResolution);
  ledcAttachPin(enA, motorAChannel);
  ledcAttachPin(enB, motorBChannel);

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
  server.addHandler(handler);
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
  /*server.on("/control", HTTP_POST, [](AsyncWebServerRequest *request){
    
    Serial.println("llega señal del control");
    Serial.print("el json es:");
    bool recibecontrol = request->hasParam("body", true);
    Serial.println(recibecontrol);
    //deserializeJson(control, recibecontrol);
    //rightX = control["rightX"];
    //rightY = control["rightY"];
    //notify();
    request->send(200);
  });
  */
  server.onNotFound(notFound);
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