# 分裂/吐孢随机误差 + HUD 修正为屏幕坐标

**项目**: `D:\code\project\0.0.1\AgarClone_Qt`

---

## 问题 1: 分裂/吐孢加入方向及距离随机误差

### 当前

[Ball.cpp:L48-L50](file:///D:/code/project/0.0.1/AgarClone_Qt/Ball.cpp#L48-L50) `splitEjectDir()` 只在**角度**上加随机偏差 `±0.26` 弧度（≈±15°），但**距离**是固定值：

- split: `offsetDist = newRadius * 2.0 + 30.0`（固定）
- eject: 没有 offsetDist 变量（直接 `pos() + dir * offsetDist` 在 split 中）

### 修复

在 `split()` 中给 `offsetDist` 加随机因子，`eject()` 中的 `EjectBall` 构造函数速度本身有方向但也可加微小随机偏差。

**Ball.cpp `split()`**：
```cpp
qreal distJitter = 1.0 + (QRandomGenerator::global()->generateDouble() - 0.5) * 0.4;  // ±20% 距离抖动
qreal offsetDist = (newRadius * 2.0 + 30.0) * distJitter;
```

**Ball.cpp `eject()`**：
```cpp
QPointF dir = splitEjectDir(QPointF(lastDx, lastDy), lastDx, lastDy);
qreal speedJitter = 1.0 + (QRandomGenerator::global()->generateDouble() - 0.5) * 0.3;  // ±15% 速度抖动
return new EjectBall(pos(), m_color, dir.x() * speedJitter, dir.y() * speedJitter);
```

---

## 问题 2: HUD 画到了地图上

### 根因

[GameView.cpp:L186-L212](file:///D:/code/project/0.0.1/AgarClone_Qt/GameView.cpp#L186-L212) `QGraphicsView::drawForeground()` 的 painter 使用**场景坐标系**（受摄像机变换影响）。当场景被 `scale()` + `centerOn()` 变换后，绘制在 `(10, 10)` 的 HUD 实际位于地图的 `(10, 10)` 位置而不是屏幕左上角。

### 修复

在绘制 HUD 前用 `painter->save()` / `painter->resetTransform()` 先重置变换矩阵回到视口坐标，画完后 `restore()` 还原。

```cpp
void GameView::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);
    if (!m_gameScene || m_state != State::Playing) return;

    painter->save();
    painter->resetTransform();  // 回到视口坐标系

    painter->setRenderHint(QPainter::Antialiasing, true);
    QFont font("Arial", 12);
    painter->setFont(font);
    painter->setPen(Qt::white);

    const int margin = 10;
    const int lineHeight = 20;
    const int maxWidth = 300;

    QRectF lineRect(margin, margin, maxWidth, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->hudScoreText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->hudTimeText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->hudRadiusText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->hudAICountText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->hudEffectsText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->hudSplitText);

    painter->restore();
}
```

---

## 修改文件清单

| 文件 | 修改 |
|------|------|
| `Ball.cpp` L66 | `split()`: offsetDist 加 `±20%` 随机抖动 |
| `Ball.cpp` L80-L81 | `eject()`: dir 方向速度加 `±15%` 随机抖动 |
| `GameView.cpp` L186 | `drawForeground()`: 开头加 `painter->save()/resetTransform()`，结尾 `restore()` |