# 项目结构与内容规划 Spec

## Why
基于 `文档.txt` 的需求1（项目概述）和需求2（功能需求），建立 Agar.io Clone 球球大作战完整复刻版的项目目录结构、核心类骨架和构建配置，为后续分阶段开发奠定基础。

## What Changes
- 在 `d:\code\project\0.0.1` 下建立 `AgarClone_Qt/` 项目目录
- 创建 CMakeLists.txt，配置 Qt6 Widgets + Core 依赖，支持 C++17
- 创建所有源文件骨架（.h / .cpp），含类声明、成员变量、方法签名
- 创建 Constants.h，集中管理游戏常量
- 废弃现有的 `untitled/` 目录中的模板代码（widget.h/cpp/ui）

## Impact
- Affected specs: 无（首个 spec）
- Affected code: 新建 `AgarClone_Qt/` 整个目录，原有 `untitled/` 项目不受影响

## ADDED Requirements

### Requirement: 项目目录结构
系统应当在 `d:\code\project\0.0.1` 下创建 `AgarClone_Qt/` 项目目录，文件结构如下：

```
AgarClone_Qt/
├── CMakeLists.txt
├── main.cpp
├── Constants.h
├── SpatialGrid.h / .cpp
├── Entity.h
├── Ball.h / .cpp
├── Food.h / .cpp
├── SkillBall.h / .cpp
├── Hazard.h / .cpp
├── EjectBall.h / .cpp
├── AIController.h / .cpp
├── GameScene.h / .cpp
├── GameView.h / .cpp
└── UIManager.h / .cpp
```

#### Scenario: 目录结构可编译
- **WHEN** 用户使用 CMake 配置项目
- **THEN** CMake 应成功生成构建文件，无配置错误

### Requirement: CMake 构建配置
系统 SHALL 提供 CMakeLists.txt，满足以下要求：
- cmake_minimum_required(VERSION 3.19)
- project 名称 `AgarClone`
- C++17 标准（`set(CMAKE_CXX_STANDARD 17)`）
- 依赖 Qt6 Core 与 Qt6 Widgets（`find_package(Qt6 REQUIRED COMPONENTS Core Widgets)`）
- `qt_add_executable` 包含所有 .h / .cpp 源文件
- 链接 Qt::Core、Qt::Widgets

#### Scenario: 构建验证
- **WHEN** 执行 `cmake --build .` 
- **THEN** 应编译成功并生成可执行文件

### Requirement: Constants.h 游戏常量
系统 SHALL 提供 Constants.h，集中定义以下全局常量：

- 窗口默认尺寸（1280×720）
- 地图尺寸（5000×5000）
- 帧间隔（16ms ≈ 60 FPS）
- 基础速度、最小半径、最大半径
- 吞食比率（EAT_RATIO = 1.1）
- 分裂半径阈值（18）、吐孢半径阈值（25）
- 豆子半径范围（3~8）、技能球半径（12）、抛射物半径（8）
- 危险物半径（50）、各实体最大数量
- 技能持续时间表、危险物效果参数
- AI 等级参数（反应时间、决策间隔、转向速度）
- 空间网格单元格大小（200）
- 摄像机缩放范围 [0.5, 1.5]
- 胜利条件：总半径 ≥ 2000

#### Scenario: 所有常量可引用
- **WHEN** 其他源文件 `#include "Constants.h"`
- **THEN** 所有游戏常量均可正常访问，编译无错误

### Requirement: Entity 抽象基类
系统 SHALL 提供 Entity 抽象基类（继承 QGraphicsEllipseItem）：

- 属性：位置（QPointF）、半径（qreal）、颜色（QColor）、存活状态（bool）
- 虚方法：`update(qreal dt)`、`onEaten(Entity* eater)`
- 纯虚方法：`type() const -> EntityType`（枚举区分实体类型）

#### Scenario: 实体派生
- **WHEN** Ball、Food、SkillBall、Hazard、EjectBall 继承 Entity
- **THEN** 各子类可正确 override 虚函数并正常工作

### Requirement: Ball 玩家/AI 球体类
系统 SHALL 提供 Ball 类（继承 Entity），包含：

- 成员变量：vx, vy, isPlayer, aiLevel, aiId, splitTimer, mergeTimer, invincibleTimer, skill, skillTimer, debuff, debuffTimer, lastDx, lastDy
- 方法：`move(qreal dx, qreal dy, qreal dt)`, `split(QPointF direction)`, `eject()`, `eat(Entity* target)`, `applySkill(SkillType)`, `applyDebuff(DebuffType)`, `isInvincible()`, `hasShield()`
- 技能/负面效果通过枚举 SkillType / DebuffType 管理
- 半径与速度公式：`speed = baseSpeed * sqrt(minRadius / currentRadius)`

#### Scenario: 玩家球体移动
- **WHEN** 用户按 WASD 键
- **THEN** 玩家球体应朝对应方向移动，速度与半径成反比

### Requirement: Food 豆子类
系统 SHALL 提供 Food 类（继承 Entity）：

