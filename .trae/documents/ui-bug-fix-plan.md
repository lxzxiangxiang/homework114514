# UI 界面 Bug 检查与修复计划

## 诊断结果

通过分析 `UIManager.cpp`、`GameView.cpp`、`UIManager.h` 完整代码，识别出 **6 个 Bug**：

---

### Bug 1: 🔴 严重 — HUD 随场景坐标移动，玩家离开初始位置后不可见

**位置**: [UIManager.cpp:L353-L378](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L353-L378)

**原因**: HUD 9 个 `QGraphicsTextItem` 通过 `addItem()` 添加到 `QGraphicsScene`，位置固定在场景坐标 `(10, 10+18N)`。摄像机 `centerOn()` 跟随玩家移动到地图中心 (2500, 2500) 后，HUD 位于视口之外的 (10, 10) 位置。

> 需求文档 3.5 节明确警告过：*"缩放摄像机时文本 HUD 也缩放 → HUD 不要添加到 QGraphicsScene"*

**修复方案**: 在 `updateHUD()` 中，将 HUD 位置设置为摄像机当前视口左上角对应的场景坐标。即：

```cpp
QPointF hudBase = view->mapToScene(0, 0); // 视口左上角场景坐标
m_hudScore->setPos(hudBase.x() + margin, hudBase.y() + margin);
m_hudTime->setPos(hudBase.x() + margin, hudBase.y() + margin + lineHeight);
// ... 其他 HUD 项同理
```

或者更简洁的做法：给每个 HUD item 设置 `setFlag(QGraphicsItem::ItemIgnoresTransformations, true)` 并把位置设回 (10, 10)。

---

### Bug 2: 🔴 严重 — `returnToMenu()` 不重置摄像机变换

**位置**: [GameView.cpp:L260-L268](file:///d:/code/project/0.0.1/AgarClone_Qt/GameView.cpp#L260-L268)

**原因**: `returnToMenu()` 只停止计时器和调用 `showMenu()`，没有 `resetTransform()`。菜单背景和文本位于场景坐标 `(0,0)-(1280,720)`，如果玩家之前在远处（如 2500, 2500），菜单不可见。

**修复**: 在 `returnToMenu()` 中添加：
```cpp
resetTransform();
centerOn(GameConstants::WINDOW_WIDTH / 2, GameConstants::WINDOW_HEIGHT / 2);
```

`startGame()` 同理，新建场景后也需重置摄像机。

---

### Bug 3: 🟡 中等 — 时间格式字符串多出 `%3`/`%4`，文本显示乱码

**位置**: 
- [UIManager.cpp:L227](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L227) — `"Survival Time: %2:%3%4"` 
- [UIManager.cpp:L275](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L275) — 同上
- [UIManager.cpp:L336](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L336) — `"Time: %1:%2%3"`

**原因**: `%3` 和 `%4` 没有对应的 `.arg()` 调用，Qt 会将其原样输出为 `%3`、`%4` 字面文本。

例如显示效果：`Survival Time: 0:30%4`（而不是 `Survival Time: 0:30`）

**修复**: 
```cpp
// showGameOver / showVictory: "%2:%3%4" → "%2:%3"
QString("Final Score: %1\nSurvival Time: %2:%3")

// updateHUD: "%1:%2%3" → "%1:%2"
QString("Time: %1:%2")
```

---

### Bug 4: 🟡 中等 — `hideAll()` 不隐藏 GameOver/Victory 的 overlay 背景和附属文本

**位置**: [UIManager.cpp:L299-L324](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L299-L324)

**原因**: `showGameOver()` / `showVictory()` 创建了 `QGraphicsRectItem* overlay`（背景矩形），标题/信息/提示文本作为 overlay 的子项添加。但 `hideAll()` 只隐藏了 `m_gameOverText` 和 `m_victoryText` 这两个标题文本指针，没有隐藏 overlay 矩形本身及其子项 info/hint。

当 `resumeGame()` 调用 `hideAll()` 时（虽然暂停后不会直接进入 GameOver 状态，但如果未来调用 `hideAll()` 时这些 overlay 存在，就会出现问题）。

**修复**: 在 `UIManager` 中添加 `QGraphicsRectItem* m_gameOverOverlay = nullptr` 和 `QGraphicsRectItem* m_victoryOverlay = nullptr` 成员，用于跟踪完整的 overlay 矩形。`hideAll()` 时隐藏这些 overlay。

---

### Bug 5: 🟢 轻微 — GameOver/Victory 每次调用创建新 overlay，内存泄漏

**位置**: [UIManager.cpp:L208-L248](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L208-L248)、[UIManager.cpp:L250-L296](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L250-L296)

**原因**: `showGameOver()` 和 `showVictory()` 每次调用都 `new QGraphicsRectItem`，不检查是否已有。如果在同一场景中多次调用（通过 `gameOver()` → Enter → `startGame()` → 再次 `gameOver()`），由于 `startGame()` 重建了整个 GameScene，旧的 overlay 随旧 scene 销毁，实际上**不泄漏**。但代码不够健壮。

**修复**: 添加对已有 overlay 的检查，如果存在则先删除再创建；或改为惰性创建+重用模式（类似 `showPause()` 的做法）。同时 Bug 4 的修复也解决此问题。

---

### Bug 6: 🟢 轻微 — 菜单按键提示错误："W - Eject Mass"

**位置**: [UIManager.cpp:L122](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L122)

**原因**: 菜单提示写 `"W - Eject Mass"`，实际吐孢按键是 **E**。

**修复**: `"W - Eject Mass"` → `"E - Eject Mass"`

---

## 修复步骤

### Step 1: 修复 Bug 1 — HUD 使用 ItemIgnoresTransformations
- 在 `createHUDItems()` 中为每个 HUD 项添加 `setFlag(QGraphicsItem::ItemIgnoresTransformations, true)`
- 不需要 setPos 到 viewport 坐标，直接用 margin 和 lineHeight 定位即可（因为不受变换影响）

### Step 2: 修复 Bug 2 — 状态切换时重置摄像机
- `returnToMenu()`: 添加 `resetTransform()` + `centerOn(WINDOW_WIDTH/2, WINDOW_HEIGHT/2)`
- `startGame()`: 进入 Playing 前重置摄像机到玩家中心
- `gameOver()` / `victory()`: 添加 `resetTransform()` + `centerOn(...)`

### Step 3: 修复 Bug 3 — 时间格式字符串
- `showGameOver()`: `"Survival Time: %2:%3%4"` → `"Survival Time: %2:%3"`
- `showVictory()`: 同上
- `updateHUD()`: `"Time: %1:%2%3"` → `"Time: %1:%2"`

### Step 4: 修复 Bug 4+5 — GameOver/Victory overlay 管理
- 在 `UIManager.h` 添加 `m_gameOverOverlay` / `m_victoryOverlay` 成员
- `showGameOver()`: 检查已有 overlay，有则删除重建；保存 overlay 到 `m_gameOverOverlay`
- `showVictory()`: 同上
- `hideAll()`: 隐藏 `m_gameOverOverlay` / `m_victoryOverlay` 及其所有子项

### Step 5: 修复 Bug 6 — 菜单按键提示
- `"W - Eject Mass"` → `"E - Eject Mass"`

### Step 6: 编译验证
- `cmake --build` 通过
