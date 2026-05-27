# Qt Code Review 报告 + 运行时全流程追踪修复计划

**审查范围**: `D:\code\project\0.0.3\ball_game_qt\src\*` (12 .h + 13 .cpp)
**审查方法**: Lint 确定性扫描 + 6 Agent 并行深度分析 + 逐帧运行时流程追踪

---

## Phase 1: Lint 发现 (22 条)

| 规则 | 文件 | 行号 | 发现 |
|------|------|------|------|
| PAT-12 | AIController.cpp | 13, 43, 56, 82, 93 | 非const range-for 可能触发COW detach |
| HDR-3 | Ball.cpp | 49,74,75,94,122,170,171,205,206,208,281-283 | std::min/max 应加括号防Windows宏 |
| HDR-3 | EjectBall.cpp | 17,18 | 同上 |
| HDR-3 | GameWorld.cpp | 172,221 | 同上 |
| HDR-3 | Hazard.cpp | 70 | 同上 |
| HDR-3 | SkillBall.cpp | 34,39 | 同上 |
| ENM-2 | Utils.h | 28,36 | `SkillType`/`HazardType` 无明确底层类型 |

---

## Phase 2: 6 Agent 深度分析

### Agent 1: QGraphicsScene 契约 (6 项发现，全部 >80 置信度)
- **缺少 `setItemIndexMethod(NoIndex)`**: 每帧数百次 BSP 树操作 → 崩溃根因
- Food/EjectBall/SkillBall/Hazard 使用 `m_x/m_y` 而非 `setPos()`: 正确但不同于 Qt 惯例
- Ball::move() 调用 `update()` (QGraphicsItem::update): 正确（标记重绘）

### Agent 2: 所有权 & 生命周期
- `GameWorld::reset()` 与 `GameScene::clearAllItems()` 时序正确: clearAllItems 先移除场景再 reset 删除
- `removeDeadEntities()` 中 lambda 正向迭代: 正确
- **Grow 技能**: `m_originalRadius` 仅在构造函数设置，从不更新 → 到期时丢失所有吃到的质量

### Agent 3: 线程安全 — **通过，无问题**
纯单线程 Qt 应用，无 QThread/QtConcurrent。

### Agent 4: API & C++ 正确性
- `Utils.h`: `SkillType`/`HazardType` 为 unscoped enum 无底层类型
- `Ball::eject()` 始终返回 `nullptr` (L210) → 功能不完整
- `Ball` 无虚拟析构函数但作为 QGraphicsItem 子类有虚函数 → 应标记 `~Ball() = default;`（Qt 管理）

### Agent 5: 错误处理
- **eject 始终返回 nullptr**: 功能不可用
- **Ball 默认 skill=SKILL_SPEED/debuff=HAZARD_BOMB 且 timer=0**: 哨兵值语义不清

### Agent 6: 性能
- `allBalls` 在 `update()` 和 `checkCollisions()` 中重复构建 (每帧 2 次)
- O(n²) 球-球碰撞检测，无空间分区
- `QTime::currentTime().msec()` 每帧每球在 paint() 中调用 → 频繁系统调用

---

## Phase 3: 运行时全流程追踪 — 逐帧模拟

### 启动流程
```
main() → GameWindow() → setupGame():
  new GameWorld()  → 空状态
  new GameScene()  → sceneRect(0,0,3000,2000)  ← 无 NoIndex!
  new GameView()   → FullViewportUpdate
  new GameLoop()   → QTimer::timeout → tick()

用户按 Enter → startGame():
  clearAllItems()  → 清空场景 trackedItems 集合
  reset()          → delete 旧实体 → 创建 1玩家+20AI+150food+3skill+2hazard
  setGameState(Playing) → m_loop->start()
```

### 第二帧 (t≈16ms) — 典型帧

