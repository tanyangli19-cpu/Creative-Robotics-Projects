/*
 * Project: Xiao Liu Ren Fortune Teller_小六壬数字实体化状态机
 * - Main Hardware / 硬件平台: Arduino Uno R4 WiFi
 * Components / 组件: 
 * - HC-SR04 Ultrasonic Sensor / 超声波传感器
 * - WS2812B LED Strip (sum: 6*12=72 LEDs) / 灯带
 * - MG996R Servo / 舵机
 * - Push Button(3pins) / 按钮
 * - 5v outside battery/ 5V 额外供电
 */

// --- Libraries ---
#include <WiFiS3.h>           // Library for R4 WiFi connectivity / R4 WiFi
#include <WiFiUdp.h>          // UDP library for NTP / UDP 协议
#include <NTPClient.h>        // Network Time Protocol client / 网络时间
#include <Adafruit_NeoPixel.h>// NeoPixel LED driver / 灯带驱动
#include <Servo.h>            // Servo motor library / 舵机库

// ------------------------------------------------------------------------------------
// 1. Configuration / 参数配置
const char* ssid     = "1234";  // WiFi
const char* password = "biubiubiu";  // Password

// --- Pin Definitions / 引脚定义 ---
const int trigPin   = 8;   // Ultrasonic Trig pin (Output) / 超声波发射
const int echoPin   = 11;   // Ultrasonic Echo pin (Input) / 超声波接收
const int buttonPin = 2;   // Button pin (Input with Pullup) / 按钮
const int ledPin    = 6;   // LED Strip Data pin / 灯带数据
const int servoPin  = 12;   // Servo Signal pin (PWM) / 舵机信号

// --- LED Strip Parameters / 灯带参数 ---
const int GROUPS         = 6;   // 5 Elements + 1 Instruction Group / 5组五行 + 1组指令灯
const int LEDS_PER_GROUP = 12;  // LEDs per group / 每组 12 颗灯
const int TOTAL_LEDS     = GROUPS * LEDS_PER_GROUP; // Total 72 LEDs / 总共 72 颗
// --------------------------------------------------------------------------------
// 2. Global Objects / 全局对象
WiFiUDP ntpUDP; // UDP object for time sync / UDP 对象
// NTP Client: pool.ntp.org
// offset 28800s (UTC+8 Beijing) / NTP 客户端配置 (东八区)
NTPClient time(ntpUDP, "pool.ntp.org", 0);

// NeoPixel Object / 灯带对象
Adafruit_NeoPixel pixels(TOTAL_LEDS, ledPin, NEO_GRB + NEO_KHZ800);
// 参数：LED 总数、数据引脚、以及 LED 的颜色顺序 (GRB) 和传输速率 (800 KHz)。

// Servo Object / 舵机对象
Servo myServo; 

// --- State Machine / 状态机 ---
enum ProgramState {
  STATE_IDLE,         // State 1: Measuring & Waiting for 1st press / 闲置: 测距并等待第一次按键
  STATE_WAIT,         // State 2: Animation done, waiting for 2nd press / 等待: 演示完毕，等待确认
  STATE_EXECUTING     // State 3: Moving Servo & LEDs / 执行: 正在执行物理动作
};
ProgramState currentState = STATE_IDLE; // Init state / 初始状态

