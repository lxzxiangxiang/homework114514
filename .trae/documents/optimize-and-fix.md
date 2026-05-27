# 优化代码+修Bug 计划

## Bug 总览（6个）

### Bug 1 🔴 暂停遮罩在摄像机偏移时不可见

**位置**: [UIManager.cpp:L111](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L111) + [GameView.cpp:L212](file:///d:/code/project/0.0.1/AgarClone_Qt/GameView.cpp#L212)

**原因**: `showPause()` 创建的遮罩在场景坐标 (0,0)-(1280,720)，但 `pauseGame()` 不重置摄像机。当玩家在 (2500,2500) 时按 ESC，遮罩渲染在视口之外。

**修复**: `pauseGame()` 中添加 `resetTransform() + centerOn(640,360)`。

---

### Bug 2 🟡 Bomb 减益 HUD 永久显示不消失

**位置**: [Ball.cpp:L123-L124](file:///d:/code/project/0.0.1/AgarClone_Qt/Ball.cpp#L123-L124)

**原因**: Bomb `debuffTimer = 0`；`update()` 中 `if (debuffTimer > 0)` → Bomb 分支被跳过 → `debuff = None` 永不执行。HUD 永久显示"炸弹"。

**修复**: Bomb 分支末尾添加 `debuff = DebuffType::None;`

---

### Bug 3 🟡 `startGame()` 后 `s_states` 残留旧 AI 悬挂指针

**位置**: [GameView.cpp:L186-L203](file:///d:/code/project/0.0.1/AgarClone_Qt/GameView.cpp#L186-L203)

**原因**: `delete m_gameScene` 时场景中残留的存活 AI 不会被 `removeDeadEntities` 清理，`s_states` 保留悬挂指针。

**修复**: `startGame()` 中 delete 前遍历清理：
```cpp
for (Ball* ai : m_gameScene->aiBalls) {
    AIController::resetState(ai);
}
```

---

### Bug 4 🟡 HUD 每帧设置6次字体位置（重复 setVisible）

**优化**，非 bug。当前每帧都调用 `setVisible(true)` 和 `setPos/setFont`。由于 `updateHUD` 已有设置即可，不改。

### Bug 4（重新编号）修正：applyAttraction 可推球出界

**位置**: [GameScene.cpp:L379-L380](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L379-L380)

**原因**: `setPos()` 直接写位置，无边界检查。引力可把球体推出地图。

**修复**: 添加 clamp 到 [0, MAP_WIDTH] × [0, MAP_HEIGHT]。

---

### Bug 5 🟢 过时注释

| 位置 | 内容 | 修复 |
|------|------|------|
| GameScene.cpp L2 | "每帧执行 9 步" | → "每帧执行 17 步" |
| GameScene.cpp L21 | "100 个豆子、5 个 AI" | → "200 个豆子、20 个 AI" |
| GameScene.cpp L92 | "分裂/合并/无敌/技能/减益" | → "技能/减益" |
| GameView.cpp L24 | "Agar.io Clone" | → 中文标题 |

---

### Bug 6 🟢 Magnet 将 Food 推到地图外

**位置**: [GameScene.cpp:L108](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L108)

**修复**: `setPos()` 后 clamp Food 位置。

---

## 优化（4项）

### 优化 1: checkCollisions 的 `else if (!sameOwner)`→`else`

**位置**: [GameScene.cpp:L335](file:///d:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L335)

`else if (!sameOwner)` → `else`，条件冗余。

### 优化 2: Ball 删除 `isInvincible()` 

已等同于 `hasShield()`，直接调用 `hasShield()` 即可。

### 优化 3: GameView::advanceGame 中 `activeCount` 计算可与总半径遍历合并

合并为一次遍历。

### 优化 4: AIController remove unused `foods` parameter

`updateAI` 接受 `foods` 但从未使用。

---

## 实施步骤

### Step 1: 暂停摄像机修复 (GameView.cpp)
### Step 2: Bomb 减益清空 (Ball.cpp)
### Step 3: s_states 清理 (GameView.cpp)
### Step 4: 吸引力/Magnet 边界 clamp (GameScene.cpp)
### Step 5: 过时注释修正 (GameScene.cpp, GameView.cpp)
### Step 6: 优化 1-4 (GameScene.cpp, Ball, GameView.cpp, AIController)
### Step 7: 编译验证
