#include <Ps3Controller.h>
#include <ESP32Servo.h>
#include <Wire.h>

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

Servo sbrazo;

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
}

void loop() {
  if (Ps3.isConnected()) {
  }
  delay(500);
}
