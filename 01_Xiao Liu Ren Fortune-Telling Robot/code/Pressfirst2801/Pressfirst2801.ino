// ====================================================================
// 项目名称: WiFi 五行计算与灯光展示系统
// 硬件平台: Arduino Uno R4 WiFi
// 功能概述: 
// 1. 通过 WiFi 获取网络时间。
// 2. 使用超声波传感器测距。
// 3. 按键触发算法计算 (结合时间、距离、坐标)。
// 4. 控制 NeoPixel 灯带进行五行颜色循环展示。
// ====================================================================

// --- 引入必要的库文件 ---
#include <WiFiS3.h>       // Arduino Uno R4 WiFi 专用的 WiFi 库
#include <WiFiUdp.h>      // UDP 协议库 (NTP 时间同步需要用到 UDP 传输)
#include <NTPClient.h>    // 网络时间协议库 (用于从互联网获取精确时间)
#include <Adafruit_NeoPixel.h> // Adafruit 官方灯带驱动库

// ==============================
// 1. 参数配置区域 (用户需修改)
// ==============================
const char* ssid     = "1234";  // 请在此处填写你的 WiFi 名称 (注意大小写)
const char* password = "biubiubiu";  // 请在此处填写你的 WiFi 密码

// --- 硬件引脚定义 ---
const int trigPin   = 8;  // 超声波传感器 Trig (发射) 引脚
const int echoPin   = 11;  // 超声波传感器 Echo (接收) 引脚
const int buttonPin = 2;  // 按钮引脚 (连接的一端，另一端接地)
const int ledPin    = 6;  // WS2812 灯带的数据 (Data) 引脚

// --- 灯带参数设置 ---
const int GROUPS         = 5;   // 将灯带分为 5 组 (对应金、木、水、火、土)
const int LEDS_PER_GROUP = 12;  // 每一组包含 12 颗灯珠
// 计算总灯珠数: 5 * 12 = 60 颗
const int TOTAL_LEDS     = GROUPS * LEDS_PER_GROUP;

// ==============================
// 2. 全局对象与变量初始化
// ==============================
WiFiUDP ntpUDP; // 创建一个 UDP 对象，用于网络通信

// 配置 NTP 客户端:
// 参数1: UDP对象
// 参数2: 时间服务器地址 "pool.ntp.org"
// 参数3: 时区偏移量 28800秒 (60秒*60分*8小时 = 东八区/北京时间)
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800);

// 创建灯带控制对象:
// 参数1: 灯珠总数, 参数2: 引脚号, 参数3: 灯带类型 (GRB排列 + 800KHz频率)
Adafruit_NeoPixel pixels(TOTAL_LEDS, ledPin, NEO_GRB + NEO_KHZ800);

int currentNum = 0;        // 存储由超声波距离映射出来的数字 (0-9)
uint32_t elementColors[5]; // 数组：用于存储五行对应的 5 种颜色值

// ==============================
// 3. Setup 初始化函数 (上电执行一次)
// ==============================
void setup() {
  Serial.begin(9600); // 开启串口通信，波特率 9600，用于在电脑屏幕打印数据
  
  // --- 硬件引脚模式配置 ---
  pinMode(trigPin, OUTPUT); // 超声波发射端设为 输出模式
  pinMode(echoPin, INPUT);  // 超声波接收端设为 输入模式
  
  // 按钮设为 INPUT_PULLUP (内部上拉模式)
  // 作用：引脚默认为高电平，按下按钮连接地线变为低电平。无需外接电阻。
  pinMode(buttonPin, INPUT_PULLUP);

  // --- 灯带初始化 ---
  pixels.begin();           // 启动灯带库
  pixels.setBrightness(60); // 设置全局亮度 (范围0-255)，60 比较适中，保护眼睛
  pixels.show();            // 立即刷新，确保上电时灯带是灭的

  // --- 定义五行颜色 (R, G, B) ---
  elementColors[0] = pixels.Color(255, 255, 255); // Index 0: 金 (白色)
  elementColors[1] = pixels.Color(0, 255, 0);     // Index 1: 木 (绿色)
  elementColors[2] = pixels.Color(0, 0, 255);     // Index 2: 水 (蓝色)
  elementColors[3] = pixels.Color(255, 0, 0);     // Index 3: 火 (红色)
  elementColors[4] = pixels.Color(255, 180, 0);   // Index 4: 土 (黄色/橙色)

  // --- WiFi 连接流程 ---
  Serial.print("正在连接 WiFi...");
  WiFi.begin(ssid, password); // 开始尝试连接 WiFi
  
  // 循环检查连接状态，直到连接成功 (WL_CONNECTED)
  // 注意：如果密码错误或没网，程序会一直卡在这里 (阻塞)
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);         // 每 500ms 检查一次
    Serial.print(".");  // 打印一个点，表示正在加载
  }
  Serial.println(" 连接成功!"); // 连接成功提示

  // --- 启动时间服务 ---
  timeClient.begin(); // 开始从网络获取时间
}

