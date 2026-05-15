# Bug 修复计划

## Bug 诊断结果

通过全量代码审查，识别出以下 **5 个 Bug**，按致命程度排列：

---

### Bug 1: 🔴 致命 — 玩家球体从未创建

**位置**: [GameScene.cpp:L15-L27](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L15-L27)

**原因**: `GameScene` 构造函数只创建了 100 个 Food 和 5 个 AI Ball，**没有创建玩家球体**。`playerBalls` 列表始终为空。

**后果**: 按 Enter 开始游戏后，`advanceGame()` 第一帧即检测到 `playerBalls.isEmpty()`→`gameOver()`，屏幕闪一下 "GAME OVER"。游戏完全不可玩。

**修复**: 在构造函数中创建玩家球体并添加到 `playerBalls`：
```cpp
auto* player = new Ball(15.0, QColor(255, 80, 80), true, 0);
player->setPos(MAP_WIDTH / 2, MAP_HEIGHT / 2);
addItem(player);
playerBalls.append(player);
```

---

### Bug 2: 🟡 中等 — AI 球体 aiId 全部为 0，同源判定失效

**位置**: [Ball.cpp:L12](file:///d:/code/project/0.0.1/AgarClone_Qt/Ball.cpp#L12) (`aiId(0)`) + [GameScene.cpp:L168](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L168) (`spawnAIBall` 不设 aiId)

**原因**: `Ball` 构造函数默认 `aiId=0`，`spawnAIBall()` 从不设置 `aiId`。同源判定：
```cpp
// applyAttraction + checkCollisions 中的 sameOwner:
(b1->aiId > 0 && b1->aiId == b2->aiId)
```
`aiId > 0` 永远为假 → 所有 AI 球体被视为"不同源"。

**后果**:
- `applyAttraction()` 中 AI 球体之间不产生吸引力
- `checkCollisions()` 中 AI 同组球体不自动合并
- AI split 后产生的子球永远无法合并回母球

**修复**: 
1. `GameScene` 添加 `int m_nextAiId = 1` 成员
2. `spawnAIBall()` 中设置 `ai->aiId = m_nextAiId++`
3. `Ball::split()` 中保留 `newBall->aiId = aiId`（已正确）

---

### Bug 3: 🟡 中等 — BASE_SPEED=5 导致移动极慢

**位置**: [Ball.cpp:L21](file:///d:/code/project/0.0.1/AgarClone_Qt/Ball.cpp#L21)

**原因**: 
```
speed = BASE_SPEED(5) × sqrt(MIN_RADIUS(10) / radius)
位移 = speed × dt(0.016)
```
半径 10 的球：每秒移动 `5 × sqrt(1) × 0.016 × 60 = 4.8 像素/秒`  
在 5000×5000 地图上几乎不可感知。

**修复**: 将 `BASE_SPEED` 从 5 改为 300：
```cpp
// Constants.h
inline constexpr float BASE_SPEED = 300.0f;
```
半径 10 的球：`300 × 0.016 = 4.8 像素/帧 ≈ 288 像素/秒`，可正常游玩。

---

### Bug 4: 🟢 轻微 — AI 巡逻目标范围错误

**位置**: [AIController.cpp:L117-L121](file:///d:/code/project/0.0.1/AgarClone_Qt/AIController.cpp#L117-L121)

**原因**: 巡逻目标生成在 `[-MAP_WIDTH/2, MAP_WIDTH/2]` = `[-2500, 2500]`，但场景和球体位于 `[0, 5000]`。

**后果**: AI 可能尝试向负坐标移动，被 `move()` 中边界 clamp 卡住。

**修复**: 巡逻目标改为 `[0, MAP_WIDTH]`：
```cpp
qreal rx = QRandomGenerator::global()->generateDouble() * GameConstants::MAP_WIDTH;
qreal ry = QRandomGenerator::global()->generateDouble() * GameConstants::MAP_HEIGHT;
```

---

### Bug 5: 🟢 轻微 — EjectBall constructor 颜色参数类型问题

**位置**: [Ball.cpp:L74](file:///d:/code/project/0.0.1/AgarClone_Qt/Ball.cpp#L74) 

**原因**: `new EjectBall(pos(), m_color, dir.x(), dir.y())` — `m_color` 是 `QColor`，但 `EjectBall` 构造函数接受 `QColor`，这没问题。编译通过说明OK。这条可能是误判，去掉。

---

### Bug 5 (重新编号): 🟢 轻微 — 同源合并时被吃球仍有 BoundingRect 问题

**位置**: [GameScene.cpp:L293-L296](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L293-L296)

**原因**: 同源合并时 `ball1->eat(ball2)` 内部调用 `target->onEaten(this)` 将 ball2 标记为死亡。然后在一帧内，外层循环 `ball2` 仍被 `spatialGrid.getNearby` 返回（已标记死亡），但内层检测 `!ball2->isAlive()` 能过滤。同一帧内靠 `ball1 > ball2` 的指针比较避免重复处理。但有一处问题：如果 `ball2->radius() > ball1->radius()`，则走 `ball2->eat(ball1)` 分支，此时 ball1 被标死亡但外层 for 循环内的 `if (!ball1->isAlive()) continue` 已检查过，本帧后续遍历不再处理 ball1。**这种情况下合并没问题，但 ball1 被 ball2 eat 后 ball1 已死，ball2 存活。同一帧后其他遍历会跳过已死的 ball1。**这条可能也不算 bug。

让我再重新核实一下...

实际上这里有个潜在问题。当 `ball2->eat(ball1)` 被调用后，ball1 被设为 `m_alive = false`。但外层的 `for (Ball* ball1 : allBalls)` 循环已经获取了 ball1 的引用，后续遍历中的 `if (!ball1->isAlive()) continue` 会跳过。这个其实没问题。

好，就确认 4 个 bug（去掉原来拿不准的）。

---

## 修复步骤

### Step 1: 修复 Bug 1 — GameScene 创建玩家球体
- 在 `GameScene` 构造函数末尾添加玩家球体创建代码
- 玩家球体：半径 15，红色，地图中心位置

### Step 2: 修复 Bug 2 — AI 球体 aiId 分配
- 在 `GameScene.h` 添加 `int m_nextAiId = 1`
- 在 `spawnAIBall()` 中设置 `ai->aiId = m_nextAiId++`

### Step 3: 修复 Bug 3 — BASE_SPEED 提速
- 修改 `Constants.h` 中 `BASE_SPEED` 从 5 改为 300

### Step 4: 修复 Bug 4 — AI 巡逻目标范围
- 修改 `AIController.cpp` 中巡逻目标生成范围

### Step 5: 编译验证
- `cmake --build` 通过
