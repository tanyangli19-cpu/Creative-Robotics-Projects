# Biomimetic Snake Robot

## 基于麦克纳姆轮底盘的仿生蛇形舞台机器人

> **Mecanum Base · Robotic Arm Head · Eight-Servo Tail · Xbox Control**

这是一个为 **1001 Nights** 机器人戏剧项目开发的仿生蛇机器人。

项目最初来自一个很简单的想法：

> **如果把原本固定在桌面上的机械臂安装到一个可以移动的底盘上，它能不能变成一个具有角色和动作表达能力的机器人？**

在后续的舞台项目中，我们把这个想法发展成了一条机械蛇。

最终机器人由三个主要运动部分组成：

- **麦克纳姆轮底盘**负责机器人在舞台上的移动和转向；
- **简化后的机械臂**作为蛇头，用来表现头部姿态；
- **8 个舵机组成的蛇尾**通过带有相位差的正弦波产生连续的蛇形波浪。

最终机器人可以通过 **Xbox 手柄实时控制**。  
操作者使用左摇杆控制麦克纳姆轮底盘，右摇杆控制蛇头，而蛇尾则由 Arduino 持续生成波浪运动。

<!-- 推荐：这里放最终机器人主图 -->

![Final Biomimetic Snake Robot](assets/final-robot.jpg)

<!-- 推荐：如果有较短 GIF，可以直接放在主图下面 -->

<!-- ![Robot Demo](assets/final-demo.gif) -->

---

## Project Information

**Project:** Biomimetic Snake Robot  
**Type:** Team Project  
**Context:** 1001 Nights Robot Theatre  
**Field:** Creative Robotics / Physical Computing / Mechatronics  
**Main Controller:** Arduino  
**Servo Driver:** PCA9685  
**Motor Driver:** 2 × SN754410NE  
**Input:** Xbox Controller + Python  
**Mobility:** Four-wheel Mecanum Base  
**Active Servos:** 10  
**Tail Joints:** 8  

### My Main Responsibilities

在这个团队项目中，我主要负责：

- 蛇尾机械结构的后续迭代
- 舵机、锁孔、连接件和底盘接口的重新测量
- 3D 打印和装配测试
- 舵机回正与校准代码
- 八舵机蛇形波浪运动代码
- 基于相位差正弦波的 gait control
- 早期键盘控制方案
- 电路接线与系统调试
- ESP32 过热和可能的电流回流问题排查
- 麦轮底盘、蛇头和蛇尾的整机集成测试

---

# 1. Introduction

这个项目的目标并不是制造一条在生物学上完全真实的机器蛇。

真实蛇依靠身体与地面的连续接触、摩擦以及身体形变产生运动。如果直接复制这种运动方式，机械结构和控制系统都会变得非常复杂。

因此，我们采用了一个更适合学生机器人原型和舞台表演的方法：

> **不完整复制真实蛇，而是提取人们最容易识别的蛇形动作。**

我们把蛇的运动拆分成三个相对独立的问题：

```text
Robot Movement
      ↓
Mecanum Base

Head Gesture
      ↓
Robotic Arm Head

Body Wave
      ↓
Eight-Servo Tail
```

麦克纳姆轮负责机器人真正的移动。

机械臂负责表现蛇头的姿态。

蛇尾则不需要推动机器人前进，它的主要任务是产生明显的波浪动作，让整个机器人在移动过程中更像一个有生命的机械动物。

这种设计把：

```text
Mobility
```

和：

```text
Biomimetic Motion
```

分开处理。

这样不仅降低了系统复杂度，也让不同的机器人模块可以分别设计、测试和调试。

---

# 2. Design Development

项目早期并没有直接确定现在看到的八舵机蛇尾。

最开始，我们考虑过一个更偏舞台道具的方案：

> 使用一个移动小车拖动多个装饰性的“小蛇”，形成蛇群的视觉效果。

第一次讨论之后，我们放弃了单纯拖动装饰物的想法。

之后又尝试设计一个类似车厢的蛇尾结构，通过弹力绳把多个部件连接在一起，希望利用小车摆动和弹力回弹产生蛇尾运动。