// ==============================
// 4. Loop 主循环 (无限重复执行)
// ==============================
void loop() {
  timeClient.update(); // 每次循环都更新一下时间客户端，保持时间同步

  // --- [模块 A] 超声波持续测距 ---
  // 1. 发送 10 微秒的高电平脉冲触发测距
  digitalWrite(trigPin, LOW); delayMicroseconds(2);  // 先拉低，确保干净
  digitalWrite(trigPin, HIGH); delayMicroseconds(10); // 拉高 10us
  digitalWrite(trigPin, LOW); // 拉低，结束触发
  
  // 2. 读取 Echo 引脚高电平持续的时间 (微秒)
  long duration = pulseIn(echoPin, HIGH);
  
  // 3. 计算距离: 距离 = 时间 * 声速(0.034 cm/us) / 2 (往返)
  long dist = duration * 0.034 / 2;
  
  // 4. 数据处理: 限制在 20cm 以内，并映射为 0-9 的数字
  if (dist > 20) dist = 20;            // 超过 20cm 强行按 20cm 算
  currentNum = map(dist, 0, 20, 0, 9); // 将 0-20 线性映射到 0-9


  // --- [模块 B] 按钮检测与功能触发 ---
  // 检测按钮是否被按下 (LOW 代表按下，因为是上拉模式)
  if (digitalRead(buttonPin) == LOW) {
    delay(20); // 延时 20ms 进行软件消抖 (过滤机械抖动)
    
    // 再次确认按钮状态，确保是真的按下了
    if (digitalRead(buttonPin) == LOW) {
      
      // >>>>>> 步骤 1: 执行计算逻辑 (原代码1的功能) <<<<<<
      Serial.println("\n=== 触发：开始计算 ===");
      performCalculation(); // 调用下方的计算函数

      // >>>>>> 步骤 2: 执行灯光展示 (原代码2的功能) <<<<<<
      Serial.println("=== 展示：五行循环 ===");
      runGroupCycle();      // 调用下方的灯光函数

      Serial.println("=== 流程结束，等待下次按键 ===");
      
      // 死循环等待：只要按钮还没松开，就一直卡在这里
      // 防止按一次按钮触发好几次程序
      while(digitalRead(buttonPin) == LOW); 
    }
  }
  
  delay(50); // 循环末尾小延时，让系统稍微休息一下，稳定运行
}

// ==============================
// 5. 自定义功能函数区域
// ==============================

// --- 函数：执行核心数据记录与计算 ---
void performCalculation() {
  // 1. 获取当前的秒数 (0-59)
  int seconds = timeClient.getSeconds();
  
  // 2. 定义经纬度 (模拟数据，字符串格式)
  String lat = "39.9042"; 
  String lon = "116.4074";

  // 3. 串口打印基础信息
  Serial.print("选中数字(由距离决定): "); Serial.println(currentNum);
  Serial.print("网络秒数: "); Serial.println(seconds);
  Serial.print("地理坐标: "); Serial.print(lat); Serial.print(","); Serial.println(lon);

  // 4. 核心算法: 提取所有数字字符求和
  int totalSum = 0;
  // 将所有相关数据拼成一个长字符串: 例如 "5" + "39.9042" + "116.4074" + "45"
  String allData = String(currentNum) + lat + lon + String(seconds);

  // 遍历字符串中的每一个字符
  for (int i = 0; i < allData.length(); i++) {
    char c = allData.charAt(i); // 获取第 i 个字符
    // 判断该字符是否为数字 ('0'-'9')
    if (isDigit(c)) {
      // 将字符转为整数并累加
      // 原理: 字符 '0' 的 ASCII 码是 48，字符 '5' 是 53
      // '5' - '0' = 53 - 48 = 5 (得到真实的整数值)
      totalSum += (c - '0'); 
    }
  }

  Serial.print("数字位总和: "); Serial.println(totalSum);
  
  // 5. 计算最终结果 core_input
  // 公式: (总和 对 6 取余) - 2
  // 结果范围: (0 到 5) - 2 = -2 到 3
  int core_input = (totalSum % 6) - 2;
  
  Serial.print("最终结果 core_input: "); 
  Serial.println(core_input);
}

// --- 函数：执行五行灯光循环动画 ---
void runGroupCycle() {
  // 外层循环: 控制动画播放的轮数 (这里设为播放 2 遍)
  for (int cycle = 0; cycle < 2; cycle++) {
    
    // 中层循环: 遍历 5 个组 (金->木->水->火->土)
    for (int groupIndex = 0; groupIndex < GROUPS; groupIndex++) {
      
      // --- 点亮当前组 ---
      // 计算当前组的起始灯珠索引 (例如第1组从12开始)
      int startPixel = groupIndex * LEDS_PER_GROUP;
      
      // 内层循环: 点亮这一组内的 12 颗灯
      for (int i = 0; i < LEDS_PER_GROUP; i++) {
        // setPixelColor(灯珠编号, 颜色)
        pixels.setPixelColor(startPixel + i, elementColors[groupIndex]);
      }
      pixels.show(); // 发送数据，让灯真正亮起来
      
      delay(1000); // 保持亮 1 秒
      
      // --- 熄灭当前组 ---
      // 再次遍历这一组，把颜色设为 0 (灭)
      for (int i = 0; i < LEDS_PER_GROUP; i++) {
        pixels.setPixelColor(startPixel + i, 0);
      }
      pixels.show(); // 刷新显示，实现熄灭效果
      
    } // 结束当前组，进入下一组
    
    delay(200); // 一轮大循环结束后，稍作停顿
  }
  
  // 动画全部结束后，再次强制全灭，防止有残留
  pixels.clear();
  pixels.show();
}