- 半径范围 3~8，随机颜色
- 被吞食后标记 `alive = false`，并在随机位置重生

#### Scenario: 豆子生成
- **WHEN** GameScene 调用 spawnFood(count)
- **THEN** 应在场景随机位置生成指定数量的豆子

### Requirement: SkillBall 技能球类
系统 SHALL 提供 SkillBall 类（继承 Entity）：

- 半径 12，带脉冲光效（通过 paint 重写实现）
- 每 5 秒概率生成，最多 8 个
- 存在 30 秒后自动消失
- 携带随机技能类型（Speed/Shield/Grow/Invisible/Magnet）

#### Scenario: 技能球触碰
- **WHEN** 球体触碰技能球
- **THEN** 技能球消失，球体获得对应技能效果

### Requirement: Hazard 危险物类
系统 SHALL 提供 Hazard 类（继承 Entity）：

- 半径 50，三种类型（Bomb/Trap/Poison）
- 每 15 秒概率生成，最多 8 个
- 存在 45 秒后自动消失
- 触碰后施加减益效果（有护盾时免疫）

#### Scenario: 危险物触碰
- **WHEN** 无护盾球体触碰 Hazard
- **THEN** 对应减益效果施加到球体

### Requirement: EjectBall 抛射物类
系统 SHALL 提供 EjectBall 类（继承 Entity）：

- 半径 8，与吐孢者同色
- 初速度 8 像素/帧，逐渐减速
- 存在 20 秒后消失

#### Scenario: 抛射物飞行
- **WHEN** 球体吐出抛射物
- **THEN** 抛射物沿指定方向减速飞行，可被任何球体吞食

### Requirement: SpatialGrid 空间网格
系统 SHALL 提供 SpatialGrid 类：

- 单元格大小 200×200
- 内部存储：`QHash<QPair<int,int>, QVector<Entity*>>`
- 方法：`clear()`, `add(Entity*)`, `getNearby(Entity*) -> QList<Entity*>`

#### Scenario: 空间查询
- **WHEN** 调用 getNearby(entity)
- **THEN** 返回该实体所在单元格及其相邻 8 格内所有实体

### Requirement: AIController AI 控制器
系统 SHALL 提供 AIController 类：

- 静态方法：`updateAI(Ball* ai, const QList<Ball*>& allBalls, const QList<Food*>& foods, qreal dt)`
- 维护每个 AI 的决策状态（AIState 结构体：方向、计时器、目标等）
- 实现三级 AI：1 级（巡逻+吃豆+基础躲避）、2 级（+猎食追逐+分裂逃生）、3 级（+主动猎食分裂）
- 决策流程：威胁检测 → 猎物检测 → 巡逻 → 分裂决策

#### Scenario: AI 行为
- **WHEN** updateAI 每帧被调用
- **THEN** AI 球体表现符合其等级对应的行为

### Requirement: GameScene 游戏场景
系统 SHALL 提供 GameScene 类（继承 QGraphicsScene）：

- 管理实体列表：playerBalls, aiBalls, foods, skillBalls, hazards, ejectBalls
- 核心方法：`updateGame(qreal dt)`，每帧执行：处理输入、更新移动、生命周期管理、吸引力合并、碰撞检测、实体补充、胜负判定
- 生成方法：spawnFood, spawnSkillBall, spawnHazard, spawnAIBall
- 碰撞检测使用 SpatialGrid 优化

#### Scenario: 游戏循环
- **WHEN** QTimer 每 16ms 触发 advanceGame
- **THEN** GameScene::updateGame 完整执行一帧逻辑

### Requirement: GameView 游戏视图 & 状态机
系统 SHALL 提供 GameView 类（继承 QGraphicsView）：

- 持有 GameScene 指针
- 重写 keyPressEvent 处理键盘输入
- 提供 updateCamera() 方法，跟随玩家中心点动态缩放
- 管理游戏状态机：`enum State { Menu, Playing, Paused, GameOver, Victory }`
- 通过 QTimer (16ms) 驱动场景更新

#### Scenario: 状态转换
- **WHEN** 进入 → 按回车 → Playing；Playing → 按 ESC → Paused；Playing → 死亡 → GameOver；Playing → 胜利 → Victory

### Requirement: UIManager UI 管理器
系统 SHALL 提供 UIManager 类：

- 管理 HUD 显示：分数、生存时间、总质量、平均半径、剩余 AI 数量、分裂状态、技能/负面效果/无敌剩余时间
- 管理界面：主菜单、暂停遮罩、游戏结束界面、胜利界面
- 使用 QGraphicsTextItem / QGraphicsRectItem 在场景内绘制

#### Scenario: HUD 更新
- **WHEN** 游戏进行中
- **THEN** HUD 应实时更新显示各项数据

### Requirement: main.cpp 入口
系统 SHALL 提供 main.cpp：

- 创建 QApplication
- 创建 GameView 并启动主循环
- 初始状态设为 Menu

#### Scenario: 游戏启动
- **WHEN** 程序启动
- **THEN** 显示主菜单界面，窗口标题为"Agar.io Clone"