// --- Global Variables / 全局变量 ---
int currentNum = 0;        // Distance mapped to 0-9
int calculatedResult = 0;  // Core algorithm result (-2 to 3)
uint32_t elementColors[5]; // Store colors for 5 elements
// --------------------------------------------------------------------------------------------------
// 3. Setup / 初始化
void setup() {
  Serial.begin(9600); // Start Serial Monitor
  
  // --- Ultrasonic and button Pin Modes / 引脚模式 ---
  pinMode(trigPin, OUTPUT); // Trig is Output / 发射脚为输出
  pinMode(echoPin, INPUT);  // Echo is Input / 接收脚为输入
  pinMode(buttonPin, INPUT_PULLUP); // Internal pullup resistor / 启用内部上拉电阻

  // --- Servo Init / 舵机初始化 ---
  myServo.attach(servoPin); // Attach servo to pin / 连接舵机
  myServo.write(0);         // Reset to 0 degrees / 归零复位

  // --- LED Init / 灯带初始化 ---
  pixels.begin();           // Init NeoPixel / 启动灯带
  pixels.setBrightness(80); // Set brightness (safety) / 设置亮度 (保护电源)
  pixels.show();            // Turn off all LEDs / 全灭

  // --- Define Colors / 定义五行颜色 ---
  elementColors[0] = pixels.Color(255, 255, 255); // Metal (White) / 金 (白)
  elementColors[1] = pixels.Color(0, 255, 0);     // Wood (Green) / 木 (绿)
  elementColors[2] = pixels.Color(0, 0, 255);     // Water (Blue) / 水 (蓝)
  elementColors[3] = pixels.Color(255, 0, 0);     // Fire (Red) / 火 (红)
  elementColors[4] = pixels.Color(255, 180, 0);   // Earth (Yellow) / 土 (黄)

  // --- WiFi Connection / 连接 WiFi ---
  Serial.print("Connecting to WiFi..."); // Print status / 打印状态
  WiFi.begin(ssid, password);            // Start connection / 开始连接
  
  // Block until connected / 阻塞直到连接成功
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);         // Wait 500ms / 等待
    Serial.print(".");  // Print dot / 打印点
  }
  Serial.println(" Connected!"); // Connected / 连接成功

  // --- Start NTP / 启动时间服务 ---
  time.begin(); 
}
// ---------------------------------------------------------------------------
// 4. Main Loop / 主循环
void loop() {
  time.update(); // Update network time / 更新时间

  // ------------------------------------------
  // Phase 1: Idle Mode / 阶段一: 闲置模式
  // ------------------------------------------
  if (currentState == STATE_IDLE) {
    measureDistance(); // Continuous measurement / 持续测距

    // Check for 1st Button Press / 检测第一次按键
    if (digitalRead(buttonPin) == LOW) {
      delay(20); // Debounce / 消抖
      if (digitalRead(buttonPin) == LOW) {
        
        // A. Perform Calculation / 执行核心计算
        calculatedResult = calculation();
        
        // B. Run Demo Animation / 播放五行循环演示
        runlightCycle();

        // C. Change State / 切换状态
        currentState = STATE_WAIT;
        Serial.println("Ready for show"); // Prompt
        
        // Wait for release / 等待松手
        while(digitalRead(buttonPin) == LOW); 
      }
    }
  }

  // ------------------------------------------
  // Phase 2: Wait / 阶段二: 等待确认
  // ------------------------------------------
  else if (currentState == STATE_WAIT) {
    
    // Check Serial for 'back' command / 检查串口 'back' 指令
    if (Serial.available() > 0) { //是否有可读数据
      String cmd = Serial.readStringUntil('\n');  // 代表enter
      if (cmd.startsWith("back")) 
      reset(); // Reset if 'back' / 如果是 back 则复位
    }

    // Check for 2nd Button Press / 检测第二次按键
    if (digitalRead(buttonPin) == LOW) {
      delay(20);
      if (digitalRead(buttonPin) == LOW) {
        // Change to Executing State / 切换到执行状态
        currentState = STATE_EXECUTING;
        // Wait for release / 等待松手
        while(digitalRead(buttonPin) == LOW);
      }
    }
  }

  // ------------------------------------------
  // Phase 3: Executing Logic / 阶段三: 执行逻辑
  // ------------------------------------------
  else if (currentState == STATE_EXECUTING) {
    // Execute Servo and LED logic / 执行舵机和灯光逻辑
    SixRenLogic(calculatedResult);
    
    // Auto reset after execution / 执行完自动复位
    reset();
  }
}
// -----------------------------------------------------------------------------
// 5. Helper Functions / 辅助函数

// --- Ultrasonic Measurement / 超声波测距 ---
void measureDistance() {
  // Trigger pulse / 发送触发脉冲
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10); digitalWrite(trigPin, LOW);
  
  // Read Echo / 读取回波
  long distance = pulseIn(echoPin, HIGH) * 0.034 / 2; // Convert to cm / 转为厘米
  
  // Limit to 20cm and Map to 0-9 / 限制在20cm内并映射到0-9
  if (distance > 20) distance = 20;
  currentNum = map(distance, 0, 20, 0, 9);
}

// --- Core Algorithm / 核心算法 ---

int calculation() {
  int seconds = time.getSeconds(); // Get current seconds / 获取秒数
  String lat = "51.4723"; String lon = "-0.0860"; // Fixed Coords
  
  // Print Info / 打印信息
  Serial.print("Num:"); Serial.print(currentNum); Serial.print(" Sec:"); Serial.println(seconds);
  Serial.print("Lat:"); Serial.print(lat); Serial.print(" Lon:"); Serial.print(lon);

  int totalSum = 0;
  // Concatenate Data
  String allData = String(currentNum) + lat + lon + String(seconds);
  
  // Sum all digits
  for (int i = 0; i < allData.length(); i++) {
    if (isDigit(allData.charAt(i))) {
      totalSum += (allData.charAt(i) - '0'); // Char to Int
    }
  }
  // Formula: (Sum % 6) - 2
  int result = (totalSum % 6) - 2;
    Serial.print("\nCalculated Result: "); Serial.println(result);
  return result;
}