这个方案虽然结构简单，但尾巴本身缺少主动控制。

我们还讨论过类似自行车转向结构的机械方案。

经过几轮讨论和测试后，最终决定：

> **使用舵机作为蛇尾的主动关节。**

这样每一个关节都有明确的目标角度，蛇尾的运动也可以直接通过程序进行调整。

最终设计逐渐形成：

```text
Four-wheel Mecanum Base
          +
Reduced Robotic Arm Head
          +
Eight-Servo Articulated Tail
```

---

# 3. System Architecture

最终机器人可以分为四个主要系统：

1. Mecanum Mobile Base
2. Robotic Arm Head
3. Eight-Servo Tail
4. Control / Power / Wiring System

整体控制关系如下：

```text
Xbox Controller
       ↓
     Python
 pygame + pyserial
       ↓
Serial Communication
       ↓
     Arduino
      /    \
     /      \
    ↓        ↓
SN754410    PCA9685
  × 2          │
    │          ├── 2 Head Servos
    │          │
    │          └── 8 Tail Servos
    ↓
4 × N20 Motors
    ↓
Mecanum Base
```

最终机器人包含：

- 4 × Mecanum Wheels
- 4 × N20 DC Motors
- 2 × SN754410NE Motor Drivers
- 1 × Arduino
- 1 × PCA9685 Servo Driver
- 2 × Active Head Servos
- 8 × MF90 Tail Servos

最终共有：

> **10 个 active servos**

---

# 4. Robotic Arm Head

机械臂结构来自之前的机器人实验。

在早期项目中，机械臂底座周围安装了超声波传感器。当某一个方向有物体接近时，机械臂可以转向对应方向。

但是这个机械臂也暴露出了一个结构问题：

> 为了支撑上部结构，底部一侧需要加强固定，因此机械臂并不能自由完成 360° 转动。

这也是后来我们开始考虑：

> **让整个机械臂底座本身移动。**

当机械臂被安装到麦克纳姆轮底盘后，它的角色发生了变化。

它不再需要作为一个完整的机械操作臂。

在这个项目中，它主要需要：

> **看起来像蛇头，并完成可见的头部姿态。**

因此最终结构拆除了底部旋转部分，只保留更适合作为蛇头的大臂和小臂部分。

这样可以：

- 减少整体尺寸
- 降低前部重量
- 减少底盘负担
- 保留蛇头需要的动作

最终版本保留两个主动控制的蛇头舵机。

<!-- 插入蛇头照片 -->

![Robotic Arm Head](assets/robotic-arm-head.jpg)

---

# 5. Mecanum Mobile Base

机器人底盘使用四个麦克纳姆轮。

每个麦克纳姆轮由一个 N20 电机驱动。

```text
Front Left       Front Right

Rear Left        Rear Right
```

麦克纳姆轮底盘可以完成：

- 前进
- 后退
- 横向移动
- 转向

这使机器人可以在舞台上比较灵活地改变位置。

因为最初的底盘只有轮子和电机，并没有完整的电机控制板，所以我们需要自己搭建控制电路。

最终使用：

> **2 × SN754410NE**

作为四个 N20 电机的驱动。

一个 SN754410NE 可以控制两个电机，因此两个芯片可以覆盖四轮底盘。

<!-- 插入底盘照片 -->

![Mecanum Base](assets/mecanum-base.jpg)

---

# 6. Motor Driver Development

电机驱动系统经历了多个阶段。

最开始，我们先使用：

```text
Breadboard
```

验证一个 SN754410NE 是否可以正常驱动两个 N20 电机。

验证成功后，再扩展到四个电机。

之后团队制作了面向 Arduino 的手工焊接控制板，希望减少大量面包板跳线。

在此基础上，我们还尝试制作 ESP32 版本，希望：

- 减少线路
- 获得更多可用接口
- 为后续功能留下更多扩展空间

但是这个尝试后来出现了严重的供电和过热问题。

这也成为整个项目中非常重要的一次系统调试经历。

<!-- 插入面包板 / 控制板照片 -->

![Motor Driver Development](assets/motor-driver-development.jpg)

---

# 7. Eight-Servo Tail

蛇尾是整个机器人最明显的仿生部分。

