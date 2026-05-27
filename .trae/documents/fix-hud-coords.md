# HUD 在地图左上角而非屏幕左上角

**项目**: `D:\code\project\0.0.1\AgarClone_Qt`

---

## 根因

Qt6 中 `QGraphicsView::drawForeground()` 的 painter **仍然携带场景的变换矩阵**（`scale()` + `centerOn()` 的映射）。所以 `margin=10` 画出来实际落在地图坐标 `(10, 10)` 处，而非屏幕左上角。

摄像机跟随玩家移动后，地图 `(10, 10)` 不在视野内，HUD 就不可见。

## 修复

一行 `painter->resetTransform()` 回到视口坐标系：

[GameView.cpp](file:///D:/code/project/0.0.1/AgarClone_Qt/GameView.cpp#L186) 的 `drawForeground()` 中，绘制 HUD 前重置变换，结束后恢复：

```cpp
void GameView::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);
    if (!m_gameScene || m_state != State::Playing) return;

    painter->save();
    painter->resetTransform();

    painter->setRenderHint(QPainter::Antialiasing, true);
    // ... 原有 HUD 绘制不变 ...

    painter->restore();
}
```

## 修改文件

| 文件 | 修改 |
|------|------|
| `GameView.cpp` L186-L212 | `save()`/`resetTransform()` 在绘制前，`restore()` 在绘制后 |