# 阶跃式缩放 + HUD 修复 + AI 数量增加 计划

## 需求分析

1. **缩放改为阶跃式** — 当前 `updateCamera()` 每帧平滑计算 zoom 值，改为离散阶梯变化
2. **缩放后 HUD 文本有问题** — `ItemIgnoresTransformations` + `mapToScene(0,0)` 在缩放突变时可能闪烁/错位
3. **增加基础 AI 数量** — 初始 AI 从 5 增加到 15

---

## Bug 诊断

### HUD 问题根因

**位置**: [GameView.cpp L308-L310](file:///d:/code/project/0.0.1/AgarClone_Qt/GameView.cpp#L308-L310) + [UIManager.cpp L332](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L332)

**原因**: `resetTransform()` + `scale(zoom)` 后，`mapToScene(0,0)` 返回的坐标取决于 zoom 值。阶跃式缩放时 zoom 突变，HUD 的 `setPos()` 场景坐标也突变。由于 `ItemIgnoresTransformations` 的作用是"用恒等变换渲染"，item 在 viewport 上的位置 = `pos - viewport_scene_origin`。当 `pos = mapToScene(0,0)` 且 view 使用 `centerOn(X)` 时，`mapToScene(0,0)` 恰好 = `X - viewport_w/(2*zoom)`，但 item 的 viewport 位置 = `pos - (X - viewport_w/(2*zoom))` = `0`（恒等渲染时）。所以理论上只要 `updateHUD` 在 `updateCamera` 之后调用就应该正确。

**实际可能的问题**: 阶跃缩放时 zoom 瞬间变化，但 HUD 更新在下一帧的 `advanceGame` 末尾（在 `updateCamera` 之后），因此应该是正确的。但当前代码流程是：
```
advanceGame():
  updateGame()
  胜负判定
  updateCamera()   ← zoom 在这里变
  updateHUD()      ← HUD 在这里定位
```

如果 HUD 仍有问题，将 `mapToScene(0,0)` 改为直接计算：`hudOrigin = center - viewportSize/(2*zoom)`，其中 center 是 `mapToScene(viewportRect().center())`，viewportSize 是 viewport 尺寸。这样避免 `mapToScene(0,0)` 在极端 zoom 下的精度问题。

---

## 实施步骤

### Step 1: 阶跃式缩放
修改 `GameView::updateCamera()`：

**当前逻辑**:
```cpp
zoom = 1.5 * (10 / maxRadius);
zoom = clamp(zoom, 0.5, 1.5);
```

**改为阶跃式**:
```cpp
// 定义缩放阶梯表
const qreal zoomLevels[] = { 1.5, 1.2, 1.0, 0.8, 0.6, 0.5 };
qreal targetZoom = 1.5 * (10 / maxRadius);
targetZoom = clamp(targetZoom, 0.5, 1.5);
// 选择 ≤ targetZoom 的最大阶梯值
qreal stepZoom = zoomLevels[0];
for (qreal z : zoomLevels) {
    if (z <= targetZoom + 0.01) { stepZoom = z; break; }
}
zoom = stepZoom;
```

缩放阶梯映射：
| 玩家最大半径 | 连续 zoom | 阶梯 zoom |
|-------------|-----------|-----------|
| 10 | 1.50 | 1.5 |
| 10~12 | 1.50~1.25 | 1.5/1.2 |
| 12~15 | 1.25~1.0 | 1.2 |
| 15~18 | 1.0~0.83 | 1.0 |
| 18~25 | 0.83~0.6 | 0.8 |
| 25~30 | 0.6~0.5 | 0.6 |
| 30+ | 0.5 | 0.5 |

### Step 2: HUD 定位加固
修改 `UIManager::updateHUD()` 中 `hudOrigin` 的计算方式，避免依赖 `mapToScene(0,0)` 的数值稳定性：

```cpp
// 用 viewport 中心 + 半尺寸反算左上角，比 mapToScene(0,0) 更稳定
QRectF vr = m_view->viewport()->rect();
QPointF vpCenter = m_view->mapToScene(vr.center());
QPointF hudOrigin(vpCenter.x() - vr.width() / 2.0, vpCenter.y() - vr.height() / 2.0);
```

### Step 3: 增加初始 AI 数量
- `Constants.h` 添加 `AIBALL_COUNT_INIT = 15`
- `GameScene.cpp` 构造函数中 `for (int i = 0; i < 5; ++i)` → `for (int i = 0; i < GameConstants::AIBALL_COUNT_INIT; ++i)`

### Step 4: 编译验证