最终蛇尾由：

> **8 个 MF90 舵机**

组成。

每一个舵机都可以理解为蛇身体上的一个关节。

```text
Joint 0
   ↓
Joint 1
   ↓
Joint 2
   ↓
Joint 3
   ↓
Joint 4
   ↓
Joint 5
   ↓
Joint 6
   ↓
Joint 7
```

这些舵机之间通过 3D 打印结构连接。

每一个连接件需要同时解决几个问题：

- 固定舵机本体
- 固定 Servo Horn
- 连接下一节蛇尾
- 留出活动空间
- 支撑蛇尾重量
- 安装辅助小轮

<!-- 插入最终蛇尾 -->

![Eight Servo Tail](assets/eight-servo-tail.jpg)

---

# 8. Tail Mechanical Iteration

蛇尾并不是一次打印成功的。

它经历了多次：

```text
Design
  ↓
3D Printing
  ↓
Assembly
  ↓
Testing
  ↓
Measurement
  ↓
Redesign
```

---

## 8.1 Version 1

第一版已经包含了几个关键结构：

- Servo Pocket
- Servo Horn Area
- Tail Connection
- Support Wheel Mount

但是实际打印后发现：

> CAD 中的理论尺寸和真实 3D 打印后的尺寸存在差异。

由于没有充分考虑打印机误差和装配公差，部分舵机无法顺利装入。

---

## 8.2 Version 2

第二版根据第一次装配的问题重新调整了一些尺寸。

舵机可以更顺利地安装。

但是运动测试后出现了新的问题：

> 顶部 Servo Horn 的连接并不够稳定。

运行一段时间后，结构可能发生滑动。

这意味着：

```text
Programmed Angle
        ≠
Real Joint Angle
```

---

## 8.3 Version 3

第三版重新测量了：

- Servo body dimensions
- Locking holes
- Connector geometry
- Tail connection
- Chassis interface

并加入了更完整的固定和连接设计。

这一版本最终成为实际使用的蛇尾结构。

<!-- 强烈推荐：这里放三个版本零件对比 -->

![Tail Iterations](assets/tail-iterations.jpg)

这个过程让我第一次非常直接地理解：

> **机器人中的 CAD 模型只是设计的开始，真实尺寸、公差、材料和装配方式最终都会影响机械结构能不能真正工作。**

---

# 9. Servo Centering

在正式安装蛇尾之前，还需要解决一个基础问题：

> 每个舵机的机械零点必须保持一致。

因此我首先编写了舵机回正程序。

在安装 Servo Horn 之前，先让所有蛇尾舵机运动到：

```text
90°
```

之后再将舵机固定在 3D 打印结构中。

这样：

```text
Software Centre
      =
Servo Centre
      =
Mechanical Centre
```

可以尽可能保持一致。

基础逻辑类似：

```cpp
for (int i = 0; i < 8; i++) {
    setServoAngle(i, 90);
}
```

完成回正后，再进行机械装配。

<!-- 插入回正代码截图 -->

![Servo Centering](assets/servo-centering.png)

---

# 10. Serpentine Gait

如果八个舵机同时进行完全相同的左右摆动：

```text
Servo 0  → → →

Servo 1  → → →

Servo 2  → → →

Servo 3  → → →
```

那么整个蛇尾只会一起摆动。

它不会产生沿着身体传播的波浪。

因此蛇尾控制的关键是：

> **让相邻关节之间存在一定的相位差。**

---

## 10.1 Core Equation

蛇尾使用带有相位差的正弦波：

```text
θᵢ(t) = A · sin(ωt − i · Δφ) + θ_offset
```

其中：

| 参数 | 含义 |
|---|---|
| `θᵢ(t)` | 第 i 个关节当前的目标角度 |
| `A` | 左右摆动幅度 |
| `ω` | 波随时间变化的速度 |
| `i` | 当前关节编号 |
| `Δφ` | 相邻两个关节之间的相位差 |
| `θ_offset` | 舵机中心角度 |

测试过程中使用过的参数包括：

```cpp
const int numServos = 8;

float amplitude = 35.0;
float phaseShift = PI / 4.0;
float offsetAngle = 90.0;
```

