# 修复 HUD 文本显示不全 + 去除黑色背景 + 去除网格

**项目**: `D:\code\project\0.0.1\AgarClone_Qt`

---

## 问题 1: HUD 文本显示不全

### 根因

[GameScene.cpp:L418-L430](file:///D:/code/project/0.0.1/AgarClone_Qt/GameScene.cpp#L418-L430) 的 `drawForeground()` 使用 `painter->drawText(x, y, text)` 绘制 HUD，`y` 参数是文本**基线**。第一行基线设在 `margin=10`，但文本的实际顶部在 `y - ascent`（约 `10 - 12 = -2px`），可能被 viewport 裁剪。

### 修复

用 `QRectF` + `Qt::AlignLeft | Qt::AlignTop` 版本替代裸 `(x,y)` 的 `drawText`，确保每行文本完整可见。

**GameScene.cpp** `drawForeground()` 修改：

```cpp
void GameScene::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);

    painter->setRenderHint(QPainter::Antialiasing, true);
    QFont font("Arial", 12);
    painter->setFont(font);
    painter->setPen(Qt::white);

    const int margin = 10;
    const int lineHeight = 20;
    const int maxWidth = 300;

    QRectF lineRect(margin, margin, maxWidth, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, hudScoreText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, hudTimeText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, hudRadiusText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, hudAICountText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, hudEffectsText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, hudSplitText);
}
```

---

## 问题 2: 去除黑色背景

### 当前

[GameView.cpp:L26](file:///D:/code/project/0.0.1/AgarClone_Qt/GameView.cpp#L26) `setBackgroundBrush(Qt::black)` 使场景背景为纯黑。

### 修复

改为深灰 `QColor(30, 30, 30)`，让场景背景不那么刺眼且与黑色实体区分开。

**GameView.cpp** L26 修改：
```cpp
setBackgroundBrush(QColor(30, 30, 30));
```

---

## 问题 3: 去除背景网格绘制

### 当前

AgarClone_Qt 项目**没有**绘制视觉网格的代码。`drawLine` 调用全在 SkillBall/Hazard 的图标绘制中，不是背景网格。SpatialGrid 是碰撞检测用的数据结构，不涉及绘制。

### 结果

无需修改。该项目的背景是纯色无网格。

---

## 修改文件清单

| 文件 | 修改 |
|------|------|
| `GameScene.cpp` L413-431 | `drawForeground()` 改用 `QRectF`+对齐方式绘制，解决文本被裁剪 |
| `GameView.cpp` L26 | `setBackgroundBrush(Qt::black)` → `setBackgroundBrush(QColor(30,30,30))` |