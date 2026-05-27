# 修 Bug 计划

## Bug 诊断

### Bug 1 🔴 Poison 减血可将半径降至 ≤ 0 导致崩溃

**位置**: [Ball.cpp L175](file:///d:/code/project/0.0.1/AgarClone_Qt/Ball.cpp#L175)

**原因**: `setRadius(m_radius - 2.0 * dt)` 无下限保护。半径降到 0 或负数时 `setRect(-r, -r, r*2, r*2)` 产生无效矩形 → 渲染崩溃。

**修复**: `setRadius(m_radius - 2.0 * dt)` → `setRadius(qMax(m_radius - GameConstants::HazardEffect::POISON_RADIUS_PER_SEC * dt, 1.0))`

同样 Bomb 爆炸也需要保护：`setRadius(m_radius * 0.85f)` → `setRadius(qMax(m_radius * 0.85f, 1.0))`

---

### Bug 2 🟡 `applyAttraction` 引用已删除的 `splitTimer`

**位置**: [GameScene.cpp L334-L337](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L334-L337)

**代码**:
```cpp
if (!b1->isAlive() || b1->splitTimer > 0) continue;
if (!b2->isAlive() || b2->splitTimer > 0) continue;
```

`splitTimer` 已不再被赋值（永远为 0），但仍引用它的逻辑。虽不崩溃，但代码混乱。

**修复**: 删除 `|| b1->splitTimer > 0` 和 `|| b2->splitTimer > 0`。

---

### Bug 3 🟡 `Ball::update()` 仍递减已废弃的 `splitTimer`

**位置**: [Ball.cpp L155](file:///d:/code/project/0.0.1/AgarClone_Qt/Ball.cpp#L155)

**代码**: `if (splitTimer > 0) splitTimer -= dt;` — splitTimer 永远为 0，死代码。

**修复**: 删除该行。

---

### Bug 4 🟡 AIController `s_states` 内存泄漏 + 悬挂指针

**位置**: [AIController.cpp L11](file:///d:/code/project/0.0.1/AgarClone_Qt/AIController.cpp#L11) + [GameScene.cpp L376-L377](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L376-L377)

**原因**: AI 球体死亡后被 `delete` 释放，但 `s_states` 哈希表中仍保留该 Ball 指针的 Entry（悬挂指针），且 `resetState()` 从未被调用。

**后果**: 内存泄漏（每个 AI 约 40 字节状态永不释放），且若未来有人遍历 `s_states` 则访问悬挂指针崩溃。

**修复**: 在 `removeDeadEntities` 中，删除 aiBalls 里的死亡实体前先调用 `AIController::resetState(ball)`。

需在 GameScene.cpp `#include "AIController.h"`，并在 `removeFromList(aiBalls)` 的 lambda 中特殊处理。

最简单的方式：不修改泛型 lambda，而是在 removeDeadEntities 末尾单独处理 aiBalls：
```cpp
// Clean AI states before removing dead AI balls
for (int i = aiBalls.size() - 1; i >= 0; --i) {
    if (!aiBalls[i]->isAlive()) {
        AIController::resetState(aiBalls[i]);
    }
}
```
然后将 `removeFromList(aiBalls)` 放在这之后（当前它已经在 lambda 调用列表中）。

实际更简单：在 `removeFromList` lambda 调用之前，加一行：
```cpp
for (Ball* ai : aiBalls) { if (!ai->isAlive()) AIController::resetState(ai); }
```

---

### Bug 5 🟢 `showMenu()` 中重复隐藏 overlay 和文本

**位置**: [UIManager.cpp L157-L171](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L157-L171)

**现状**: `showMenu()` 单独隐藏了 `m_gameOverText`/`m_gameOverOverlay`/`m_victoryText`/`m_victoryOverlay`，然后末尾又隐藏了所有 HUD。可以合并清理。

**修复**: 不修复（影响太小）。

---

### Bug 6 🟢 `startGame()` 不重置摄像机，但 `updateCamera()` 第一帧会处理

摄像机状态：`startGame()` 不调用 `resetTransform()`，但新玩家球体在 (2500,2500)，第一帧 `updateCamera()` 会正常跟随。旧变换在 `setScene(nullptr)` 时可能残留。

**修复**: `startGame()` 中添加 `resetTransform()`。

---

## 实施步骤

### Step 1: Bug 1 — Poison/Bomb 半径下限保护
- `Ball::update()` L175: `setRadius(qMax(m_radius - ..., 1.0))`
- `Ball::applyDebuff()` L124: `setRadius(qMax(m_radius * 0.85f, 1.0))`

### Step 2: Bug 2 — applyAttraction 去 splitTimer
- `GameScene.cpp` L334: 删除 `|| b1->splitTimer > 0`
- `GameScene.cpp` L337: 删除 `|| b2->splitTimer > 0`

### Step 3: Bug 3 — Ball::update 去 splitTimer 递减
- `Ball.cpp` L155: 删除 `if (splitTimer > 0) splitTimer -= dt;`

### Step 4: Bug 4 — AIController resetState 调用
- `GameScene.cpp` `removeDeadEntities()` 中，在 `removeFromList(aiBalls)` 调用前添加 `for (Ball* ai : aiBalls) { if (!ai->isAlive()) AIController::resetState(ai); }`

### Step 5: Bug 6 — startGame 重置摄像机
- `GameView::startGame()` 中添加 `resetTransform();`

### Step 6: 编译验证
