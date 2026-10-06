#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>

// 实例化 PCA9685 驱动对象，默认 I2C 地址为 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// 舵机脉冲长度配置
#define SERVOMIN  150
#define SERVOMAX  600

// 蛇形机器人运动参数 
const int numSnakeServos = 8;    // 蛇身舵机数量
float amplitude = 35.0;          // A: 振幅
float speed = 0.05;              // ω: 运动速度系数
float phaseShift = M_PI / 4.0;   // Δφ: 相位差
float offsetAngle = 90.0;        // 舵机中心中立点

unsigned long timeStep = 0;      // 模拟时间 t

// 整体运动控制开关
bool isMoving = false;

void setup() {
  Serial.begin(9600);
  Serial.println("Serpentine & Arm Robot Initialization...");
  Serial.println(">>> 键盘控制指南：发送 'S' 开始动作，发送 'P' 暂停动作 <<<");

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50); // 模拟舵机工作频率 50Hz

  // 初始化：让所有11个舵机（8个蛇身 + 3个机械臂）回到 90度 直线状态
  delay(10);
  for (int i = 0; i < 11; i++) {
    setServoAngle(i, offsetAngle); //
  }
  Serial.println("All servos set to 90 degrees. Waiting 2 seconds...");
  delay(2000); //
}

void loop() {
  // 1. 监听键盘（串口）输入来控制启停
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 's' || cmd == 'S') {
      isMoving = true;
      Serial.println("Status: ACTION STARTED");
    } else if (cmd == 'p' || cmd == 'P') {
      isMoving = false;
      Serial.println("Status: ACTION PAUSED");
    }
  }

  // 2. 如果处于运动状态，则执行核心逻辑
  if (isMoving) {
    // 蛇形躯干运动逻辑 (舵机 0 到 7)
    for (int i = 0; i < numSnakeServos; i++) {
      float currentAngle = amplitude * sin((speed * timeStep) - (i * phaseShift)) + offsetAngle; //
      setServoAngle(i, currentAngle); //
    }

    // 机械臂自动编舞逻辑 (舵机 8, 9, 10)
    // 8号和9号配合升降，10号进行末端挥舞
    float armLift1 = 10.0 * sin(speed * timeStep * 0.8) + 0;          // 8号：较缓慢的主臂升降
    float armLift2 = 20.0 * sin(speed * timeStep * 0.8) + 60.0; // 9号：带 90° 相位差的副臂折叠
    float armTwist = 50.0 * sin(speed * timeStep * 1.5) + 90.0;          // 10号：末端较快速的挥舞/旋转

    setServoAngle(8, armLift1);
    setServoAngle(9, armLift2);
    setServoAngle(10, armTwist);

    // 时间推进
    timeStep++; //
  }
  
  // 短暂延时以控制整体更新循环的平滑度
  delay(15); //
}

// 辅助函数：将角度转换为 PWM 脉冲发送给 PCA9685
void setServoAngle(uint8_t servonum, float angle) {
  // 限制角度在安全范围内以防卡死受损
  if (angle < 10) angle = 10; //
  if (angle > 170) angle = 170; //

  // 将角度映射为 PCA9685 的脉冲长度
  uint16_t pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX); //
  pwm.setPWM(servonum, 0, pulse); //
}