// --- Demo Animation / 五行循环演示 ---
void runlightCycle() {
  for (int cycle = 0; cycle < 2; cycle++) { // Loop times
    for (int g = 0; g < 5; g++) { // Iterate 5 groups / 遍历5组
      int start = g * LEDS_PER_GROUP;
      
      // Turn ON group / 点亮该组
      for (int i = 0; i < LEDS_PER_GROUP; i++) pixels.setPixelColor(start + i, elementColors[g]);
      pixels.show();
      delay(888); // Wait 0.5s
      
      // Turn OFF group / 熄灭该组
      for (int i = 0; i < LEDS_PER_GROUP; i++) pixels.setPixelColor(start + i, 0);
      pixels.show();
    }
    delay(500); // Short pause / 短暂间隔
  }
  pixels.clear(); // clear data
  pixels.show(); // Ensure all off
}

// --- Reset Function / 复位函数 ---
void reset() {
  currentState = STATE_IDLE; // Set state to IDLE / 状态设为闲置
  myServo.write(0);          // Reset Servo / 舵机归零
  pixels.clear(); pixels.show(); // Clear LEDs / 关灯
  Serial.println("\nSystem Reset."); // Print msg / 打印复位信息
}

// --- Execute Logic (Switch-Case) / 执行状态机逻辑 ---
void SixRenLogic(int code) {
  int targetAngle = 0;      // Target Servo Angle / 目标角度
  int elementGroupIdx = -1; // Index of Element Group 0,1,2,3,4,5
  uint32_t instrColor = 0;  // Color of Instruction LEDs / 指令灯颜色
  Serial.print("showtime");

  // From the caculation result
  switch (code) {
    case 1: // Da An / 大安
      targetAngle = 55; 
      elementGroupIdx = 1; // Wood / 木
      instrColor = pixels.Color(148, 0, 211); // Strong Purple / 强紫
      break;

    case 2: // Liu Lian / 留连
      targetAngle = 10; 
      elementGroupIdx = 2; // Water / 水
      instrColor = pixels.Color(20, 20, 20);  // Weak Gray (Low Black) / 弱灰
      break;

    case 3: // Su Xi / 速喜
      targetAngle = 40; 
      elementGroupIdx = 3; // Fire / 火
      instrColor = pixels.Color(74, 0, 105);  // Medium Purple / 中紫
      break;

    case -2: // Chi Kou / 赤口
      targetAngle = 5; 
      elementGroupIdx = 0; // Metal / 金
      instrColor = pixels.Color(60, 60, 60);  // Medium Gray / 中灰
      break;

    case -1: // Xiao Ji / 小吉
      targetAngle = 30; 
      elementGroupIdx = 1; // Wood / 木
      instrColor = pixels.Color(37, 0, 52);   // Weak Purple / 浅紫
      break;

    case 0: // Kong Wang / 空亡
      targetAngle = 0; 
      elementGroupIdx = 4; // Earth / 土
      instrColor = pixels.Color(150, 150, 150); // Strong Gray (High Black) / 强灰
      break;
  }

  // 1. Move Servo Slowly / 舵机慢速转动
  ServoSlow(0, targetAngle);

  // 2. Set LEDs / 设置灯光
  pixels.clear();
  // Set Element Group / 点亮五行组
  if (elementGroupIdx != -1) {
    for (int i = 0; i < 12; i++) 
    pixels.setPixelColor(elementGroupIdx * 12 + i, elementColors[elementGroupIdx]);
  }
  // Set Instruction Group (Index 5) / 点亮指令组 (第6组)
  for (int i = 0; i < 12; i++) 
  pixels.setPixelColor(5 * 12 + i, instrColor);
  pixels.show(); 

  // 3. Hold seconds
  delay(6666); 

  // 4. Finish / 收尾
  pixels.clear(); pixels.show(); // Lights off / 关灯
  ServoSlow(targetAngle, 0); // Servo return / 舵机回零
}

// --- Slow Servo Movement / 舵机缓动函数 ---
void ServoSlow(int start, int end) {
  // Move positively / 正向转
  if (start < end) {
    for (int pos = start; pos <= end; pos++) { 
      myServo.write(pos); delay(30); // 30ms delay per degree / 每度延时30ms
    }
  } 
  else {
    // Move negatively / 反向转
    for (int pos = start; pos >= end; pos--) { 
      myServo.write(pos); delay(30); 
    }
  }
}