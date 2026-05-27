# 全流程 Bug 修复 + 代码优化计划

## Bug 总览（8 个 Bug + 4 项优化）

---

## Bug 1 🔴 致命 — `Ball::update(dt)` 从未被调用 → 所有计时器失效

**位置**: [GameScene.cpp L77-L88](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L77-L88)

**原因**: `updateGame()` 中对玩家球体只调了 `move()`，对 AI 球体通过 AIController 也只调了 `move()`，**从未调用 `ball->update(dt)`**。

**后果**:
- `splitTimer` 永不递减 → 分裂一次后永远不能再分裂
- `mergeTimer` 永不递减 → 同源合并冷却永远不结束
- `invincibleTimer` 永不递减 → 一旦无敌就永远无敌
- `skillTimer` 永不递减 → 技能获得后永不过期
- `debuffTimer` 永不递减 → Poison 持续掉血不生效，Trap 永久减速
- `vx/vy *= 0.95f` 摩擦不生效

**修复**: 在 `updateGame()` 中，AI 循环之后添加遍历所有 ball 的 update：
```cpp
for (Ball* ball : allBalls) {
    if (ball->isAlive()) ball->update(dt);
}
```

---

## Bug 2 🔴 致命 — `SkillBall::update(dt)` / `Hazard::update(dt)` 从未被调用

**位置**: [GameScene.cpp L90-L96](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L90-L96)

**原因**: 只有 ejectBalls 的 `update()` 被调用了。SkillBall 和 Hazard 的 `update()` 负责递减 lifetime。

**后果**: 技能球和危险物生成后永不过期，直到被碰撞。

**修复**: 添加遍历：
```cpp
for (SkillBall* sb : skillBalls) { if (sb->isAlive()) sb->update(dt); }
for (Hazard* h : hazards) { if (h->isAlive()) h->update(dt); }
```

---

## Bug 3 🟡 中等 — `Grow` 技能从未增大半径

