#include "WiFi.h"
#include "ArduinoJson.h"
#include "HTTPClient.h"
#include <Ps3Controller.h>
#include <Wire.h>

#define Ledbluetooth 23
const char* ssid     = "ESP1";
const char* password = "Passwordsupersegura";

const char* IPheadcontrol  = "192.168.1.10l";
 
IPAddress local_IP(192, 168, 1, 140);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

String nomwifi;
char packetBuffer[255];
unsigned int localPort = 4444;
WiFiUDP udp;


int rightX = 0;
int rightY = 0;
int leftX  = 0;
int leftY  = 0;
int R1;
int L1;
int R2=0;
int L2=0;
int UP;
int DOWN;
int RIGHT;
int LEFT;
int X;
int O;
int S;
int T;

int brazoStatus = 1;//selector de modulos
int idmin = 1;// id minimo 
int idmax = 2;// id maximo


void notify() {
  leftX  = (Ps3.data.analog.stick.lx);
  leftY  = (Ps3.data.analog.stick.ly);
  rightX = (Ps3.data.analog.stick.rx);
  rightY = (Ps3.data.analog.stick.ry);

  if( Ps3.event.button_down.r1 ){R1=1;}
  if( Ps3.event.button_up.r1 ){R1=0;}
  if( Ps3.event.button_down.l1 ){L1=1;}
  if( Ps3.event.button_up.l1 ){L1=0;}

  if( Ps3.event.button_down.up ){UP=1;}
  if( Ps3.event.button_up.up ){UP=0;}
  if( Ps3.event.button_down.down ){DOWN=1;}
  if( Ps3.event.button_up.down ){DOWN=0;}
  if( Ps3.event.button_down.right ){RIGHT=1;}
  if( Ps3.event.button_up.right ){RIGHT=0;}
  if( Ps3.event.button_down.left ){LEFT=1;}
  if( Ps3.event.button_up.left ){LEFT=0;}

  if( Ps3.event.button_down.square ){S=1;}
  if( Ps3.event.button_up.square ){S=0;}
  if( Ps3.event.button_down.cross ){X=1;}
  if( Ps3.event.button_up.cross ){X=0;}
  if( Ps3.event.button_down.circle ){O=1;}
  if( Ps3.event.button_up.circle ){O=0;}
  if( Ps3.event.button_down.triangle ){T=1;}
  if( Ps3.event.button_up.triangle ){T=0;}
  //Serial.print(R1);  Serial.print(L1);  Serial.print(UP);  Serial.print(DOWN); Serial.print(RIGHT); Serial.println(LEFT);
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
    udp.begin(localPort);
}
void loop(){ 
  if (Ps3.isConnected()) { 
    if(brazoStatus>=idmin && brazoStatus<=idmax){
      if(X == 1 && brazoStatus>1){
      brazoStatus--;
      }
      else if(T ==  1 && brazoStatus<idmax){
        brazoStatus++;
      }
    }
    JsonDocument controlmod;
    udp.beginPacket("192.168.1.101", localPort);
    controlmod["R1"]=R1;
    controlmod["L1"]=L1;
    controlmod["rightX"]=rightX;  
    controlmod["rightY"]=rightY;
    controlmod["leftX"]=leftX;
    controlmod["leftY"]=leftY;
    controlmod["UP"]=UP;
    controlmod["DOWN"]=DOWN;
    controlmod["brazostatus"]=brazoStatus;
    serializeJson(controlmod, udp);
    udp.endPacket();
  }
  delay(10);
}

