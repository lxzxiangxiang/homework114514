# 修复"运行几秒后崩溃 + 禁用滚轮 + HUD重构foreground"计划

## 问题 1: 运行几秒后崩溃 — BSP 树根因分析

### 根因

Qt `QGraphicsScene` 默认使用 **BspTreeIndex** 索引模式。每帧发生以下操作都会触达 BSP 树：

| 操作 | 触发位置 | 频率 |
|------|----------|------|
| `addItem()` | `spawnFood()`, `spawnAIBall()`, `spawnSkillBall()`, `spawnHazard()` | 每帧 ~N 次 |
| `removeItem()` | `removeDeadEntities()` — 6 个列表遍历 | 每帧 ~N 次 |
| `delete item` | `removeDeadEntities()` | 每帧 ~N 次 |
| `setRect()` | `Entity::setRadius()` → `eat()`, `update()`, `split()` | 每帧 ~N 次 |
| `setPos()` | `Ball::move()`, `applyAttraction()`, magnet logic | 每帧 ~N 次 |

每次 `setPos`/`setRect` 都触发 `prepareGeometryChange()` → BSP 树节点更新。运行数秒后，BSP 树经过数万次增删改操作累积，内部结构失衡导致 **栈溢出** 或 **断言失败** → 崩溃。

### 修复方案

