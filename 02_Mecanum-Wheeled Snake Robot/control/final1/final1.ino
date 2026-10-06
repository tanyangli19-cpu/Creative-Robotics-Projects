#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>

// ================= 麦轮小车配置 =================
const int EN_FL = 3, IN1_FL = 2, IN2_FL = 4;
const int EN_FR = 6, IN1_FR = 7, IN2_FR = 8;
const int EN_BL = 5, IN1_BL = 12, IN2_BL = 13;
const int EN_BR = 9, IN1_BR = 10, IN2_BR = 11;

bool REVERSE_FL = false; bool REVERSE_FR = false; 
bool REVERSE_BL = false; bool REVERSE_BR = true;
int carSpeed = 150;

// ================= PCA9685 舵机通用配置 =================
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
#define SERVOMIN 150 
#define SERVOMAX 600 

void setServoAngle(uint8_t n, float angle) {
  angle = constrain(angle, 0, 180);
  pwm.setPWM(n, 0, map(angle, 0, 180, SERVOMIN, SERVOMAX));
}

// ================= 1. 蛇形波动 (0-7号) =================
const int numServos = 8;
float amplitude = 35.0, speed = 0.05, phaseShift = M_PI / 4.0, offsetAngle = 90.0;
unsigned long timeStep = 0;
unsigned long lastSnakeTime = 0;

void updateSnake() {
  if (millis() - lastSnakeTime > 15) {
    for (int i = 0; i < numServos; i++) {
      float currentAngle = amplitude * sin((speed * timeStep) - (i * phaseShift)) + offsetAngle;
      setServoAngle(i, currentAngle);
    }
    timeStep++;
    lastSnakeTime = millis();
  }
}

// ================= 2. 机械臂 (8-9号) =================
const int ARM_BASE_PIN = 8;  
const int ARM_END_PIN = 9;   
int currentAngleBase = 90, targetAngleBase = 90;
int currentAngleEnd = 90, targetAngleEnd = 90;
unsigned long lastArmStepTime = 0;
unsigned long lastSequenceDelayTime = 0;
int armSequenceState = 0;

void updateArmMovement() {
  if (millis() - lastArmStepTime > 15) {
    bool isMoving = false;
    
    // 8号舵机保持固定在 50 度
    if (currentAngleBase != 0) {
        currentAngleBase = 0;
        setServoAngle(ARM_BASE_PIN, currentAngleBase);
    }
    
    // 9号舵机平滑步进
    if (currentAngleEnd != targetAngleEnd) {
      currentAngleEnd += (targetAngleEnd > currentAngleEnd) ? 1 : -1;
      setServoAngle(ARM_END_PIN, currentAngleEnd);
      isMoving = true;
    }
    if (isMoving) lastArmStepTime = millis();
  }
}

void updateArmSequence() {
  unsigned long currentTime = millis();
  bool hasReachedTarget = (currentAngleEnd == targetAngleEnd);
  
  if (!hasReachedTarget) {
    lastSequenceDelayTime = currentTime;
    return;
  }

  // 状态机循环：在 70 度和 130 度之间往复运动
  switch (armSequenceState) {
    case 0:
      targetAngleEnd = 70; 
      if (currentTime - lastSequenceDelayTime > 1000) armSequenceState = 1;
      break;
    case 1:
      targetAngleEnd = 5; 
      if (currentTime - lastSequenceDelayTime > 1000) armSequenceState = 0;
      break;
  }
}

// ================= 核心子系统：小车控制 =================
void driveWheel(int en, int in1, int in2, int dir, bool isReversed) {
  if (isReversed) dir = -dir;
  if (dir == 0) { 
    digitalWrite(in1, LOW); digitalWrite(in2, LOW); analogWrite(en, 0); 
  } else if (dir == 1) { 
    digitalWrite(in1, HIGH); digitalWrite(in2, LOW); analogWrite(en, carSpeed); 
  } else if (dir == -1) { 
    digitalWrite(in1, LOW); digitalWrite(in2, HIGH); analogWrite(en, carSpeed); 
  }
}

void move(int fl, int fr, int bl, int br) {
  driveWheel(EN_FL, IN1_FL, IN2_FL, fl, REVERSE_FL);
  driveWheel(EN_FR, IN1_FR, IN2_FR, fr, REVERSE_FR);
  driveWheel(EN_BL, IN1_BL, IN2_BL, bl, REVERSE_BL);
  driveWheel(EN_BR, IN1_BR, IN2_BR, br, REVERSE_BR);
}

// ================= 主循环 =================
void setup() {
  Serial.begin(115200);
  int pins[] = {EN_FL, IN1_FL, IN2_FL, EN_FR, IN1_FR, IN2_FR, EN_BL, IN1_BL, IN2_BL, EN_BR, IN1_BR, IN2_BR};
  for (int i = 0; i < 12; i++) pinMode(pins[i], OUTPUT);
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);
}

void loop() {
  updateSnake();
  updateArmSequence();
  updateArmMovement();
  
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'w': move(1, 1, 1, 1); break;
      case 's': move(-1, -1, -1, -1); break;
      case 'a': move(-1, 1, 1, -1); break;
      case 'd': move(1, -1, -1, 1); break;
      case 'x': move(0, 0, 0, 0); break;
    }
  }
}