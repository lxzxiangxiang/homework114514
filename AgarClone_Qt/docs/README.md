# Agar.io Clone — 球球大作战完整复刻版

C++17 + Qt 6.11 (MinGW 64-bit) | CMake | QGraphicsView

---

## 1. 项目概览

```
AgarClone_Qt/
├── CMakeLists.txt          — 构建配置（C++17, Qt6 Core+Widgets）
├── main.cpp                — 入口，创建 QApplication + GameView
├── Constants.h             — 所有游戏常量
├── Entity.h                — 实体抽象基类（QGraphicsEllipseItem）
├── Ball.h / .cpp           — 玩家/AI 球体
├── Food.h / .cpp           — 豆子（可吞食）
├── SkillBall.h / .cpp      — 技能球（5种技能）
├── Hazard.h / .cpp         — 危险物（3种减益）
├── EjectBall.h / .cpp      — 抛射物（吐孢）
├── SpatialGrid.h / .cpp    — 空间网格碰撞优化
├── AIController.h / .cpp   — AI 控制器（3级决策）
├── GameScene.h / .cpp      — 游戏场景引擎
├── GameView.h / .cpp       — 视图窗口 + 状态机 + 摄像机
├── UIManager.h / .cpp      — 菜单/暂停/结束/胜利/HUD
├── docs/                   — 中文文档
└── build/                  — 编译产物（含 AgarClone.exe）
```

---

## 2. 编译与运行

```powershell
# 配置
cmake -S AgarClone_Qt -B AgarClone_Qt/build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=D:/anzhuangweizhi/qt/6.11.0/mingw_64

# 编译
cmake --build AgarClone_Qt/build -j4

# 运行
AgarClone_Qt/build/AgarClone.exe
```

---

## 3. 完整运行时调用链

### 3.1 启动 → 主菜单

```
main()
  └─ QApplication a(argc, argv)           // Qt 应用初始化
  └─ GameView view                        // 构造函数：
       ├─ m_gameScene = new GameScene()   // 创建场景(5000×5000, 100Food, 5AI, 1Player)
       ├─ setScene(m_gameScene)           // 绑定场景到视图
       ├─ m_uiManager = new UIManager(scene, view)  // 创建 UI
       │    ├─ createHUDItems()           // 9个 HUD 文本项(白色, z=100, 默认隐藏)
       │    └─ createMenuItems()          // 半透明黑背景(z=200) + 标题 + 按键提示
       ├─ setWindowTitle("Agar.io Clone")
       ├─ resize(1280, 720)
       ├─ setBackgroundBrush(Qt::black)   // 黑色背景
       ├─ 禁用抗锯齿、滚动条              // 性能优化
       ├─ m_gameTimer = new QTimer(16ms)  // 60 FPS 主循环
       │    └─ connect(timeout → advanceGame)
       └─ returnToMenu()                  // 初始状态
            ├─ m_state = Menu
            ├─ m_gameTimer->stop()
            ├─ resetTransform()           // 摄像机归位
            ├─ centerOn(640, 360)         // 居中窗口坐标
            └─ m_uiManager->showMenu()    // 显示菜单背景+标题+提示

  └─ view.show()
  └─ return a.exec()                      // 进入 Qt 事件循环
```

屏幕状态：**黑色背景 + 半透明菜单 + "Agar.io Clone" 标题 + 按键提示**

---

### 3.2 开始游戏（按 Enter）

```
keyPressEvent(Qt::Key_Enter)
  └─ m_state == Menu
       └─ startGame()
            ├─ setScene(nullptr)           // 解除旧场景
            ├─ delete m_gameScene          // 销毁（所有实体自动释放）
            ├─ delete m_uiManager
            ├─ m_gameScene = new GameScene()  // 重建场景
            │    ├─ setSceneRect(0, 0, 5000, 5000)
            │    ├─ spawnFood(100)            // 随机地图位置 100 个豆子
            │    ├─ spawnAIBall() × 5         // 随机半径15~60, 颜色, 等级1~3, aiId递增
            │    └─ new Ball(15, 红色, isPlayer=true)
            │         ├─ setPos(2500, 2500)   // 地图中心
            │         └─ playerBalls.append()
            ├─ setScene(m_gameScene)
            ├─ m_uiManager = new UIManager(...)
            ├─ m_keysPressed.clear()
            ├─ m_state = Playing
            └─ m_gameTimer->start()         // 启动 16ms 定时器
```

