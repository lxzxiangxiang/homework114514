# HUD 字体大小随缩放变化 计划

## 原因

无 `ItemIgnoresTransformations` → view `scale(zoom)` 会缩放字体视觉大小。需将字体点大小设为 `12/zoom` 使屏幕视觉恒定。

## 实施（1 步）

### UIManager.cpp `updateHUD()` L263 后

在设置文本后、定位前，为 5 个 HUD item 设置动态字体：

```cpp
int fontSize = qMax(1, qRound(12.0 / zoom));
QFont hudFont("Arial", fontSize);
m_hudScore->setFont(hudFont);
m_hudTime->setFont(hudFont);
m_hudRadius->setFont(hudFont);
m_hudAICount->setFont(hudFont);
m_hudEffects->setFont(hudFont);
```

### 编译验证
