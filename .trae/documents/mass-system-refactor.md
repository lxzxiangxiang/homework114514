# 质量系统重构 + 效果统一 + AI 追豆 + 文档更新 计划

## 变更总览

| 需求 | 类型 | 简述 |
|------|:---:|------|
| 半径依据质量 | 🔴 重构 | Entity 存储 `m_mass`，`radius() = √(mass/π)` |
| skill/debuff→效果 | 📄 文档 | 文档中合并描述，HUD 已合并 |
| Grow 改为质量 | 🔴 代码 | 吃时 mass×2，消失时 mass×0.6 |
| AI 追 Food | 🟡 代码 | 无威胁时追最近豆子 |
| 设计文档更新 | 📄 | 同步所有变更 |

---

## 1. Entity.h — 质量存储

```
当前: m_radius (qreal)
改为: m_mass (qreal)

radius() → sqrt(m_mass / M_PI)
setRadius(r) → m_mass = M_PI × r²; setRect(...)
新增: mass() / setMass(m) 直接操作质量
```

Entity 构造函数 `Entity(radius, color)` → `Entity(mass, color)`（或保留重载做兼容）。

---

## 2. Constants.h — 所有半径常量→质量常量

| 原常量 | 原值 | 新常量 | 新值(=π×r²) |
|--------|------|--------|-------------|
| `MIN_RADIUS` 10 | 删除 | `MIN_MASS` | `314.16` |
| `FOOD_RADIUS_MIN` 3 | 删除 | `FOOD_MASS_MIN` | `28.27` |
| `FOOD_RADIUS_MAX` 8 | 删除 | `FOOD_MASS_MAX` | `201.06` |
| `SKILLBALL_RADIUS` 12 | 删除 | `SKILLBALL_MASS` | `452.39` |
| `EJECTBALL_RADIUS` 8 | 删除 | `EJECTBALL_MASS` | `201.06` |
| `HAZARD_RADIUS` 60 | 删除 | `HAZARD_MASS` | `11309.73` |
| `SPLIT_THRESHOLD` 18 | 删除 | `SPLIT_MASS_THRESHOLD` | `1017.88` |
| `EJECT_THRESHOLD` 25 | 删除 | `EJECT_MASS_THRESHOLD` | `1963.50` |
| `VICTORY_TOTAL_RADIUS` 2000 | 删除 | `VICTORY_TOTAL_MASS` | 根据面积等效 |
| `BOMB_RADIUS_RATIO` 0.85 | 改为 | `BOMB_MASS_RATIO` | `0.85²=0.7225` |
| `POISON_RADIUS_PER_SEC` 2 | 改为 | `POISON_MASS_PER_SEC` | `π×2²/1=?` 简化 |

**胜利条件**: `VICTORY_TOTAL_RADIUS=2000` 含义不明确（总半径和?），保持为总质量判定阈值，不修改。

---

## 3. Entity 子类改动

### 3.1 Food
- 构造函数: `Food(qreal mass = -1)`，随机 `bounded(FOOD_MASS_MIN, FOOD_MASS_MAX)`
- 质量随机 `generateDouble() × (MAX-MIN) + MIN`

### 3.2 SkillBall
- 构造：`Entity(SKILLBALL_MASS, color)`

### 3.3 Hazard
- 构造：`Entity(HAZARD_MASS, color)`

### 3.4 EjectBall
- 构造：`Entity(EJECTBALL_MASS, color)`

### 3.5 Ball
- 构造：`Ball(mass, color, isPlayer, aiLevel)`

---

## 4. Ball 质量系统重构

### 4.1 Ball::move()
```diff
- speed = BASE_SPEED × √(MIN_RADIUS / radius)
+ speed = BASE_SPEED × √(MIN_MASS / mass())
```
（等价变换，因为 `√(10²/r²) = 10/r = √(π×100 / π×r²) = √(MIN_MASS / mass)`）

### 4.2 Ball::split()
```diff
- if (radius < SPLIT_THRESHOLD) return
- newRadius = radius × √0.5
+ if (mass() < SPLIT_MASS_THRESHOLD) return
+ newMass = mass() × 0.5
+ setMass(newMass)
- new Ball(newRadius, ...)
+ new Ball(newMass, ...)
+ offsetDist = 2×newBall.radius() + 30  (偏移仍用半径算距离)
```