---

### 3.3 主游戏循环（每 16ms，~60 FPS）

```
QTimer::timeout
  └─ advanceGame()
       ├─ if (m_state != Playing) return    // 非游戏状态直接返回
       │
       ├─ ① processPlayerInput()
       │    读取 m_keysPressed → WASD组合归一化方向 → playerInputDirection
       │    Space → wantSplit=true, E → wantEject=true
       │
       ├─ ② m_gameScene->updateGame(0.016)  ——— 详见 3.4
       │
       ├─ ③ 胜负判定
       │    totalRadius = Σ(存活playerBalls.radius)
       │    if (playerBalls空 || totalRadius≤0) → gameOver()
       │    if (totalRadius ≥ 2000) → victory()
       │
       ├─ ④ updateCamera()  ——— 详见 3.5
       │
       └─ ⑤ 聚合HUD → m_uiManager->updateHUD(score, time, mass, radius, ...)
            设置9行文本 + 动态 viewport 定位(用 mapToScene 固定在左上角)
```

---

### 3.4 updateGame(dt) 内部 13 步

```
GameScene::updateGame(0.016)
│
├─ 1. 移动玩家球体
│    for ball ∈ playerBalls:
│        ball->move(playerInputDirection, dt)
│        ┌─────────────────────────────────────┐
│        │ Ball::move(dx, dy, dt):             │
│        │  speed = 300 × √(10 / radius)       │  ← 半径越大越慢
│        │  if Trap: speed ×= 0.5              │
│        │  vx = dx × speed                    │
│        │  vy = dy × speed                    │
│        │  setPos(pos + (vx,vy)×dt)           │
│        │  clamp pos 到 [0,5000]              │
│        └─────────────────────────────────────┘
│
├─ 2. 分裂（Space 触发的瞬时行为）
│    if wantSplit:
│        for ball ∈ playerBalls(快照):
│            newBall = ball->split(direction)
│            ┌─────────────────────────────────────┐
│            │ Ball::split(dir):                   │
│            │  if(radius<18 || splitTimer>0) →null│
│            │  newRadius = radius / √2            │
│            │  setRadius(newRadius)   // 自缩小  │
│            │  new Ball(newR, 同色, isPlayer, lv) │
│            │  newBall.aiId = aiId                │
│            │  newBall.splitTimer = 1.5s          │
│            │  newBall.mergeTimer = 1.5s          │
│            │  newBall.invincibleTimer = 3s       │
│            │  newBall.setPos(自身+dir×(r+r×0.1)) │
│            │  return newBall                     │
│            └─────────────────────────────────────┘
│            if newBall: addPlayerBall(newBall)
│        wantSplit = false
│
├─ 3. 吐孢（E 触发的瞬时行为）
│    if wantEject:
│        for ball ∈ playerBalls:
│            eb = ball->eject()
│            ┌─────────────────────────────────────┐
│            │ Ball::eject():                      │
│            │  if(radius<25) →null                │
│            │  setRadius(radius - 3)              │
│            │  dir = (lastDx, lastDy)             │
│            │  return new EjectBall(pos,色,dir)   │
│            │    → vx=dx×8, vy=dy×8, lifetime=20  │
│            └─────────────────────────────────────┘
│            if eb: addEjectBall(eb)
│        wantEject = false
│
├─ 4. allBalls = playerBalls + aiBalls    // 供后续使用
│
├─ 5. AI 更新
│    for ai ∈ aiBalls:
│        ai->pendingSplitBall = nullptr
│        AIController::updateAI(ai, allBalls, foods, dt)  ——— 详见 3.6
│        if ai->pendingSplitBall:
│            addItem → aiBalls.append → ai指针清空
│
├─ 6. 抛射物更新
│    for eb ∈ ejectBalls:  eb->update(dt)
│        ┌─────────────────────────────────────┐
│        │ EjectBall::update(dt):              │
│        │  lifetime -= dt                     │
│        │  setPos(pos + (vx,vy)×dt)           │
│        │  vx×=0.98, vy×=0.98   // 逐渐减速  │
│        │  if lifetime≤0 → m_alive=false      │
│        └─────────────────────────────────────┘
│
├─ 7. 技能球生成
│    m_skillSpawnTimer -= dt
│    if timer≤0 且 count<MAX(20):
│        spawnSkillBall()         // 随机位置, 5种技能随机, 半径12, lifetime=30s
│        timer = 5~10s 随机
│
├─ 8. 危险物生成
│    m_hazardSpawnTimer -= dt
│    if timer≤0 且 count<MAX(10):
│        spawnHazard()            // 随机位置, Bomb/Trap/Poison, 半径50, lifetime=45s
│        timer = 8~15s 随机
│
├─ 9. 同源吸引力  ——— 详见 3.8
│    applyAttraction(dt)
│
├─ 10. 碰撞检测   ——— 详见 3.7
│     checkCollisions()
│
├─ 11. 移除死亡实体
│     removeDeadEntities()
│      lambda 逆向遍历 player/ai/food/skill/hazard/eject 6个列表:
│        if !alive → removeItem + delete + removeAt
│
├─ 12. 补豆子
│     while foods.size() < MAX_FOOD_COUNT(500):
│         spawnFood(1)
│
└─ 13. survivalTime += dt
```

