# HUD 随摄像机定位 + 缩放 计划

## 方案

去掉 `ItemIgnoresTransformations`，HUD item 受 view transform 影响。
每帧 `updateHUD()` 中，用 `mapToScene()` 将视口左上角映射到场景坐标定位 HUD，行高除以当前 zoom 使视觉间距恒定。

## 实施（1 步）

### UIManager.cpp

1. `createHUDItem()`: 删除 `setFlag(ItemIgnoresTransformations)`
2. `updateHUD()` 签名增加 `qreal zoom` 参数
3. `updateHUD()` 定位逻辑：
```cpp
qreal baseLineHeight = 18.0;
qreal baseMargin = 10.0;
qreal effectiveLineHeight = baseLineHeight / zoom;
qreal effectiveMargin = baseMargin / zoom;
QPointF hudOrigin = m_view->mapToScene(QPointF(baseMargin, baseMargin));
// 5 项逐行：hudOrigin + (0, effectiveLineHeight * N)
```

### UIManager.h

- `updateHUD()` 签名加 `qreal zoom` 参数

### GameView.cpp

- 调用 `updateHUD(...)` 时传入 `m_currentZoom`

### 编译验证