**位置**: [Ball.cpp L91-L113](file:///d:/code/project/0.0.1/AgarClone_Qt/Ball.cpp#L91-L113) `applySkill`

**原因**: `applySkill(SkillType::Grow)` 只设置了 `skillTimer = 4s`，但从未实际增大半径。文档要求半径 ×1.3。

**修复**: 在 `applySkill` 中：
```cpp
case SkillType::Grow:
    skillTimer = GROW;
    m_growOriginalRadius = m_radius;  // 保存原始半径
    setRadius(m_radius * 1.3f);
    break;
```
在 `update()` 中 skillTimer 到期时：
```cpp
if (skillTimer <= 0) {
    if (skill == SkillType::Grow && m_growOriginalRadius > 0) {
        setRadius(m_growOriginalRadius);
        m_growOriginalRadius = 0;
    }
    skill = SkillType::None;
}
```
需在 `Ball.h` 添加 `qreal m_growOriginalRadius = 0`。

---

## Bug 4 🟡 中等 — `Speed` 技能从未加速

**位置**: [Ball.cpp L19-L24](file:///d:/code/project/0.0.1/AgarClone_Qt/Ball.cpp#L19-L24) `move()`

**原因**: `move()` 中有 Trap 减速判断，但没有 Speed 加速判断。文档要求速度 ×1.5。

**修复**: 在 `move()` 中添加：
```cpp
if (skill == SkillType::Speed && skillTimer > 0) {
    speed *= 1.5f;
}
```

---

## Bug 5 🟡 中等 — `Invisible` 技能 AI 不无视

**位置**: [AIController.cpp L73-L84](file:///d:/code/project/0.0.1/AgarClone_Qt/AIController.cpp#L73-L84)

**原因**: AI 的威胁检测和猎物检测遍历所有球体，不检查目标的 `skill == Invisible`。

**后果**: 有隐身技能的球体仍被 AI 看到和攻击。

**修复**: 在威胁检测和猎物检测的循环中添加：
```cpp
if (other->skill == SkillType::Invisible && other->skillTimer > 0) continue;
```

---

## Bug 6 🟡 中等 — `Ball::update()` 中存在类继承冲突

**位置**: [Ball.cpp L151](file:///d:/code/project/0.0.1/AgarClone_Qt/Ball.cpp#L151)

**原因**: `Ball::update(qreal dt)` 和 `Entity::update(qreal dt)` 同名不同功能。`Ball::update` 管理计时器，但 `EjectBall::update` 管理运动。当前只有 Ball::update 需要被调用。

**说明**: 这不是 bug，只是需要确保 GameScene 中分别调用 `eb->update(dt)` 和 `ball->update(dt)`。

---

## Bug 7 🟢 轻微 — 死代码 `clampLength`

**位置**: [AIController.cpp L14-L20](file:///d:/code/project/0.0.1/AgarClone_Qt/AIController.cpp#L14-L20)

**修复**: 删除该函数。

---

## Bug 8 🟢 轻微 — GameView::advanceGame 中 `maxBallRadius` 未使用

**位置**: [GameView.cpp L86](file:///d:/code/project/0.0.1/AgarClone_Qt/GameView.cpp#L86)

**修复**: 删除 `qreal maxBallRadius = 0;` 和 `maxBallRadius = qMax(...);`

---

## 优化 1: 合并 GameView::advanceGame 中两次遍历

**位置**: [GameView.cpp L62-L134](file:///d:/code/project/0.0.1/AgarClone_Qt/GameView.cpp#L62-L134)

**现状**: `playerBalls` 先遍历一次算 totalRadius，再遍历一次算 HUD 数据。

**优化**: 合并为一次遍历，同时累计 totalRadius + HUD 数据。

---

## 优化 2: 合并 GameScene::checkCollisions 中的 4 次遍历

**位置**: [GameScene.cpp L213-L275](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L213-L275)

**现状**: 碰撞 1~4 各自独立遍历 `allBalls`。

**优化**: 合并为单次遍历，每个 ball 一次性检查 Food/SkillBall/Hazard/EjectBall。

---

## 优化 3: UIManager::createHUDItems 辅助函数

**位置**: [UIManager.cpp L25-L101](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L25-L101)

**现状**: 9 个 HUD 项，每个 6 行近乎重复代码。

**优化**: 提取 `createHUDItem(text)` 辅助函数，返回 `QGraphicsTextItem*`。

---

## 优化 4: AIController 重复的 turnSpeed switch 合并

**位置**: [AIController.cpp L51-L63](file:///d:/code/project/0.0.1/AgarClone_Qt/AIController.cpp#L51-L63) + [L144-L157](file:///d:/code/project/0.0.1/AgarClone_Qt/AIController.cpp#L144-L157)

**现状**: decisionTimer 和 turnSpeed 的 switch 结构完全相同。

**优化**: 不需要修改，两个 switch 取值不同（一个取 DECISION_TIME，一个取 TURN_RATE），无法合并。改为用辅助函数 `getAILevelParam(level)` 或直接保持现状。

实际上这个优化不合适，两个 switch 虽然结构相同但取不同常量。跳过。

---

## 实施步骤

### Step 1: Bug 1+2 — 修复 update() 调用缺失
- `GameScene::updateGame` 中添加 `ball->update(dt)`、`sb->update(dt)`、`h->update(dt)` 遍历

### Step 2: Bug 3 — Grow 技能半径变化
- `Ball.h` 添加 `m_growOriginalRadius`
- `Ball::applySkill` 中 Grow 分支设置半径 ×1.3
- `Ball::update` 中 skill 到期时恢复原始半径

### Step 3: Bug 4 — Speed 技能加速
- `Ball::move` 中添加 Speed 加速判断

### Step 4: Bug 5 — Invisible 对 AI 隐身
- `AIController::updateAI` 中威胁/猎物检测循环添加隐身跳过

### Step 5: Bug 7+8 — 死代码清理
- 删除 AIController::clampLength
- 删除 GameView::maxBallRadius

### Step 6: 优化 1+2 — 合并遍历
- GameView::advanceGame 合并为一次 playerBalls 遍历
- GameScene::checkCollisions 合并碰撞 1~4 的 4 次 allBalls 遍历为一次

### Step 7: 优化 3 — UIManager 辅助函数
- 添加 `createHUDItem()` 辅助方法，减少重复代码

### Step 8: 编译验证

---

## 任务依赖
- Step 1 独立
- Step 2~4 可并行（无相互依赖）
- Step 5~7 可并行（无相互依赖）
- Step 8 依赖 Step 1~7
