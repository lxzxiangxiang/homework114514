# 核心玩法 TODO 实现计划

## 目标
实现检查报告中识别的 7 项核心玩法 TODO，使游戏可玩。

## 变更范围
- 修改文件：`Ball.cpp`、`GameScene.cpp`、`AIController.cpp`、`Entity.h`、`Ball.h`
- 不新建文件
- 不涉及 UI、摄像机、网络等模块

---

## 实施步骤

### Step 1: Entity 基类扩展
**前置依赖**：无

- [ ] 在 `Entity.h` 中添加 `void setRadius(qreal r)` 方法，同时更新 `setRect()` 使 QGraphicsEllipseItem 的包围盒同步
- [ ] 在 `Entity.h` 中添加 `void setColor(QColor c)` 方法，同步更新 `setBrush()`

> **原因**：Ball::eat() 吞食后需要增长半径，Ball::split() 需要缩小自身半径，都需要修改 Entity 的 m_radius。

### Step 2: Ball::eat() — 吞食实体
**前置依赖**：Step 1

根据文档需求 2.1 中的公式：
- **质量** = π × 半径²，吞食后新质量 = 自身质量 + 目标质量
- **最终半径** = √(自身质量/π + 目标质量/π) = √(自身半径² + 目标半径²)
- 目标调用 `onEaten(this)`
- 如果目标为 Food，不增加分数（Food 无分数或固定 1 分，由 GameScene 处理）

```cpp
void Ball::eat(Entity* target) {
    // 质量合并：新面积 = 旧面积 + 目标面积
    qreal newRadius = std::sqrt(m_radius * m_radius + target->radius() * target->radius());
    setRadius(newRadius);
    target->onEaten(this);
}
```

### Step 3: Ball::split() — 分裂球体
**前置依赖**：Step 1

- 当前球体半径缩小为：`m_radius / √2`
- 创建新 Ball：`new Ball(newRadius, m_color, isPlayer, aiLevel)`
- 新球初始位置 = 当前球位置 + 方向 × (新半径 + 原半径×0.1)
- 新球初始速度 = 方向 × 200
- 设置 splitTimer = 1.5s（防止立即合并）
- 调用 GameScene 的 `addBall(Ball*)` 注册到场景
  - 需要在 GameScene.h 中添加 `void addBall(Ball* ball)` 方法

```cpp
void Ball::split(QPointF direction) {
    qreal newRadius = m_radius / std::sqrt(2.0);
    setRadius(newRadius);
    // 新球创建和注册在 GameScene 中协调
}
```

> **架构注意**：split 需要创建新球并注册到场景，因此 GameScene 需要暴露 `addBall(Ball*)` 方法。

### Step 4: Ball::eject() — 吐孢
**前置依赖**：Step 1

- 自身半径减少 3：`setRadius(m_radius - 3)`
- 创建 EjectBall 在自身位置，方向 = (lastDx, lastDy)
- GameScene 需要暴露 `addEjectBall(EjectBall*)` 方法

```cpp
void Ball::eject() {
    setRadius(m_radius - 3);
    // 创建 EjectBall 在 GameScene 中协调
}
```

### Step 5: Ball::applyDebuff() — 应用负面效果
**前置依赖**：无

| 减益类型 | 效果 | 实现 |
|----------|------|------|
| Bomb | 半径瞬间 ×0.85 | `setRadius(m_radius * 0.85f)` |
| Trap | 速度 ×0.5，持续 3s | 设置 debuff/DebuffType::Trap，debuffTimer=3 |
| Poison | 每秒 -2 半径，持续 4s | 设置 debuff/DebuffType::Poison，debuffTimer=4 |

- Ball::update() 中已有 debuffTimer 递减逻辑，需补充 Poison 的实际半径递减：
  - 在 `update()` 中：若 `debuff == Poison && debuffTimer > 0`，每帧 `setRadius(m_radius - Poison::RADIUS_PER_SEC * dt)`

### Step 6: GameScene::checkCollisions() — 碰撞检测
**前置依赖**：Step 1~5

使用 SpatialGrid 优化，按以下顺序处理 5 种碰撞：

#### 6.1 Ball ↔ Food 碰撞
```
for each ball ∈ allBalls:
    for each food ∈ spatialGrid.getNearby(ball):
        若 ball.radius > food.radius × EAT_RATIO:
            ball.eat(food)
            若 ball.isPlayer: score += 1
```

#### 6.2 Ball ↔ SkillBall 碰撞
```
for each ball ∈ allBalls:
    for each sb ∈ skillBalls:
        若碰撞(ball, sb):
            ball.applySkill(sb.skillType)
            sb.onEaten(ball)
```

#### 6.3 Ball ↔ Hazard 碰撞
```
for each ball ∈ allBalls:
    若 ball.hasShield(): continue
    for each hazard ∈ hazards:
        若碰撞(ball, hazard):
            ball.applyDebuff(hazard.hazardType)
            hazard.onEaten(ball)
```