---

### 3.5 动态摄像机

```
GameView::updateCamera()
│
├─ 计算质量加权中心 (半径作为权重)
│   for ball ∈ playerBalls:
│       cx += ball.x × ball.radius
│       cy += ball.y × ball.radius
│       totalWeight += ball.radius
│   cx/=totalWeight, cy/=totalWeight
│
├─ 平滑插值 (lerp=0.1)
│   currentCenter = mapToScene(viewport中心)
│   smoothCenter = currentCenter + (target - currentCenter)×0.1
│
├─ 动态缩放
│   maxR = max(存活playerBalls.radius)
│   zoom = 1.5 × (10 / maxR)
│   zoom = clamp(zoom, 0.5, 1.5)
│   球越小→zoom越大→视野越宽
│
└─ resetTransform() → scale(zoom) → centerOn(smooth)
```

---

### 3.6 AI 决策（每帧调用）

```
AIController::updateAI(ai, allBalls, foods, dt)
│
├─ if(!ai || !ai->isAlive) return
├─ 获取/创建 AIState state = s_states[ai]
├─ state.decisionTimer -= dt
│
├─ ═══ 决策到期 ═══ (Level1:0.4s, Level2:0.2s, Level3:0.1s)
│   if state.decisionTimer ≤ 0:
│   │
│   ├─ 重置计时器 (按 ai->aiLevel)
│   │
│   ├─ ① 威胁检测
│   │   for other ∈ allBalls:
│   │       if other.radius > self.radius×1.1 且 dist < self.radius×6:
│   │           hasThreat = true
│   │           fleeDirection -= (other.pos - self.pos)  // 叠加逃离向量
│   │
│   ├─ ② 猎物检测
│   │   for other ∈ allBalls:
│   │       if other.radius < self.radius×0.9:
│   │           score = other.radius / (dist+1)          // 价值评分
│   │           追最高分
│   │
│   ├─ ③ 巡逻
│   │   if !hasThreat && !hasPrey:
│   │       if 到达巡逻点: 随机[0,5000]新目标
│   │
│   └─ ④ 分裂决策 (Level≥2)
│       if radius≥18 && splitTimer≤0:
│           if hasThreat && threatDist<self×3:
│               ai->pendingSplitBall = ai->split(逃离方向)
│           elif hasPrey && preyDist<self×(4 or 5) && self>prey×1.5:
│               ai->pendingSplitBall = ai->split(猎物方向)
│
├─ ═══ 平滑转向 ═══
│   turnSpeed = (Level1:2, Level2:3, Level3:5) rad/s
│   angleCurrent = atan2(currentDir)    // 当前方向
│   angleTarget  = atan2(targetDir)     // 目标方向
│   angleDiff = clamp(angleTarget-angleCurrent, -turnSpeed×dt, +turnSpeed×dt)
│   currentDir = (cos(newAngle), sin(newAngle))
│
└─ ai->move(currentDir, dt)          // 执行移动
```

**三级 AI 差异：**

| 等级 | 决策间隔 | 反应时间 | 转向速度 | 分裂 |
|------|----------|----------|----------|:----:|
| 1 | 0.4s | 0.5s | 2.0 rad/s | 无 |
| 2 | 0.2s | 0.3s | 3.0 rad/s | 逃生+猎食 |
| 3 | 0.1s | 0.15s | 5.0 rad/s | 激进（阈值更宽） |

