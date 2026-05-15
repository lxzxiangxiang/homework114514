# Food.cpp — 豆子(Food)实体实现

## 文件概述
豆子是地图上散布的可吞食静态实体，继承自 `Entity`（即 `QGraphicsEllipseItem`）。被球体吞食后标记死亡并在地图随机位置重生。

## 涉及类
- `Food : public Entity` — 豆子实体类

## 方法说明

### `Food::Food(qreal radius = -1)`
构造函数。若传入 radius < 0（默认），则随机生成 3~8 之间的半径值；同时随机生成 RGB 颜色。

| 参数 | 说明 |
|------|------|
| radius | 豆子半径，负数表示随机生成 (3~8) |

**算法：**
- 半径：`QRandomGenerator::global()->bounded(3, 9)` — 返回 [3, 9) 的整数，即 3~8
- 颜色：RGB 各通道独立随机 0~255

### `void Food::update(qreal dt)`
空实现，豆子为静态实体，位置和状态不随时间变化。

### `void Food::onEaten(Entity* eater)`
被球体吞食时的回调。将 `m_alive` 设为 `false`，由 `GameScene::removeDeadEntities()` 在帧末统一移除。

### `void Food::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)`
绘制豆子外观：用 `m_color` 绘制实心圆。

### `QRectF Food::boundingRect() const`
返回以实体中心为原点的边界矩形，大小为 `2×radius`。