```
GameLoop::tick():
  dt = 0.016s
  m_world->update(dt, dx, dy):
    [1] 移动玩家球 → Ball::move() → update() (QGraphicsItem::update)
        20 AI球 → updateTimers → AI决策 → 可能split → update()
    [2] 构建 allBalls (第1次)
    [3] AI决策 → split → 新球append到 aiBalls
    [4] checkCollisions():
        └ 重建 allBalls (第2次)
        └ Ball vs Food: 每个球遍历所有食物 → 吃 → setRadius → prepareGeometryChange → BSP树!
        └ Ball vs SkillBall → applySkill(Grow) → setRadius → prepareGeometryChange → BSP树!
        └ Ball vs Hazard → applyDebuff → setRadius → prepareGeometryChange → BSP树!
        └ Ball vs EjectBall → setRadius → prepareGeometryChange → BSP树!
        └ Ball vs Ball O(n²) → 吃 → setRadius → prepareGeometryChange → BSP树!
          └ **ball1被吃后无break → 死ball1继续吃其他球 → 链式反应**
    [5] updateCamera()
    [6-7] spawn food/skill/hazard
  
  m_scene->updateScene():
    syncEntities → addItem(新实体) → BSP树insert!
               → removeItem(死实体) → BSP树remove!
  
  m_world->removeDeadEntities():
    遍历6个vector → delete死实体 → erase
    spawn补充食物/AI → new实体 → 下帧addItem
```

### 崩溃根因分析

**每帧 BSP 树操作统计 (运行5秒后)**:

| 操作 | 来源 | 每帧次数 |
|------|------|----------|
| `prepareGeometryChange` (BSP 更新) | `setRadius()` 在 eat/applySkill/applyDebuff 中 | ~10-50 |
| `addItem` (BSP 插入) | syncEntities 新增实体 | ~0-5 |
| `removeItem` (BSP 删除) | syncEntities 移除死亡实体 | ~0-10 |
| `update` (QGraphicsItem 重绘标记) | Ball::move/SkillBall::update/Hazard::update | ~30-80 |

**5秒后** BSP 树经过 **500-2000** 次增删改操作，内部结构失衡导致:
- BSP 递归深度过深 → **栈溢出** (Stack Overflow)
- BSP 内部断言 `node->depth < 32` 触发 → **SIGABRT 崩溃**

**致命 Bug 链**:
1. `NoIndex` 缺失 → BSP 树承受所有操作
2. `checkCollisions` 中 `ball1.setAlive(false)` 无 `break` → 死球继续吃其他球 → 单帧内更多 `setRadius` → 更多 `prepareGeometryChange` → 加剧 BSP 压力
3. `allBalls` 构建2次 → 浪费CPU但非直接崩溃原因

---

## 修复计划

### 步骤 1: 禁用 BSP 树 — 修复崩溃 🔴

**文件**: `GameScene.cpp`

```cpp
GameScene::GameScene(QObject* parent)
    : QGraphicsScene(parent), m_world(nullptr)
{
    setSceneRect(0, 0, WORLD_WIDTH, WORLD_HEIGHT);
    setItemIndexMethod(QGraphicsScene::NoIndex);  // ← 新增
}
```

### 步骤 2: checkCollisions 添加 break — 防止死球链式反应 🔴

**文件**: `GameWorld.cpp` → `checkCollisions()`

在第 184 行和 195 行 `ball1->setAlive(false)` 之后各添加 `break`:

```cpp
// 合并分支 (L181-184)
} else {
    ball2->setRadius(...);
    ball1->setAlive(false);
    break;  // ← 新增
}

// 吞食分支 (L193-196)
if (ball2->eat(ball1)) {
    ball1->setAlive(false);
    break;  // ← 新增
}
```

### 步骤 3: 修复 Grow 技能吃质量丢失 🟡

**文件**: `Ball.h` — 添加 `m_growOriginalRadius` 成员

```cpp
double m_originalRadius;
double m_growOriginalRadius;  // ← 新增: Grow激活时保存半径
```

**文件**: `Ball.cpp` — `applySkill()` Grow 分支保存当前半径

```cpp
case SKILL_GROW: 
    m_skillTimer = 4; 
    m_growOriginalRadius = m_radius;  // ← 新增: 保存Grow前的半径
    setRadius(m_radius * 1.3); 
    break;
```

**文件**: `Ball.cpp` — `clearSkill()` Grow 分支保留额外质量

