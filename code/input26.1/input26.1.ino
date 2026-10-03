#include <WiFiS3.h>      // R4 WiFi 专用库
#include <WiFiUdp.h>     // UDP 协议库 (用于获取时间)
#include <NTPClient.h>   // 网络时间库

// --- 1. 配置 WiFi ---
const char* ssid     = "1234"; 
const char* password = "biubiubiu";

// --- 2. 硬件引脚 ---
const int trigPin = 8;   // 超声波 发送
const int echoPin = 4;   // 超声波 接收
const int buttonPin = 2; // 按钮

// --- 3. 全局对象与变量 ---
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600*0); // 28800秒 = 东八区(+8小时)
int currentNum = 0; // 当前选中的数字

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  // 连接 WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" 连接成功!");

  // 启动时间服务
  timeClient.begin();
}

void loop() {
  timeClient.update(); // 保持时间同步

  // --- A. 超声波测距 ---
  // 发送声波
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // 接收并计算距离 (cm)
  long duration = pulseIn(echoPin, HIGH);
  long dist = duration * 0.034 / 2;
  
  // 限制范围 0-20cm，映射为 0-9
  if (dist > 20) dist = 20;
  currentNum = map(dist, 0, 20, 0, 9);

  // --- B. 按钮检测与核心计算 ---
  if (digitalRead(buttonPin) == LOW) {
    delay(20); // 消抖
    if (digitalRead(buttonPin) == LOW) {
      
      // 1. 获取网络时间的秒数
      int seconds = timeClient.getSeconds();
      
      // 2. 模拟获取经纬度 (作业常用模拟数据)
      // 使用 String 类型方便后续处理
      String lat = "51.50853";  // 纬度
      String lon = "-0.12574"; // 经度
      
      // 3. 打印基础信息
      Serial.println("\n--- 记录数据 ---");
      Serial.print("选中数字: "); Serial.println(currentNum);
      Serial.print("网络秒数: "); Serial.println(seconds);
      Serial.print("地理坐标: "); Serial.print(lat); Serial.print(","); Serial.println(lon);

      // 4. 核心算法：提取所有数字相加
      int totalSum = 0;
      
      // 将所有数据拼成一个长字符串，方便一次性处理
      // 例如: "5" + "39.9042" + "116.4074" + "45"
      String allData = String(currentNum) + lat + lon + String(seconds);
      
      // 遍历字符串中的每一个字符
      for (int i = 0; i < allData.length(); i++) {
        char c = allData.charAt(i);
        // 如果这个字符是数字 (0-9)
        if (isDigit(c)) {
          // 将字符转为整数累加 ('0'的ASCII码是48，所以减去'0'就是真实数字)
          totalSum += (c - '0'); 
        }
      }

      Serial.print("数字位总和: "); Serial.println(totalSum);

      // 5. 最终计算: (和 % 6) - 2
      int core_input = (totalSum % 6) - 2;
      
      Serial.print("最终结果 core_input: "); 
      Serial.println(core_input);
      Serial.println("----------------\n");
      
      delay(500); // 防止按一次触发多次
    }
  }
  delay(50);
}