#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"
#include "HTTPClient.h"
#include "ESP32Servo.h"
#include "Wire.h"
#include "ScioSense_ENS160.h"
#include <Adafruit_AHTX0.h>
#include "Adafruit_MPU6050.h"
#include "Adafruit_Sensor.h"
#include <TB6612_ESP32.h>

#define AIN1 19
#define BIN1 5
#define AIN2 18
#define BIN2 17
#define PWMA 23
#define PWMB 16
#define STBY 33

const int offsetA = 1;
const int offsetB = 1;

Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBY, 5000 ,8,1 );
Motor motor2 = Motor(BIN1, BIN2, PWMB, offsetB, STBY, 5000 ,8,2 );

#define SERVO_PINA 12
#define SERVO_PINB 13
#define PIN_ACOPLE 14

int id = 1;/* HAY QUE CAMBIARLO PARA CADA MODULO partiendo desde 1, pq la cabeza no tiene id, 
              con el id identificamos que modulo es el que debe prenderse.                 */


// Set your access point network credentials
const char* ssid     = "ESP1";
const char* password = "Passwordsupersegura";

const char* IPhead  = "http://192.168.1.101/RSSI.json";
const char* IPmov  = "http://192.168.1.101/movstatus";
const char* IPbrazo  = "http://192.168.1.101/brazostatus";
const char* IPpost  = "http://192.168.1.101/estadoModulos";

IPAddress local_IP(192, 168, 1, 101+id);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

int answer = 0;
int comprobador = 0;
bool flag = true;

String nomwifi;

int xservoPos = 90;
int zservoPos = 90;
Servo xbrazo;
Servo zbrazo;
Servo acople;

int humidity; 
int tempC;
Adafruit_MPU6050 mpu;
Adafruit_AHTX0 aht;
ScioSense_ENS160      ens160(ENS160_I2CADDR_1);
int status = 0;
int rightY;
int rightX;
int UP  = 0;
int DOWN  = 0;

AsyncWebServer server(80);

JsonDocument leerSensores() {
  sensors_event_t humidity1, temp;
  sensors_event_t a, g;
  JsonDocument  sensores;
  JsonArray data = sensores["sensor_1"].to<JsonArray>();
  JsonArray data2= sensores["sensor_2"].to<JsonArray>();
  JsonArray data3= sensores["sensor_3"].to<JsonArray>();
  JsonArray data4= sensores["sensor_4"].to<JsonArray>();
  
  /*aht.getEvent(&humidity1, &temp);
  tempC = (temp.temperature);
  humidity = (humidity1.relative_humidity);*/
  int MiCS = analogRead(34);
  
  //AHT2X
  /*Serial.print("Temperatura: "); Serial.print(tempC); Serial.print("°"); Serial.print("\t");
  Serial.print("Humedad: ");     Serial.print(humidity); Serial.print("% rH "); Serial.print("\t");*/
  //MiCS5524
  Serial.print("MiCS5524: ");    Serial.print(MiCS); Serial.println("\t");
  //ens160
  /*if (ens160.available()) {
    ens160.set_envdata(tempC, humidity);
    ens160.measure(true);   ens160.measureRaw(true);
    Serial.print("AQI: ");  Serial.print(ens160.getAQI());Serial.print("\t");
    Serial.print("TVOC: "); Serial.print(ens160.getTVOC());Serial.print("ppb\t");
    Serial.print("eCO2: "); Serial.print(ens160.geteCO2());Serial.println("ppm\t");
  }*/
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
  /*data.add(ens160.getAQI());
  data.add(ens160.getTVOC());
  data.add(ens160.geteCO2());*/
  data2.add(MiCS);
  data3.add(temp.temperature);
  //data3.add(humidity);
  data4.add(g.gyro.x);
  data4.add(g.gyro.y);
  data4.add(g.gyro.z);
  return sensores;
}
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
int getRequestMov(const char* servername){
  HTTPClient http;
  http.begin(servername);
  int resultado;
  int httpResponseCode=http.GET();
  while(WiFi.status() == WL_CONNECTED){
    if(httpResponseCode > 0) {
      if(httpResponseCode == HTTP_CODE_OK) {
        resultado=atoi(http.getString().c_str());
        http.end();
        return resultado;
      }
      else{
        Serial.print("error mov");
        resultado = 5;
        http.end();
        return resultado;
      }
    }
    else{
      Serial.println("intentando de nuevo");
      resultado = getRequestMov(servername);
      http.end();
      return resultado;
    }
  }  
  http.end();
  return resultado;

}
JsonDocument getRequestBrazo(const char* servername){
  HTTPClient http;
  JsonDocument doc;
  http.useHTTP10(true);
  http.begin(servername);
  int httpResponseCode=http.GET();
  while(WiFi.status() == WL_CONNECTED){
    if(httpResponseCode > 0) {
      if(httpResponseCode == HTTP_CODE_OK) {
        deserializeJson(doc, http.getStream());
        http.end();
        return doc;
      }
      else{
        Serial.print("error brazo");
        http.end();
        JsonDocument error;
        error["error"]=1;
        return error;
      }
    }
    else{
      Serial.println("intentando de nuevo");
      http.end();
      doc = getRequestBrazo(servername);
      return doc;
    }
  }  
  http.end();
  return doc;

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
void movBrazo(){
  if(rightY < -100){
    xservoPos = 90;
    xbrazo.write(xservoPos);
  }else{
    if (rightX < -10 && xservoPos < 180) {
      xservoPos=xservoPos+5;
      xbrazo.write(xservoPos);
      }

    if (rightX > 10 && xservoPos > 0) {
      xservoPos=xservoPos-5;
      xbrazo.write(xservoPos);
    }
  }
  if (UP  ==  1) {
    zbrazo.write(145);
    delay(10);
  } 
  if (DOWN == 1) {
    zbrazo.write(90);
    delay(10);
  }
}

void moveMotors(bool mtrdir) {
  if (!mtrdir) {
    back(motor1, motor2, -255);
  } 
  else {
    forward(motor1, motor2, 255);
  }
}

void setup(){
  //Puerto Serial
  Serial.begin(115200); 
  //Sensores
  /*ens160.begin();
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
  }*/
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

  //Servos eje X,Z y acople 
  xbrazo.attach(SERVO_PINA);
  xbrazo.write(xservoPos);
  zbrazo.attach(SERVO_PINB);
  zbrazo.write(zservoPos);
  acople.attach(PIN_ACOPLE);
  acople.write(90);
  pinMode(32,OUTPUT); //LED indicador de acople 
  digitalWrite(32,HIGH);
  //La señal para el switch de comunicación
  //pinMode(33, OUTPUT);//switch comunicacion(router) 
  //digitalWrite(33,LOW);
  //configuración del WIFi
  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.setSleep(false);
  //iniciación de wifi
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
  if (answer == id){
    while(flag){
      Serial.println("desconectando modulo de la cola");
      digitalWrite(32,  LOW);// esto debería hace que el servo o stepper que haga de gancho se desenganche.
      acople.write(120);
      delay(3000);//tiempo para que se desenganche.
      
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
        serializeJsonPretty(leerSensores(), response);
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
    int mov = getRequestMov(IPmov);
    if( mov !=  -1){
      moveMotors(mov);
    }
    else{
      brake(motor1, motor2);
    }
    
    JsonDocument algo =  getRequestBrazo(IPbrazo);
    if(algo["brazostatus"]  ==  id){
      rightY=algo["Y"];
      rightX=algo["X"];      
      UP=algo["Zu"];
      DOWN=algo["Zd"];
      movBrazo();
    }
    delay(100);  
  }
}
