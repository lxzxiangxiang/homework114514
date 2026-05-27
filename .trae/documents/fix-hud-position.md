# HUD 位置修复计划

## Bug 诊断

**位置**: [UIManager.cpp:L329-L331](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L329-L331)

**原因**: 上次"加固"引入了一个新 Bug。

```cpp
// 当前（有 Bug）:
QRectF vr = m_view->viewport()->rect();
QPointF vpCenter = m_view->mapToScene(vr.center().toPoint());
QPointF hudOrigin(vpCenter.x() - vr.width() / 2.0, vpCenter.y() - vr.height() / 2.0);
```

`vr.width() / 2.0` = 640（**设备像素**），但 `vpCenter` 是**场景坐标**。当 zoom ≠ 1.0 时，这两个值不在同一个坐标空间，减法无意义。

例如 zoom = 0.5 时，viewport 640 像素对应放大后的 1280 场景单位，但代码仍减 640 而非 1280，导致 HUD 定位错误。

**原始方案 `mapToScene(0, 0)` 才是正确的**：Qt 内部正确处理了缩放变换，返回 viewport 左上角精确对应的场景坐标。

---

## 修复（1 步）

- `UIManager.cpp` L329-L331：恢复为 `QPointF hudOrigin = m_view->mapToScene(0, 0);`

```cpp
// 修复后:
QPointF hudOrigin = m_view->mapToScene(0, 0);
```

---

## 编译验证
