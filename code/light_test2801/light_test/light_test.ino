#include <Adafruit_NeoPixel.h>

// ----------------------
// 1. 参数配置
// ----------------------
const int BUTTON_PIN     = 2;    // 按钮引脚
const int LED_PIN        = 6;    // 灯带数据引脚
const int GROUPS         = 5;    // 总共有 5 组 (金木水火土)
const int LEDS_PER_GROUP = 12;   // 每一组有 12 个灯
const int TOTAL_LEDS     = GROUPS * LEDS_PER_GROUP; // 总灯数 = 60

// 初始化 NeoPixel 对象
Adafruit_NeoPixel pixels(TOTAL_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ----------------------
// 2. 颜色定义
// ----------------------
uint32_t elementColors[5];

void setup() {
  Serial.begin(9600);
  
  // 按钮配置：内部上拉，无需电阻
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pixels.begin();
  
  // 设置全局亮度 (0-255)
  // 建议设置在 50-100 之间，以减小电流压力，保护眼睛和电源
  pixels.setBrightness(60); 
  
  pixels.show(); // 初始化全灭

  // 定义五行颜色
  // 1. 金 (Metal) - 白色
  elementColors[0] = pixels.Color(255, 255, 255);
  // 2. 木 (Wood) - 绿色
  elementColors[1] = pixels.Color(0, 255, 0);
  // 3. 水 (Water) - 蓝色
  elementColors[2] = pixels.Color(0, 0, 255);
  // 4. 火 (Fire) - 红色
  elementColors[3] = pixels.Color(255, 0, 0);
  // 5. 土 (Earth) - 黄色
  elementColors[4] = pixels.Color(255, 180, 0);
}

void loop() {
  // 检测按钮是否按下 (LOW 为按下)
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50); // 防抖
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("开始五行循环...");
      
      runGroupCycle();
      
      // 等待松开
      while(digitalRead(BUTTON_PIN) == LOW);
    }
  }
}

/**
 * 执行五行分组循环
 */
void runGroupCycle() {
  // 循环 2 遍
  for (int cycle = 0; cycle < 2; cycle++) {
    
    // 遍历 5 个组 (金->木->水->火->土)
    for (int groupIndex = 0; groupIndex < GROUPS; groupIndex++) {
      
      // --- 点亮当前组 ---
      
      // 计算这一组的起始灯珠编号
      // 例如：第0组是从0开始，第1组是从12开始，第2组是从24开始...
      int startPixel = groupIndex * LEDS_PER_GROUP;
      
      // 将这一组内的 12 个灯全部设为对应颜色
      for (int i = 0; i < LEDS_PER_GROUP; i++) {
        // 这里的 startPixel + i 就是具体的灯珠编号 (0-59)
        pixels.setPixelColor(startPixel + i, elementColors[groupIndex]);
      }
      pixels.show(); // 刷新显示，让这一组亮起
      
      delay(1000); // 每一组亮 1 秒 (可调整)
      
      // --- 熄灭当前组 ---
      
      // 循环开始前，把当前这一组熄灭，实现“前灯熄灭后灯亮”
      for (int i = 0; i < LEDS_PER_GROUP; i++) {
        pixels.setPixelColor(startPixel + i, 0); // 0 代表灭
      }
      pixels.show(); // 刷新显示，让这一组熄灭
      
    } // 这一组结束，进入下一组
    
    delay(200); // 每一遍大循环中间稍作停顿
    
  } // 2遍结束
  
  // 再次确保全灭
  pixels.clear();
  pixels.show();
}