```cpp
void Ball::clearSkill() {
    if (m_skill == SKILL_GROW) {
        double expectedRadius = m_growOriginalRadius * 1.3;
        if (m_radius > expectedRadius) {
            setRadius(m_radius / 1.3);
        } else {
            setRadius(m_growOriginalRadius);
        }
    }
    m_skill = SKILL_SPEED;
    m_skillTimer = 0;
}
```

### 步骤 4: 修复 Ball 默认 skill/debuff 哨兵值 🟢

**文件**: `Utils.h` — 添加 None 值

```cpp
enum SkillType : uint8_t {
    SKILL_NONE = 0,
    SKILL_SPEED,
    SKILL_SHIELD,
    SKILL_GROW,
    SKILL_INVISIBLE,
    SKILL_MAGNET
};

enum HazardType : uint8_t {
    HAZARD_NONE = 0,
    HAZARD_BOMB,
    HAZARD_TRAP,
    HAZARD_POISON
};
```

**文件**: `Ball.h` — 修改默认初始值

```cpp
SkillType m_skill;    // constructor: SKILL_NONE
HazardType m_debuff;  // constructor: HAZARD_NONE
```

**文件**: `Ball.cpp` — 构造函数改为 NONE

```cpp
m_skill(SKILL_NONE), m_skillTimer(0),
m_debuff(HAZARD_NONE), m_debuffTimer(0),
```

**文件**: `Ball.cpp` — `clearSkill()` 和 `clearDebuff()` 改为 NONE

```cpp
void Ball::clearSkill() {
    if (m_skill == SKILL_GROW) {
        double expectedRadius = m_growOriginalRadius * 1.3;
        if (m_radius > expectedRadius) {
            setRadius(m_radius / 1.3);
        } else {
            setRadius(m_growOriginalRadius);
        }
    }
    m_skill = SKILL_NONE;  // ← 改为 NONE
    m_skillTimer = 0;
}

void Ball::clearDebuff() {
    m_debuff = HAZARD_NONE;  // ← 改为 NONE
    m_debuffTimer = 0;
}
```

### 步骤 5: 避免 allBalls 重复构建 🟢

**文件**: `GameWorld.cpp` → `update()`

将 update() 中构建的 allBalls 传递给 checkCollisions()，避免重复构建。

**GameWorld.h** — 修改 checkCollisions 签名:
```cpp
void checkCollisions(const std::vector<Ball*>& allBalls);
```

**GameWorld.cpp** — update() 传递 allBalls:
```cpp
checkCollisions(allBalls);  // ← 传递已构建的 allBalls
```

**GameWorld.cpp** — checkCollisions() 不再重建:
```cpp
void GameWorld::checkCollisions(const std::vector<Ball*>& allBalls) {
    // 删除 L98-102 的 allBalls 构建代码
    for (Ball* ball : allBalls) {
        ...
    }
}
```

---

## 修改文件清单

| 文件 | 修改内容 |
|------|----------|
| `GameScene.cpp` | **步骤1**: `setItemIndexMethod(NoIndex)` 禁用 BSP 树 |
| `GameWorld.h` | **步骤5**: `checkCollisions` 接受 `const std::vector<Ball*>&` |
| `GameWorld.cpp` | **步骤2**: 2处 break; **步骤5**: 传递 allBalls 避免重复构建 |
| `Ball.h` | **步骤3**: 添加 `m_growOriginalRadius`; **步骤4**: 默认值改 NONE |
| `Ball.cpp` | **步骤3**: Grow 质量保留; **步骤4**: 构造/清除改为 NONE |
| `Utils.h` | **步骤4**: enum 添加 `_NONE=0` + `uint8_t` 底层类型 |

---

## 预期效果

1. **不再崩溃** — NoIndex 消除 BSP 树所有操作，根除栈溢出
2. **死球不再链式反应** — break 阻止 dead ball1 继续吃其他球
3. **Grow 技能正确** — 期间吃到的东西不再丢失
4. **代码语义清晰** — SKILL_NONE/HAZARD_NONE 取代 SPEED/BOMB 哨兵值
5. **allBalls 只构建一次** — 每帧省去 O(n) 重建