---

### 3.7 碰撞检测（5 种，逐球遍历）

```
GameScene::checkCollisions()
│
├─ 构建空间网格
│   m_spatialGrid.clear()
│   for ball ∈ allBalls: grid.add(ball)
│
├─ ═══ 1. Ball ↔ Food ═══
│   for ball ∈ allBalls:
│       for food ∈ foods:
│           dist² ≤ (rB+rF)² && rB > rF×1.1
│           → ball->eat(food) → score += 1(玩家)
│
├─ ═══ 2. Ball ↔ SkillBall ═══
│   for ball ∈ allBalls:
│       for sb ∈ skillBalls:
│           dist² ≤ (rB+rS)²
│           → ball->applySkill(sb.type)  // 设置技能+计时器
│           → sb->onEaten()              // 标记死亡
│
├─ ═══ 3. Ball ↔ Hazard ═══
│   for ball ∈ allBalls:
│       if ball.hasShield(): skip        // 护盾免疫
│       for haz ∈ hazards:
│           dist² ≤ (rB+rH)²
│           → ball->applyDebuff(haz.type)
│           → haz->onEaten()
│
│   减益效果：
│   ┌────────┬──────────────────────────────────────┐
│   │ Bomb   │ setRadius(r × 0.85)  瞬时           │
│   │ Trap   │ debuff=Trap, timer=3s, move()中×0.5 │
│   │ Poison │ debuff=Poison, timer=4s, update()中  │
│   │        │ 每帧 radius -= 2×dt                 │
│   └────────┴──────────────────────────────────────┘
│
├─ ═══ 4. Ball ↔ EjectBall ═══
│   for ball ∈ allBalls:
│       for eb ∈ ejectBalls:
│           dist² ≤ (rB+rE)² && rB > rE×1.1
│           → ball->eat(eb) → score += 0.5(玩家)
│
└─ ═══ 5. Ball ↔ Ball（空间网格查询） ═══
    for ball1 ∈ allBalls:
        nearby = grid.getNearby(ball1)   // 3×3邻域
        for e ∈ nearby:
            if e不是Ball: continue
            ball2 = (Ball*)e
            if ball1 > ball2: continue    // 避免重复
            dist² ≤ (r1+r2)² ?
            │
            ├─ 同源(同isPlayer 或 同aiId>0) && mergeTimer≤0:
            │   大球->eat(小球)            // 合并
            │
            └─ 非同源 && r大>r小×1.1 && 小无护盾:
                大球->eat(小球)
                玩家: score += 被吃球radius×0.5
```

**吞食公共公式** (`Ball::eat`):
```
newRadius = √(self.radius² + target.radius²)
setRadius(newRadius)
target->onEaten(this)   → target.m_alive = false
```

---

### 3.8 同源吸引力

```
GameScene::applyAttraction(dt)
│
└─ 遍历同 owner 球体对:
      if !sameOwner: continue
      if splitTimer>0: continue    // 分裂冷却中不吸引
      dist = |b2.pos - b1.pos|
      if dist < 1e-6: continue
      │
      attraction = 50 + 0.001×dist² + 500/(b1.mergeTimer + b2.mergeTimer + 1)
      │            常数项    距离项(越远越大)    时间项(刚分裂很强, 随时间减弱)
      │
      moveStep = attraction×dt/2
      b1.setPos(b1 + normalized方向 × moveStep)   // 相向移动
      b2.setPos(b2 - normalized方向 × moveStep)
      │
      └─ 靠近后由 checkCollisions 处理合并
```

---

### 3.9 暂停 / 继续

```
按 ESC (Playing状态)
  → keyPressEvent(Esc)
    → pauseGame()
        m_state = Paused
        m_gameTimer->stop()
        m_uiManager->showPause()
        ┌────────────────────────┐
        │ 惰性创建(仅首次):      │
        │ 半透明遮罩(z=250)     │
        │ "PAUSED" (36号粗体)   │
        │ "ESC继续 / M回菜单"   │
        └────────────────────────┘

按 ESC (Paused状态)
  → keyPressEvent(Esc)
    → resumeGame()
        m_state = Playing
        m_gameTimer->start()
        m_uiManager->hideAll()    // 隐藏暂停遮罩
```