**禁用 Qt 的 BSP 树**，因为本游戏已有自己的 [SpatialGrid](file:///d:/code/project/0.0.1/AgarClone_Qt/SpatialGrid.h) 做空间碰撞检测，Qt 的 BSP 树完全冗余。

在 `GameScene` 构造函数中添加一行：

```cpp
setItemIndexMethod(QGraphicsScene::NoIndex);
```

`NoIndex` 模式下：
- 不建 BSP 树，无增删改开销
- `QGraphicsScene::items()` 类查询变慢（但我们不用）
- 碰撞检测使用自己的 SpatialGrid，不受影响
- 渲染使用 `FullViewportUpdate`，遍历所有 item 绘制，不受影响

---

## 问题 2: 禁用鼠标滚轮

### 修复方案

在 [GameView.h](file:///d:/code/project/0.0.1/AgarClone_Qt/GameView.h) 中声明 `wheelEvent` 重写，在 [GameView.cpp](file:///d:/code/project/0.0.1/AgarClone_Qt/GameView.cpp) 中空实现：

**GameView.h** — 在 `protected` 区域添加：
```cpp
void wheelEvent(QWheelEvent* event) override;
```

**GameView.cpp** — 添加空实现：
```cpp
void GameView::wheelEvent(QWheelEvent* event)
{
    Q_UNUSED(event);
}
```

---

## 问题 3: HUD 文本重构为 foreground

### 当前问题

HUD 使用 6 个 `QGraphicsTextItem` 对象（[UIManager.cpp:L36-L41](file:///d:/code/project/0.0.1/AgarClone_Qt/UIManager.cpp#L36-L41)），存在以下问题：
- 作为 scene item，参与 BSP 树管理
- 位置需用 `mapToScene` 做摄像机坐标转换
- 字体大小需手动 `/zoom` 缩放
- 行高和 margin 也需手动 `/zoom`
- 代码复杂且脆弱

### 修复方案

将 HUD 从 `QGraphicsTextItem` 改为在 **GameScene::drawForeground()** 中用 `QPainter` 直接绘制。

`drawForeground()` 特性：
- 在所有 item 绘制完成后调用
- 使用 **viewport 坐标系**（屏幕坐标），不受摄像机变换影响
- 永远在最上层
- 不需要 scene item，不参与 BSP 树
- 字体固定像素大小，无需缩放计算

### 具体修改

#### 3.1 GameScene.h — 添加 HUD 数据成员 + drawForeground 声明

```cpp
protected:
    void drawForeground(QPainter* painter, const QRectF& rect) override;

public:
    // HUD 数据（由 GameView 设置）
    QString hudScoreText;
    QString hudTimeText;
    QString hudRadiusText;
    QString hudAICountText;
    QString hudEffectsText;
    QString hudSplitText;
```

#### 3.2 GameScene.cpp — 实现 drawForeground()

```cpp
void GameScene::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);
    
    painter->setRenderHint(QPainter::Antialiasing, true);
    QFont font("Arial", 12);
    painter->setFont(font);
    painter->setPen(Qt::white);
    
    const int margin = 10;
    const int lineHeight = 18;
    
    painter->drawText(margin, margin + lineHeight * 0, hudScoreText);
    painter->drawText(margin, margin + lineHeight * 1, hudTimeText);
    painter->drawText(margin, margin + lineHeight * 2, hudRadiusText);
    painter->drawText(margin, margin + lineHeight * 3, hudAICountText);
    painter->drawText(margin, margin + lineHeight * 4, hudEffectsText);
    painter->drawText(margin, margin + lineHeight * 5, hudSplitText);
}
```

#### 3.3 UIManager.h — 简化 updateHUD 签名 + 删除 HUD QGraphicsTextItem 成员

删除：
- `m_hudScore`, `m_hudTime`, `m_hudRadius`, `m_hudAICount`, `m_hudEffects`, `m_hudSplit` (6 个 member)
- `createHUDItems()` 方法声明

修改 `updateHUD` 签名（去掉 `zoom` 参数，返回 HUD 数据给 scene）：

或者更简洁的方案：**UIManager::updateHUD 直接设置 GameScene 的 HUD 文本字段，不再管理 QGraphicsTextItem**。

```cpp
// UIManager.h — 修改
void updateHUD(qreal score, qreal survivalTime, qreal avgRadius,
               int aiCount, const QString& effects, bool canSplit);
```

#### 3.4 UIManager.cpp — 重写 updateHUD，删除 createHUDItems

```cpp
void UIManager::updateHUD(qreal score, qreal survivalTime, qreal avgRadius,
                          int aiCount, const QString& effects, bool canSplit)
{
    // 不再创建/更新 QGraphicsTextItem，直接设置 GameScene 的文本字段
    auto* gs = static_cast<GameScene*>(m_scene);
    
    int minutes = static_cast<int>(survivalTime) / 60;
    int seconds = static_cast<int>(survivalTime) % 60;
    
    gs->hudScoreText   = QString::fromUtf8("分数: %1").arg(score, 0, 'f', 1);
    gs->hudTimeText    = QString::fromUtf8("时间: %1:%2")
        .arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
    gs->hudRadiusText  = QString::fromUtf8("半径: %1").arg(avgRadius, 0, 'f', 1);
    gs->hudAICountText = QString("AI: %1").arg(aiCount);
    gs->hudEffectsText = QString::fromUtf8("效果: ") + effects;
    gs->hudSplitText   = canSplit
        ? QString::fromUtf8("分裂: 可")
        : QString::fromUtf8("分裂: 不可");
}
```

同时需要：
- 删除 `createHUDItems()` 实现
- 构造函数中删除 `createHUDItems()` 调用
- `showMenu()` 中删除 6 行 `hudXxx->setVisible(false)`
- `hideAll()` 中删除 6 行 `hudXxx->setVisible(false)`

#### 3.5 GameView.cpp — 简化 updateHUD 调用

```cpp
// 去掉 zoom 参数
m_uiManager->updateHUD(
    m_gameScene->score,
    m_gameScene->survivalTime,
    avgRadius,
    m_gameScene->aiBalls.size(),
    effects,
    canSplit
);
```

---

## 修改文件清单

| 文件 | 修改内容 |
|------|----------|
| `GameScene.h` | 添加 `drawForeground()` 声明 + 6 个 HUD 文本成员 |
| `GameScene.cpp` | 构造中 `setItemIndexMethod(NoIndex)` + 实现 `drawForeground()` |
| `GameView.h` | 添加 `wheelEvent()` 声明 |
| `GameView.cpp` | 空实现 `wheelEvent()` + 简化 `updateHUD` 调用（去掉 zoom） |
| `UIManager.h` | 删除 6 个 HUD QGraphicsTextItem 成员 + `createHUDItems()` |
| `UIManager.cpp` | 删除 `createHUDItems()` + 重写 `updateHUD()` + 清理 showMenu/hideAll |

---

## 预期效果

1. **不再崩溃** — NoIndex 彻底消除 BSP 树相关的所有崩溃路径
2. **滚轮无效** — `wheelEvent` 空实现吞掉滚轮事件
3. **HUD 简洁** — 6 行 `drawText` 替代 200+ 行 QGraphicsTextItem 管理代码，无需 zoom 计算
4. **性能提升** — 少 6 个 scene item，少 6 次/帧的 setFont/setPlainText/setPos 调用