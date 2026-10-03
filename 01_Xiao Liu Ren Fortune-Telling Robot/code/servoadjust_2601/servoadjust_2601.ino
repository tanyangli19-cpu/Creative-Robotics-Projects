#include <Servo.h>
const int servoPin = 9; 
Servo myServo; 
const int initialAngle = 0;
const int moveDelay = 30; 

void setup() {
  // 初始化串口通信，用于接收用户输入和输出信息
  Serial.begin(9600);
  
  // 将 Servo 对象连接到指定的引脚
  myServo.attach(servoPin);
  
  // 开机后，舵机调整到初始角度
  myServo.write(initialAngle);
  delay(1000); 
  
  Serial.println("--- 舵机相对角度控制程序 ---");
  Serial.print("初始角度已设置为: ");
  Serial.print(initialAngle);
  Serial.println(" 度（此为运动的起点和终点）");
  printInstructions();
}

void loop() {
  // 检查串口是否有数据输入
  if (Serial.available() > 0) {
    // 读取用户输入（视为相对于初始角度的“偏移量”）
    int offsetAngle = Serial.parseInt();
    
    // 清空输入缓冲区
    while (Serial.available() > 0) {
      Serial.read();
    }
    
    // --------------------------------------------------
    // 1. 计算绝对目标角度
    // --------------------------------------------------
    // 用户输入（offsetAngle）被视为在 initialAngle 基础上正向转动的度数
    int targetAngle = initialAngle + offsetAngle;
    
    Serial.print("\n用户输入偏移量: +");
    Serial.print(offsetAngle);
    Serial.println(" 度");

    // --------------------------------------------------
    // 2. 检查约束条件
    // --------------------------------------------------
    // 检查：1. 用户输入是否为正数 (因为您限定了 0-180)
    // 检查：2. 最终的绝对目标角度是否在舵机的物理范围 (0-180) 内
    if (offsetAngle >= 0 && offsetAngle <= 180) {
      
      if (targetAngle >= 0 && targetAngle <= 180) {
        Serial.print("目标绝对角度计算结果为: ");
        Serial.print(targetAngle);
        Serial.println(" 度");
        
        // 执行舵机操作流程
        executeMovement(initialAngle, targetAngle);
        
      } else {
        Serial.print("错误：目标绝对角度 (");
        Serial.print(targetAngle);
        Serial.println(" 度) 超出舵机物理范围 (0-180 度)。");
        Serial.println("请确保 '初始角度 + 偏移量' 的结果在 0 到 180 之间。");
        printInstructions();
      }
      
    } else {
      Serial.print("无效输入: ");
      Serial.print(offsetAngle);
      Serial.println("。请确保输入角度在 0 到 180 之间 (作为正向偏移量)。");
      printInstructions();
    }
  }
}

/**
 * 打印使用说明
 */
void printInstructions() {
  Serial.println("------------------------------------");
  Serial.println("请输入一个 0 到 180 的正整数，舵机将从初始角度正向转动该度数:");
}

/**
 * 舵机缓慢转动到目标角度，保持，然后归位
 * @param startAngle 运动的起始角度 (始终为 initialAngle)
 * @param endAngle 运动的终止角度 (initialAngle + offset)
 */
void executeMovement(int startAngle, int endAngle) {
  
  // -------------------------------------------------
  // 步骤 1: 缓慢转动到目标角度
  // -------------------------------------------------
  Serial.print("1. 缓慢转动中 (从 ");
  Serial.print(startAngle);
  Serial.print(" 度 到 ");
  Serial.print(endAngle);
  Serial.println(" 度)...");
  
  // 舵机从起始角度（当前位置）缓慢转动到目标角度
  if (endAngle > startAngle) {
    // 正向转动
    for (int pos = startAngle; pos <= endAngle; pos += 1) { 
      myServo.write(pos);
      delay(moveDelay); 
    }
  } else if (endAngle < startAngle) {
    // 反向转动
    for (int pos = startAngle; pos >= endAngle; pos -= 1) { 
      myServo.write(pos);
      delay(moveDelay);
    }
  }
  
  // 确保舵机最终停在精确的目标角度
  myServo.write(endAngle);
  
  // -------------------------------------------------
  // 步骤 2: 保持 1 秒
  // -------------------------------------------------
  Serial.println("2. 到达目标，保持 1 秒...");
  delay(1000); 

  // -------------------------------------------------
  // 步骤 3: 缓慢归位到初始角度
  // -------------------------------------------------
  Serial.print("3. 缓慢归位中 (回到 ");
  Serial.print(startAngle);
  Serial.println(" 度)...");
  
  // 舵机从目标角度缓慢归位到初始角度
  if (startAngle > endAngle) {
    // 归位时正向转动
    for (int pos = endAngle; pos <= startAngle; pos += 1) {
      myServo.write(pos);
      delay(moveDelay); 
    }
  } else if (startAngle < endAngle) {
    // 归位时反向转动
    for (int pos = endAngle; pos >= startAngle; pos -= 1) {
      myServo.write(pos);
      delay(moveDelay);
    }
  }
  
  // 确保舵机最终停在精确的初始角度
  myServo.write(startAngle);
  delay(100); // 最终等待，确保稳定
  
  Serial.println("------------------------------------");
  Serial.println("操作完成，等待下一次输入。");
  printInstructions();
}