---

### 3.10 游戏结束（失败）

```
advanceGame() 中检测:
   playerBalls.isEmpty() || totalRadius ≤ 0

→ gameOver()
     m_state = GameOver
     m_gameTimer->stop()
     resetTransform()          // 摄像机归位
     centerOn(640, 360)
     m_uiManager->showGameOver(score, survivalTime)
     ┌──────────────────────────────┐
     │ 删除旧 overlay(如有)→重建   │
     │ 红色 "GAME OVER" (42号粗体) │
     │ Score: 42                    │
     │ Survival Time: 01:30         │
     │ Enter 重新开始 / M 回菜单    │
     └──────────────────────────────┘

按 Enter → startGame() 重新开始
按 M     → returnToMenu() 回主菜单
```

---

### 3.11 胜利

```
advanceGame() 中检测:
   totalRadius ≥ 2000

→ victory()
     m_state = Victory
     m_gameTimer->stop()
     resetTransform()
     centerOn(640, 360)
     m_uiManager->showVictory(score, survivalTime)
     ┌──────────────────────────────┐
     │ 绿色 "VICTORY!" (42号粗体)  │
     │ Score + Time                 │
     │ Enter 重新开始 / M 回菜单    │
     └──────────────────────────────┘
```

---

### 3.12 返回主菜单

```
按 M (Paused/GameOver/Victory 状态)
  → keyPressEvent(M)
    → returnToMenu()
        m_state = Menu
        m_gameTimer->stop()
        resetTransform()
        centerOn(640, 360)
        m_uiManager->showMenu()
            → 显示菜单背景+标题+提示
            → 隐藏暂停/结束/胜利 overlay
            → 隐藏 HUD
```

---

## 4. 关键数据流

```
键盘按下
  └─ keyPressEvent() → m_keysPressed.insert(key)
       │
       ▼
  processPlayerInput()                   [GameView, 每帧]
       ├─ 读 m_keysPressed → 归一化方向
       ├─ Space → wantSplit
       └─ E → wantEject
       │
       ▼
  GameScene::updateGame()                [每帧 13 步]
       ├─ playerBalls → move()           [Ball, 速度公式]
       ├─ wantSplit → split()            [Ball, 质量分裂]
       ├─ wantEject → eject()            [Ball, 吐孢]
       ├─ aiBalls → AIController         [AI, 决策+转向]
       ├─ ejectBalls → update()          [EjectBall, 飞行]
       ├─ applyAttraction()              [同源吸引]
       ├─ checkCollisions()              [5种碰撞]
       └─ removeDeadEntities()           [清理]
       │
       ▼
  advanceGame() 继续
       ├─ 胜负判定
       ├─ updateCamera()                 [质量加权+zoom]
       └─ updateHUD()                    [UIManager, 动态定位]
```

---

## 5. 按键表

| 按键 | 状态 | 功能 |
|------|------|------|
| **Enter** | Menu | 开始游戏 |
| **W/A/S/D** | Playing | 上/左/下/右移动 |
| **空格** | Playing | 分裂（半径≥18） |
| **E** | Playing | 吐孢（半径≥25） |
| **ESC** | Playing | 暂停 |
| **ESC** | Paused | 继续 |
| **M** | Paused/GameOver/Victory | 返回主菜单 |
| **Enter** | GameOver/Victory | 重新开始 |
| 关闭窗口 | 任意 | 退出 |

---

## 6. 关键常量速查

| 常量 | 值 | 说明 |
|------|------|------|
| WINDOW_SIZE | 1280×720 | 窗口尺寸 |
| MAP_SIZE | 5000×5000 | 地图尺寸 |
| FRAME_INTERVAL | 16ms | ~60FPS |
| BASE_SPEED | 300 | 基础速度 |
| EAT_RATIO | 1.1 | 吞食半径比 |
| SPLIT_THRESHOLD | 18 | 可分裂最小半径 |
| EJECT_THRESHOLD | 25 | 可吐孢最小半径 |
| VICTORY_RADIUS | 2000 | 胜利条件 |
| MAX_FOOD | 500 | 豆子上限 |
| MAX_AI | 5(初始) | AI数量 |
| CELL_SIZE | 200×200 | 空间网格单元格 |
| CAMERA_ZOOM | [0.5, 1.5] | 摄像机缩放范围 |
