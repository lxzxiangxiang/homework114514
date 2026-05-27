# 修复摄像机 + HUD + 分裂 + 碰撞判断

**项目**: `D:\code\project\0.0.1\AgarClone_Qt`

---

## 问题 1: 摄像机缩放应依据总质量而非最大半径

### 当前

[GameView.cpp:L296-L304](file:///D:/code/project/0.0.1/AgarClone_Qt/GameView.cpp#L296-L304) 用 `maxRadius`（所有玩家球中的最大单个半径）来计算缩放：

```cpp
qreal maxRadius = GameConstants::MIN_RADIUS;
for (const Ball* ball : balls) {
    if (ball->isAlive()) maxRadius = qMax(maxRadius, ball->radius());
}
qreal targetZoom = GameConstants::CAMERA_ZOOM_MAX * (GameConstants::MIN_RADIUS / maxRadius);
```

**问题**: 分身后变成多个小球，`maxRadius` 很小 → 镜头立即拉近 → 视野缩小，无法看到远处的威胁和食物。

### 修复

改用 `totalMass`（所有玩家球的质量加权平均半径）来计算缩放。质量 = πr²，因此等效半径 = √(totalMass/π)。

**GameView.cpp** `updateCamera()` 中替换缩放部分：

```cpp
// 3. 依据总质量计算缩放
qreal totalMass = 0;
for (const Ball* ball : balls) {
    if (ball->isAlive()) totalMass += ball->radius() * ball->radius();
}
qreal equivalentRadius = (totalMass > 0) ? std::sqrt(totalMass) : GameConstants::MIN_RADIUS;
equivalentRadius = qMax(equivalentRadius, GameConstants::MIN_RADIUS);

qreal targetZoom = GameConstants::CAMERA_ZOOM_MAX * (GameConstants::MIN_RADIUS / equivalentRadius);
targetZoom = qBound(GameConstants::CAMERA_ZOOM_MIN, targetZoom, GameConstants::CAMERA_ZOOM_MAX);
m_currentZoom += (targetZoom - m_currentZoom) * 0.08f;
```

等效半径 = √(Σr²)，即"如果把所有球合并成一个球，它的半径是多少"。分身后多个小球合并的总质量不变，屏幕不会缩回去。

---

## 问题 2: HUD 文本不显示

### 根因

`GameScene::drawForeground()` 在 [GameScene.cpp:L413-L435](file:///D:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L413-L435) 中使用 viewport 坐标绘制（`margin=10`），但 `QGraphicsScene::drawForeground()` 的 painter 坐标取决于 Qt 版本和 `FullViewportUpdate` 模式下的具体实现。当前的绘制可能因为坐标系不一致而不可见。

### 修复

将 HUD 绘制从 `QGraphicsScene::drawForeground()` 移到 `QGraphicsView::drawForeground()`，后者**确保是 viewport 坐标系**。

**GameView.h** — 添加 `drawForeground` 声明：
```cpp
protected:
    void drawForeground(QPainter* painter, const QRectF& rect) override;
```

**GameView.cpp** — 实现 `drawForeground()`，从 GameScene 读取 HUD 文本字段绘制：

```cpp
void GameView::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);
    if (!m_gameScene || m_state != State::Playing) return;

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
}
```

**GameScene.h** — 删除 `drawForeground` 声明和实现：
```cpp
// 删除 protected 中的:
// void drawForeground(QPainter* painter, const QRectF& rect) override;
```

**GameScene.cpp** — 删除 `drawForeground()` 实现（整个函数体）。

---

## 问题 3: 按一次空格连续多次分裂

### 根因

[GameView.cpp:L335](file:///D:/code/project/0.0.1/AgarClone_Qt/GameView.cpp#L335) 每帧都设置 `wantSplit = m_keysPressed.contains(Qt::Key_Space)`。只要空格按住，**每一帧** `wantSplit` 都为 `true`，而 `processSplitEject()` 中遍历所有符合条件的球体分裂。如果玩家按住空格不放，每一帧（16ms）都会触发分裂，导致连续多次分裂。

### 修复

改为**边沿触发**：只在按下瞬间消费一次，需要松开再按。

**GameView.h** — 添加成员：
```cpp
bool m_splitFired = false;
bool m_ejectFired = false;
```

**GameView::processPlayerInput()** — 改脉冲式：

```cpp
bool spaceDown = m_keysPressed.contains(Qt::Key_Space);
bool eDown = m_keysPressed.contains(Qt::Key_E);

if (spaceDown && !m_splitFired) {
    m_gameScene->wantSplit = true;
    m_splitFired = true;
}
if (!spaceDown) {
    m_splitFired = false;
}

if (eDown && !m_ejectFired) {
    m_gameScene->wantEject = true;
    m_ejectFired = true;
}
if (!eDown) {
    m_ejectFired = false;
}
```

删除原来的：
```cpp
m_gameScene->wantSplit = m_keysPressed.contains(Qt::Key_Space);
m_gameScene->wantEject = m_keysPressed.contains(Qt::Key_E);
```

---

## 问题 4: 合并/吞噬判断：大圆边碰小圆心

### 当前

[GameScene.cpp:L339-L340](file:///D:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L339-L340)：

```cpp
qreal contactDist = ball1->radius() + ball2->radius();
if (distSq > contactDist * contactDist) continue;
```

即两个球的**边缘接触**时触发合并/吞噬（两圆半径之和）。

### 修复

改为大圆边碰小圆心，即 `contactDist = max(r1, r2)`。大球只需要自己的边缘碰到小球的中心点即可。

```cpp
qreal contactDist = qMax(ball1->radius(), ball2->radius());
if (distSq > contactDist * contactDist) continue;
```

替换 [GameScene.cpp:L339](file:///D:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L339) 的一行。

---

## 修改文件清单

| 文件 | 修改 |
|------|------|
| `GameView.h` | 添加 `drawForeground()` + `m_splitFired`/`m_ejectFired` 成员 |
| `GameView.cpp` | 摄像机改总质量缩放 + 实现 `drawForeground()` + `processPlayerInput` 边沿触发 |
| `GameScene.h` | 删除 `drawForeground()` 声明 |
| `GameScene.cpp` | 删除 `drawForeground()` 实现 + 碰撞 contactDist 改 `qMax(r1,r2)` |