---

## 10.2 Why Phase Shift Matters

如果：

```text
Δφ = 0
```

所有舵机基本同步运动。

而当不同关节之间加入相位差之后：

```text
Joint 0    ~~~~~~~

Joint 1      ~~~~~~~

Joint 2        ~~~~~~~

Joint 3          ~~~~~~~

Joint 4            ~~~~~~~
```

波形就会沿着蛇尾传播。

从视觉上看，蛇尾会形成一个连续移动的曲线。

<!-- 这里非常推荐放 GIF -->

![Serpentine Gait](assets/tail-gait.gif)

---

# 11. Why the Tail Does Not Propel the Robot

从理论上来说，真实蛇可以通过身体波浪与地面摩擦产生推进力。

但是这个项目已经拥有麦克纳姆轮底盘。

因此我们并不需要让蛇尾承担真正的推进任务。

在这个机器人里：

```text
Mecanum Base
     ↓
Real Movement

Servo Tail
     ↓
Visual Snake Motion
```

这种设计减少了蛇尾机械结构需要承担的任务。

它只需要：

> **看起来像正在运动的蛇身体。**

机器人真正的移动则由四个 N20 电机完成。

对于舞台机器人来说，这种方式更加容易控制，也更加稳定。

---

# 12. PCA9685 Servo Control

机器人中存在多个舵机。

为了避免直接占用大量 Arduino PWM 接口，我们使用：

> **PCA9685**

作为统一的舵机 PWM 控制板。

最终版本的通道分配为：

```text
PCA9685

Channel 0
│
├── Tail Servo 1
├── Tail Servo 2
├── Tail Servo 3
├── Tail Servo 4
├── Tail Servo 5
├── Tail Servo 6
├── Tail Servo 7
└── Tail Servo 8
   Channel 0–7

Channel 8
└── Head Servo 1

Channel 9
└── Head Servo 2

Channel 10–15
└── Reserved
```

也就是说，最终机器人实际使用：

> **10 个 active servo channels**

从软件角度看，每个舵机可以被简化为：

```text
Servo Channel
      +
Target Angle
```

这让多舵机控制更加清晰。

---

# 13. Xbox Controller

项目早期首先使用键盘控制麦克纳姆轮。

我编写了第一版基础方向控制，用键盘按键测试：

- 前进
- 后退
- 左右运动
- 停止

之后 Haonan Li 在这个基础上进一步开发了 Python 上位机和 Xbox Controller 控制方式。

最终的控制链路为：

```text
Xbox Controller
        ↓
      pygame
        ↓
      Python
        ↓
     pyserial
        ↓
      Arduino
```

最终操作者不需要通过键盘输入，而可以直接使用手柄控制机器人。

---

# 14. Controller Mapping

Python 会读取 Xbox Controller 的两个摇杆。

数据被转换后通过串口发送给 Arduino。

使用的串口数据格式为：

```text
J,lx,ly,rx,ry
```

其中：

```text
lx + ly
    ↓
Left Stick
    ↓
Mecanum Base
```

负责机器人移动。

而：

```text
rx + ry
    ↓
Right Stick
    ↓
Snake Head
```

负责蛇头两个舵机。

最终操作方式可以简单理解成：

> **左摇杆控制机器人去哪里，右摇杆控制蛇头看哪里。**

同时：

> **八舵机蛇尾继续自动执行波浪运动。**

<!-- 推荐放手柄操作 GIF -->

![Xbox Controller Demo](assets/controller-demo.gif)

---

# 15. Parallel Motion

最终系统一个比较重要的特点是：

> 手柄控制与蛇尾波浪可以同时运行。

Arduino 主循环大致包含：

```cpp
void loop() {

    updateSnake();

    handleSerialInput();

    updateXboxTimeout();

    updateManualArmFromJoystick();
}
```

其中：

```text
updateSnake()
```

持续更新蛇尾波浪。

与此同时：

```text
handleSerialInput()
```

继续接收新的手柄信息。

因此机器人可以：

```text
Move
+
Move Head
+
Wave Tail
```

同时进行。

---

# 16. Manual Control + Procedural Motion

最终机器人并不是一个完全自主机器人。

