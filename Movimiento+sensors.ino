#include <Ps3Controller.h>
#include "ScioSense_ENS160.h"
#include <Adafruit_AHTX0.h>
#include <ESP32Servo.h>
#include <Wire.h>

#define enA 19
#define in1 18
#define in2 5
#define in3 26
#define in4 27
#define enB 14
#define SERVO_PINA 15
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
int tempC; 
int humidity; 

Servo sbrazo;
Adafruit_AHTX0 aht;
ScioSense_ENS160      ens160(ENS160_I2CADDR_1);

void notify() {
  leftX = (Ps3.data.analog.stick.lx);
  leftY = (Ps3.data.analog.stick.ly);
  rightX = (Ps3.data.analog.stick.rx);
  rightY = (Ps3.data.analog.stick.ry);

  if (leftY < -100) {
    servoPos = 90;
    sbrazo.write(servoPos);
    delay(10);
  } 
  else {
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
  } 
  else {
    motorDir = false;
  }

  int speedX = (abs(rightX) * 2);
  int speedY = (abs(rightY) * 2);

  if (rightX < -10) {
    motorAPWM = speedY - speedX;
    motorBPWM = speedY + speedX;
  } 
  else if (rightX > 10) {
      motorAPWM = speedY + speedX;
      motorBPWM = speedY - speedX;
    } 
  else {
      motorAPWM = speedY;
      motorBPWM = speedY;
    }
  motorAPWM = constrain(motorAPWM, 0, 150);
  motorBPWM = constrain(motorBPWM, 0, 150);

  moveMotors(motorAPWM, motorBPWM, motorDir);

  Serial.print("X value = ");
  Serial.print(rightX);
  Serial.print(" - Y value = ");
  Serial.print(rightY);
  Serial.print(" - Motor A = ");
  Serial.print(motorAPWM);
  Serial.print(" - Motor B = ");
  Serial.println(motorBPWM);
}

void onConnect() {
  digitalWrite(Ledbluetooth, HIGH);
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

int serial_putc(char c, struct __file*) {
  Serial.write(c);
  return c;
}

void setup() {
  Serial.begin(115200);
  Ps3.attach(notify);
  Ps3.attachOnConnect(onConnect);
  Ps3.begin("00:00:00:00:00:01");
  while (!Serial) {}
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
}

void loop() {
 if (!Ps3.isConnected())
    return;
  sensors_event_t humidity1, temp;
  aht.getEvent(&humidity1, &temp);
  tempC = (temp.temperature);
  humidity = (humidity1.relative_humidity);
  int MiCS = analogRead(34);
  Serial.print("Temperatura: "); Serial.print(tempC); Serial.print("°"); Serial.print("\t");
  Serial.print("Humedad: "); Serial.print(humidity); Serial.print("% rH "); Serial.print("\t");
  Serial.print("MiCS5524: "); Serial.print(MiCS); Serial.println("\t");
  if (ens160.available()) {
    ens160.set_envdata(tempC, humidity);
    ens160.measure(true);
    ens160.measureRaw(true);
    Serial.print("AQI: ");Serial.print(ens160.getAQI());Serial.print("\t");
    Serial.print("TVOC: ");Serial.print(ens160.getTVOC());Serial.print("ppb\t");
    Serial.print("eCO2: ");Serial.print(ens160.geteCO2());Serial.println("ppm\t");
  }
  delay(500);
}
