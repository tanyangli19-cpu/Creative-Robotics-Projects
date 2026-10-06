#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>

// 实例化 PCA9685 驱动对象，默认 I2C 地址为 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// 舵机脉冲长度配置 (根据你的 MF90 舵机实际情况微调)
// 通常 0度~180度 对应的脉冲宽度为 150 到 600
#define SERVOMIN  150 // 0度时的最小脉冲长度
#define SERVOMAX  600 // 180度时的最大脉冲长度

// 蛇形机器人运动参数
const int numServos = 8;         // 舵机数量
float amplitude = 35.0;          // A: 振幅，最大摆动角度(度)
float speed = 0.05;              // ω: 运动速度系数
float phaseShift = M_PI / 4.0;   // Δφ: 相位差 (PI/4 相当于 45度)
float offsetAngle = 90.0;        // 舵机中心中立点(度)

unsigned long timeStep = 0;      // 模拟时间 t

void setup() {
  Serial.begin(9600);
  Serial.println("Serpentine Robot Initialization...");

  pwm.begin();
  // 模拟舵机的工作频率通常为 50Hz
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);

  // 初始化：让所有舵机回到 90度 直线状态
  delay(10);
  for (int i = 0; i < numServos; i++) {
    setServoAngle(i, offsetAngle);
  }
  Serial.println("All servos set to 90 degrees. Waiting 2 seconds...");
  delay(2000);
}

void loop() {
  for (int i = 0; i < numServos; i++) {
    float currentAngle = amplitude * sin((speed * timeStep) - (i * phaseShift)) + offsetAngle;
    
    setServoAngle(i, currentAngle);
  }

  // 时间推进
  timeStep++;
  
  // 短暂延时以控制整体更新循环的平滑度
  delay(15); 
}

// 辅助函数：将角度 (0-180) 转换为 PWM 脉冲并发送给 PCA9685
void setServoAngle(uint8_t servonum, float angle) {
  // 限制角度在安全范围内以防卡死受损
  if (angle < 10) angle = 10;
  if (angle > 170) angle = 170;

  // 使用 map 函数将角度映射为 PCA9685 的脉冲长度
  uint16_t pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  
  // 设置 PWM 通道 (引脚号, 开启时间, 关闭时间)
  pwm.setPWM(servonum, 0, pulse);
}