考虑到这是一个舞台表演机器人，操作者需要根据演员和场景实时调整动作。

因此，我们保留了：

> **Human Operator in the Loop**

最终控制方式可以理解为：

```text
                Robot
               /     \
              /       \
             ↓         ↓

      Manual Control   Procedural Motion

       Base + Head           Tail
            ↓                 ↓
      Xbox Controller    Sine-wave Gait
```

操作者负责：

- 机器人位置
- 移动方向
- 蛇头动作

程序负责：

- 持续生成蛇尾波浪

这样可以减少操作者需要实时控制的变量。

---

# 17. Electronics and Wiring

整个机器人同时包含：

```text
4 × DC Motors
      +
10 × Active Servos
      +
Arduino
      +
PCA9685
      +
Motor Drivers
```

因此电路和供电逐渐成为整个项目中最复杂的问题之一。

机器人最初进行了：

```text
Breadboard Test
      ↓
Perfboard / Custom Wiring
      ↓
Arduino Version
      ↓
ESP32 Experiment
      ↓
Back to Arduino
```

<!-- 插入电路照片 -->

![Electronics](assets/electronics.jpg)

---

# 18. ESP32 Experiment

在 Arduino 控制版本工作后，我们曾经尝试改用 ESP32。

主要原因是希望：

- 减少线路
- 获得更多可用接口
- 为后续功能增加扩展空间

因此团队重新制作了面向 ESP32 的电机控制板。

但是在接线和烧录测试过程中，一块 ESP32 出现了明显过热。

随后系统中还出现过：

- 主板异常发热
- 电池连接线过热
- 供电路径异常

这些问题说明系统中可能存在异常电流路径或供电设计问题。

为了安全完成最终演示，我们最终决定：

```text
ESP32
   ↓
Abandoned

Arduino
   ↓
Final Version
```

<!-- ESP32 / Arduino 对比 -->

![Arduino and ESP32 Test](assets/controller-board-comparison.jpg)

---

# 19. Power Problems

供电是整个项目中最重要的实际问题之一。

机器人需要同时驱动：

```text
4 DC Motors
+
10 Active Servos
+
Main Controller
```

多个舵机同时运动时，会出现较高的瞬时电流需求。

在测试过程中，我们观察到：

- 电机负载时不稳定
- 舵机负载时不稳定
- 控制板发热
- 电池线路过热
- ESP32 异常过热

这些问题让我认识到：

> **机器人供电不能只根据元件标称电压进行设计，还必须考虑真实负载下的电流。**

在后续版本中，电机、舵机和逻辑电路应该拥有更加清晰的供电规划。

---

# 20. Failure-Driven Design

这个项目中很多重要的设计决定，并不是来自最开始的方案。

而是来自：

> **失败。**

---

## 20.1 3D Printing Tolerance

**问题：**

第一版蛇尾在 CAD 中尺寸正确，但是打印后无法顺利装配。

**原因：**

没有充分考虑：

- 打印误差
- 舵机真实尺寸
- 装配公差

**结果：**

重新测量并制作新的连接件。

---

## 20.2 Servo Horn Slipping

**问题：**

第二版结构运行后连接位置可能发生滑动。

**结果：**

重新设计锁孔和连接区域。

---

## 20.3 Wiring Complexity

**问题：**

把更多线路集成到一块板上虽然可以减少线材，但是故障反而更难定位。

**结果：**

认识到系统应该按照：

```text
Individual Module Test
        ↓
Subsystem Test
        ↓
System Integration
```

逐步进行。

---

## 20.4 ESP32 Overheating

**问题：**

ESP32 在测试过程中出现异常发热。

**结果：**

为了安全性和可靠性，最终重新使用 Arduino。

---

## 20.5 Servo Current Peaks

**问题：**

多个舵机同时运动会产生较高峰值电流。

**结果：**

认识到后续版本应该更清楚地规划：

```text
Motor Power
Servo Power
Logic Power
```

---

# 21. Reliability Before Complexity

这个项目让我逐渐认识到：

> **机器人功能更多，并不一定意味着机器人更好。**

在实际舞台环境中，更重要的是：

