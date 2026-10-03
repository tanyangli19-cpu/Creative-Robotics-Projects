#include <Servo.h>
#include <FastLED.h>

// ---------------------------
// 1. 硬件引脚及可调参数定义
// ---------------------------

// 舵机配置
const int SERVO_PIN = 9;         
Servo myServo; 
const int INITIAL_ANGLE = 90;    // 舵机的初始角度
const int MOVE_DELAY = 30;       // 舵机每步移动的延迟 (毫秒)

// RGB灯环配置 (假设 APA102 类型，使用 FastLED 库)
// 假设：RGB = DATA, DUOT = CLOCK
const int RGB_DATA_PIN = 6;      
const int DUOT_CLOCK_PIN = 7;    
const int NUM_LEDS = 12;         // HS-F12A 环形灯上的 LED 数量（请根据实际数量修改）

// FastLED 数组和对象
CRGB leds[NUM_LEDS];
const long HOLD_DURATION = 5000; // 持续时间 5 秒 (5000毫秒)

// ---------------------------
// 2. 状态机数据结构定义
// ---------------------------

// 定义亮度常量 (FastLED 亮度 0-255)
#define BRIGHTNESS_HIGH   200 
#define BRIGHTNESS_MEDIUM 100
#define BRIGHTNESS_LOW    30

// 存储状态机配置的结构体
struct StateConfig {
  int offsetAngle;       // 舵机相对初始位置的偏移角度
  CRGB color;            // LED 颜色值 (FastLED CRGB 类型)
  int brightness;        // LED 亮度值 (0-255)
};

// 状态配置表 (索引 0 对应用户输入 1，索引 5 对应用户输入 6)
const StateConfig states[] = {
  // 1: 绿, 大, 80度
  {80, CRGB::Green, BRIGHTNESS_HIGH},
  // 2: 绿, 中, 60度
  {60, CRGB::Green, BRIGHTNESS_MEDIUM},
  // 3: 绿, 小, 40度
  {40, CRGB::Green, BRIGHTNESS_LOW},
  // 4: 红, 小, 20度
  {20, CRGB::Red, BRIGHTNESS_LOW},
  // 5: 红, 中, 10度
  {10, CRGB::Red, BRIGHTNESS_MEDIUM},
  // 6: 红, 大, 0度 (不动)
  {0,  CRGB::Red, BRIGHTNESS_HIGH}
};


// ---------------------------
// 3. 函数声明
// ---------------------------
void moveServoSlowly(int startAngle, int endAngle);
void setRingColor(CRGB color, int brightness);
void printInstructions();


// ---------------------------
// 4. setup 函数
// ---------------------------
void setup() {
  Serial.begin(9600);
  
  // 舵机初始化
  myServo.attach(SERVO_PIN);
  
  // LED灯环初始化 (使用 FastLED)
  // 假设芯片类型为 APA102，颜色顺序 BGR (常见设置)
  FastLED.addLeds<APA102, RGB_DATA_PIN, DUOT_CLOCK_PIN, BGR>(leds, NUM_LEDS);
  
  // 初始状态：灯熄灭
  setRingColor(CRGB::Black, 0); // 黑色，亮度为 0

  // 舵机归位到初始角度
  myServo.write(INITIAL_ANGLE);
  delay(1000); 
  
  Serial.println("--- 舵机与 4 针 RGB 灯环状态机控制程序 ---");
  Serial.print("舵机初始角度已归位至: ");
  Serial.print(INITIAL_ANGLE);
  Serial.println(" 度");
  Serial.println("注意: 假设 RGB 为数据线, DUOT 为时钟线，芯片为 APA102。");
  printInstructions();
}


// ---------------------------
// 5. loop 函数 (主循环)
// ---------------------------
void loop() {
  if (Serial.available() > 0) {
    int input = Serial.parseInt();
    
    while (Serial.available() > 0) {
      Serial.read();
    }
    
    if (input >= 1 && input <= 6) {
      executeStateMachine(input);
    } else {
      Serial.print("无效输入: ");
      Serial.print(input);
      Serial.println(". 请输入 1 到 6 之间的数字。");
      printInstructions();
    }
  }
}


// ---------------------------
// 6. 核心状态机执行函数
// ---------------------------
void executeStateMachine(int stateValue) {
  int stateIndex = stateValue - 1; 
  StateConfig config = states[stateIndex];
  
  // --------------------------------------------------
  // A. 计算目标角度并开始运动
  // --------------------------------------------------
  int targetAngle = INITIAL_ANGLE + config.offsetAngle;
  if (targetAngle > 180) targetAngle = 180;
  
  Serial.print("\n--- 状态 ");
  Serial.print(stateValue);
  Serial.println(" 执行中 ---");
  Serial.print("1. 舵机开始运动，偏移角度: ");
  Serial.print(config.offsetAngle);
  Serial.print(" 度");
  
  moveServoSlowly(INITIAL_ANGLE, targetAngle);

  // --------------------------------------------------
  // B. 灯环变色、调整亮度并保持
  // --------------------------------------------------
  Serial.print("2. 灯环点亮，持续 ");
  Serial.print(HOLD_DURATION / 1000);
  Serial.println(" 秒");
  
  setRingColor(config.color, config.brightness);
  
  delay(HOLD_DURATION); // 保持指定时间
  
  // --------------------------------------------------
  // C. 舵机归位，灯环熄灭
  // --------------------------------------------------
  Serial.println("3. 舵机归位，灯环熄灭");

  moveServoSlowly(targetAngle, INITIAL_ANGLE);

  // 灯环熄灭 (CRGB::Black = 0, 0, 0)
  setRingColor(CRGB::Black, 0); 

  Serial.println("------------------------------------");
  Serial.println("操作完成，等待下一次输入。");
  printInstructions();
}


// ---------------------------
// 7. 辅助功能函数
// ---------------------------

/**
 * 舵机缓慢转动
 */
void moveServoSlowly(int startAngle, int endAngle) {
  if (endAngle > startAngle) {
    for (int pos = startAngle; pos <= endAngle; pos += 1) { 
      myServo.write(pos);
      delay(MOVE_DELAY); 
    }
  } else if (endAngle < startAngle) {
    for (int pos = startAngle; pos >= endAngle; pos -= 1) { 
      myServo.write(pos);
      delay(MOVE_DELAY);
    }
  }
  myServo.write(endAngle);
}

/**
 * 设置灯环颜色和亮度 (使用 FastLED)
 */
void setRingColor(CRGB color, int brightness) {
  FastLED.setBrightness(brightness);
  fill_solid(leds, NUM_LEDS, color); // 将所有灯填充为指定颜色
  FastLED.show(); 
}

/**
 * 打印使用说明 
 */
void printInstructions() {
  Serial.println("------------------------------------");
  Serial.println("请在串口监视器中输入 1 到 6 之间的数字，以触发对应的状态:");
}