#### 6.4 Ball ↔ EjectBall 碰撞
```
for each ball ∈ allBalls:
    for each eb ∈ ejectBalls:
        若 ball.radius > eb.radius × EAT_RATIO:
            ball.eat(eb)
            若 ball.isPlayer: score += 0.5
```

#### 6.5 Ball ↔ Ball 碰撞（同源合并 + 吞食）
```
for each ball1 ∈ allBalls:
    for each ball2 ∈ spatialGrid.getNearby(ball1):
        若 ball1 == ball2: continue
        若 sameOwner(ball1, ball2) 且 距离 < radius1+radius2 且 mergeTimer≤0:
            merge(ball1, ball2)  ← 见 Step 7
        否则若 ball1.radius > ball2.radius × EAT_RATIO 且 !ball2.hasShield():
            ball1.eat(ball2)
            若 ball1.isPlayer: score += ball2.radius × 0.5
```

- 碰撞判定使用平方距离比较：`distSq ≤ (r1 + r2)²`
- sameOwner 判定：同 `isPlayer` 玩家球体之间、同 `aiId` AI 球体之间
- SpatialGrid 构建：每帧 `m_spatialGrid.clear()` → 遍历 allBalls `m_spatialGrid.add(ball)`

### Step 7: GameScene::applyAttraction() — 同源球体吸引力
**前置依赖**：Step 1

按文档 2.5 公式：
- **同源判定**：同玩家（isPlayer==true）或同 AI 组（aiId相同且>0）
- **吸引力** = 常数引力(50) + 距离引力(0.001 × dist²) + 时间初始引力
- **合并条件**：两同源球体圆心距离 < 半径之和
- **合并公式**：新半径 = √(r1² + r2²)

```cpp
void GameScene::applyAttraction(qreal dt) {
    for each pair of same-owner balls:
        若 splitTimer > 0 跳过（分裂冷却）
        距离 = |pos1 - pos2|
        吸引力 = 50 + 0.001 × dist² + 500 / (mergeTimer + 1)
        方向 = normalized(pos2 - pos1)
        移动 = 方向 × 吸引力 × dt
        ball1.moveTowards(ball2, 移动)  // 各移动一半
        若 距离 < r1 + r2: 合并（大球吃小球）
}
```

> 简化方案：在两球上分别施加相向力，各移动吸引力×dt/2。

### Step 8: AI 分裂决策完善
**前置依赖**：Step 3, Step 6

在 `AIController::updateAI()` 的分裂决策 placeholder 中：

- **分裂逃生**（Level ≥ 2）：若有威胁且威胁距离 < 自身半径×3，朝逃离方向分裂
- **分裂猎食**（Level ≥ 2）：若有猎物且猎物距离 < 自身半径×4 且自身半径 > 猎物半径×1.5，朝猎物方向分裂
- Level 3 更激进：分裂阈值更宽松
- 添加 `ai->splitTimer` 冷却（1.5s）防止连续分裂

### Step 9: GameScene 补充方法
**前置依赖**：Step 3, Step 4

- 在 `GameScene.h` 中添加 `void addPlayerBall(Ball* ball)` — 用于 split 后注册新球
- 在 `GameScene.h` 中添加 `void addEjectBall(EjectBall* eb)` — 用于 eject 后注册孢子
- split 和 eject 需要通过某种方式回调 GameScene，方案：
  - 方案 A（推荐）：在 Ball 构造时传入 GameScene* 指针
  - 方案 B：split/eject 返回值，由 GameScene::updateGame 处理
  - 采用方案 B 更简洁：split 返回新球指针（或 nullptr），eject 返回新 EjectBall 指针，GameScene 在 updateGame 中处理

> **修改 Ball.h**：`Ball* split(QPointF direction)` 替代 `void split(...)`，返回新球（nullptr 表示不满足条件）
> **修改 Ball.h**：`EjectBall* eject()` 替代 `void eject()`，返回新孢子（nullptr 表示不满足条件）

### Step 10: 编译验证
**前置依赖**：Step 1~9 全部完成

- `cmake --build build` 编译通过
- 确保无 warning、无 error
- 更新对应的 docs 文件（如 Ball.md、GameScene.md）

---

## 任务依赖图

```
Step 1 (Entity 扩展)
 ├─→ Step 2 (eat) ──┐
 ├─→ Step 3 (split) ─┤
 ├─→ Step 4 (eject) ─┤
 ├─→ Step 5 (debuff) ┤
 ├─→ Step 7 (attraction) ┤
 │                      ↓
 └────────────────→ Step 6 (碰撞检测) ← Step 9 (GameScene 补充方法)
                           ↓
                    Step 8 (AI 分裂)
                           ↓
                    Step 10 (编译验证)
```

- Step 1 是所有的基础
- Step 9 需在 Step 3/4 之前考虑设计
- Step 6 依赖 Step 2~5
- Step 8 依赖 Step 3 + Step 6