- 动作可以预测
- 控制足够直接
- 系统可以重复运行
- 供电安全
- 出现问题后容易检查

因此最终机器人没有继续增加更多自由度或更多控制功能。

相反，我们简化了蛇头动作，并优先保证整个系统能够完成最终表演。

对于一个舞台机器人：

> **能够稳定重复的简单动作，比偶尔成功的复杂动作更有价值。**

---

# 22. Final System

最终原型成功整合了：

```text
Four-wheel Mecanum Base
        +
Reduced Robotic Arm Head
        +
Eight-Servo Tail
        +
PCA9685 Servo Control
        +
Arduino
        +
Xbox Controller
```

机器人最终能够：

- 前进和后退
- 横向移动
- 转向
- 调整蛇头姿态
- 持续生成蛇尾波浪
- 使用 Xbox 手柄进行实时控制

<!-- 推荐这里放最完整的一段 GIF -->

![Final Robot Demo](assets/final-demo.gif)

---

# 23. Demo Videos

## Full Robot Demo

[![Watch Full Robot Demo](assets/final-video-cover.jpg)](YOUR_VIDEO_LINK)

## Xbox Controller Demo

[![Watch Xbox Controller Demo](assets/controller-video-cover.jpg)](YOUR_CONTROLLER_VIDEO_LINK)

## 1001 Nights Performance

[![Watch Stage Performance](assets/stage-video-cover.jpg)](YOUR_STAGE_VIDEO_LINK)

---

# 24. Current Limitations

虽然最终机器人已经能够完成基本舞台动作，但它仍然主要是一个：

> **Open-loop Robot**

机器人主要根据：

```text
Human Input
      ↓
Controller
      ↓
Robot Motion
```

运行。

目前它还不能根据舞台环境自动判断：

- 自身姿态
- 周围障碍物
- 演员位置
- 环境变化

因此它目前主要是一个：

> **可控制的表演机器人**

而不是完全自主的机器人。

---

# 25. Next Iteration

如果继续开发这个项目，我认为下面几个方向最值得优先改进。

---

## 25.1 Closed-Loop Sensing

加入：

- IMU
- Distance Sensor
- Proximity Sensor

让机器人能够感知自己的状态或周围环境。

未来可以让蛇头根据演员或目标的位置自动调整姿态。

---

## 25.2 Tail and Base Coordination

目前：

```text
Mecanum Movement
```

和：

```text
Tail Wave
```

主要是两个独立系统。

未来可以进一步建立关系：

```text
Robot Speed
     ↓
Tail Wave Speed
```

以及：

```text
Robot Direction
     ↓
Tail Motion
```

这样整个机器人会更像一个统一的身体，而不是“移动小车 + 摆动尾巴”。

---

## 25.3 Better Power System

当前系统仍然需要分别考虑：

- 电机供电
- 舵机供电
- 控制系统供电

未来可以使用更适合机器人移动平台的电池系统。

但需要同时考虑：

- Discharge Current
- Weight
- Safety
- Runtime

---

## 25.4 Better Balance

蛇头向前伸出时，会让机器人重心向前移动。

未来可以考虑：

- 后部增加配重
- 限制蛇头最大角度
- 调整底盘内部元件位置

提高整体稳定性。

---

## 25.5 Better Wiring

机器人最终线路仍然比较密集。

下一版本可以重点改善：

- Cable Routing
- Connectors
- Modular Wiring
- Maintenance Access
- Fault Isolation

目标是：

> **维修一个模块时，不需要拆开整个机器人。**

---

# 26. Reflection

这个项目让我第一次比较完整地经历了一个机器人系统从设计到运行的过程：

```text
Concept
   ↓
Mechanical Design
   ↓
3D Printing
   ↓
Servo Control
   ↓
Motor Driving
   ↓
Electronics
   ↓
Programming
   ↓
System Integration
   ↓
Debugging
   ↓
Final Demonstration
```

这个过程让我认识到：

> **机器人不是单独的软件、机械或者电子系统，而是这些部分共同工作的结果。**

---

## 26.1 Mechanical Design

蛇尾结构让我更清楚地理解：

> CAD 模型正确，并不代表真实零件一定能够正常装配。