### 4.3 Ball::eject()
```diff
- newRadius = √(r² - EJECTBALL_RADIUS²)
- setRadius(newRadius)
+ newMass = mass() - EJECTBALL_MASS
+ if (newMass < 10) return nullptr     // 防止质量过低
+ setMass(newMass)
```

### 4.4 Ball::eat()
```diff
- newRadius = √(r1² + r2²)
- setRadius(newRadius)
+ setMass(mass() + target->mass())
```

面积相加 → 质量直接相加，**公式更简洁**。

### 4.5 Ball::applySkill(Grow)
```diff
- m_growOriginalRadius = m_radius
- setRadius(m_radius × 1.3)
+ m_growOriginalMass = mass()
+ setMass(mass() × 2.0)          // ← 用户要求 ×2
```

### 4.6 Ball::update() — Grow 到期
```diff
- setRadius(m_growOriginalRadius)
+ setMass(m_growOriginalMass × 0.6)    // ← 用户要求 ×0.6
```

### 4.7 Ball::applyDebuff(Bomb)
```diff
- setRadius(max(r × 0.85, 1))
+ setMass(max(mass() × BOMB_MASS_RATIO, 10))   // 0.85² = 0.7225
```

### 4.8 Ball::update() — Poison 持续
```diff
- setRadius(max(r - 2×dt, 1))
+ setMass(max(mass() - POISON_MASS_PER_SEC × dt, 10))
```

---

## 5. SkillType + DebuffType → 统一效果系统

**代码层**: 保留两个 enum 不变（一个球可同时有技能和减益），但 Ball 成员重命名：

```diff
- SkillType skill; qreal skillTimer;
- DebuffType debuff; qreal debuffTimer;
+ EffectType effect; qreal effectTimer;   // 正效果
+ DebuffType debuff; qreal debuffTimer;   // 负面效果
```

**HUD层**: 已经合并为 "效果: 加速(3.2s), 中毒(1.5s)"，无需代码改动。

**文档层**: 统一描述为"效果系统"，分正效果/负效果。

**Ball.h 新增成员**:
```cpp
qreal m_growOriginalMass = 0;   // 替代 m_growOriginalRadius
```

---

## 6. AI 追 Food

### 6.1 AIController.h
恢复 `foods` 参数：
```cpp
static void updateAI(Ball* ai, const QList<Ball*>& allBalls,
                     const QList<Food*>& foods, qreal dt);
```

### 6.2 AIController.cpp — 决策优先级扩展
```
当前: 逃脱 > 追猎(Ball) > 巡逻
改为: 逃脱 > 追猎(Ball) > 追Food > 巡逻
```

新增猎物检测（Ball）之后的 Food 检测：
```cpp
// ===== Food检测 =====
bool hasFoodTarget = false;
QPointF foodDirection(0, 0);
qreal bestFoodDist = 1e9;
for (Food* food : foods) {
    if (!food->isAlive()) continue;
    QPointF delta = food->pos() - myPos;
    qreal dist = length(delta);
    if (dist < bestFoodDist) {
        bestFoodDist = dist;
        foodDirection = delta;
        hasFoodTarget = true;
    }
}

// 行为优先级：逃脱 > 追猎 > 追Food > 巡逻
if (hasThreat) {
    state.targetDirection = normalized(fleeDirection);
} else if (hasPrey) {
    state.targetDirection = normalized(chaseDirection);
} else if (hasFoodTarget) {
    state.targetDirection = normalized(foodDirection);
} else {
    // 巡逻...
}
```

---

## 7. 设计文档更新

文档中所有"半径"描述改为基于质量，新增质量系统说明章节。

---

## 实施步骤

### Step 1: Entity.h 质量重构 (新增 mass()/setMass(), radius() 改为计算)
### Step 2: Constants.h 全部半径常量 → 质量常量
### Step 3: Food/SkillBall/Hazard/EjectBall 构造适配
### Step 4: Ball 全套重构 (move/split/eject/eat/applySkill/update)
### Step 5: Ball.h 成员更新 (Grow 用 mass, 合并描述)
### Step 6: GameScene 适配 (碰撞判定/spawn/checkCollisions/applyAttraction)
### Step 7: AI 追 Food (AIController+GameScene)
### Step 8: GameView/UIManager 适配
### Step 9: 文档同步更新
### Step 10: 编译验证