真实机器人设计还必须考虑：

- Printing Tolerance
- Physical Measurement
- Assembly Space
- Locking Method
- Mechanical Stress

---

## 26.2 Multi-Servo Control

从代码上来说，让多个舵机得到不同角度并不复杂。

但是当舵机数量增加后，问题很快会从：

```text
Programming
```

扩展到：

```text
Power
+
Mechanical Load
+
Timing
+
Wiring
+
Calibration
```

这让我第一次真正理解多舵机机器人是一个系统问题，而不仅仅是程序问题。

---

## 26.3 Debugging

项目过程中出现过：

- 打印件无法安装
- Servo Horn 滑动
- 电机供电不稳定
- ESP32 过热
- 电池线路过热
- 可能的电流回流

这些问题最初看起来都是“失败”。

但最终它们帮助我们理解了：

> 哪些设计在真实机器人上是可靠的，哪些设计只是理论上可行。

---

## 26.4 From “It Moves” to “It Moves Reliably”

这个项目最开始的目标很简单：

> 让机器人动起来。

但完成整个系统之后，我认为更重要的问题已经变成：

> **它能不能稳定地再次完成同样的动作？**

如果继续开发，我会更加重视：

- Reliability
- Repairability
- Power Management
- Closed-loop Sensing
- Modular Design

---

# 27. My Contribution

这是一个团队合作项目。

我的主要工作集中在蛇尾结构、运动控制和整机调试。

### Mechanical Development

- 后续蛇尾结构版本修改
- 舵机尺寸重新测量
- Locking hole 调整
- Connector geometry 调整
- Chassis interface 调整
- 3D printing
- Assembly testing

### Motion Control

- Servo centering script
- Eight-servo tail control
- Phase-shifted sine-wave gait
- Gait parameter testing
- Early keyboard control

### System Debugging

- Wiring
- Controller testing
- ESP32 overheating investigation
- Power backflow troubleshooting
- Full-system integration
- Mecanum / Head / Tail combined testing

---

# 28. Team Contribution

## Tanyang Li

主要负责：

- Tail iteration
- 3D printing
- Servo measurement
- Mechanical redesign
- Servo centering
- Phase-shifted gait code
- Initial keyboard control
- Wiring and debugging
- System integration

## Haonan Li

主要负责：

- Early tail structure
- First tail CAD model
- Motor-driver soldering
- Controller-board fabrication
- Connector and cable organisation
- Python control interface
- Xbox controller workflow

## Shared Work

共同完成：

- Robotic arm assembly
- PCA9685 integration
- Electronics testing
- Full robot assembly
- Motion tuning
- Final stage testing

---

# 29. Repository Structure

```text
02_Biomimetic_Snake_Robot/
│
├── README.md
├── Report.pdf
│
├── assets/
│   ├── final-robot.jpg
│   ├── final-demo.gif
│   ├── robotic-arm-head.jpg
│   ├── mecanum-base.jpg
│   ├── motor-driver-development.jpg
│   ├── eight-servo-tail.jpg
│   ├── tail-iterations.jpg
│   ├── tail-gait.gif
│   ├── servo-centering.png
│   ├── electronics.jpg
│   ├── controller-board-comparison.jpg
│   └── controller-demo.gif
│
├── Arduino/
│   └── SnakeRobot/
│       └── SnakeRobot.ino
│
├── Python/
│   └── xbox_controller.py
│
└── CAD/
    └── tail-connectors/
```

---

# 30. Summary

这个项目最终并不是在尝试复制一条真正的蛇。

我们做的是：

> **把复杂的蛇形运动拆解成几个可以设计、编程和控制的机器人行为。**

最终：

```text
Mecanum Base
      ↓
Reliable Mobility

Robotic Arm Head
      ↓
Expressive Gesture

Eight-Servo Tail
      ↓
Travelling Wave

Xbox Controller
      ↓
Real-time Stage Control
```

这些不同的系统最终被组合成一个完整的机器人。

对我来说，这个项目最重要的收获并不是单独学会某一种硬件或代码，而是开始理解：

> **一个真正能够工作的机器人，是机械、电子、控制、供电、软件和调试共同组成的系统。**