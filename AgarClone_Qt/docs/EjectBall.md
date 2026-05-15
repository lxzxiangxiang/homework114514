# EjectBall.cpp — 抛射物(EjectBall)实体实现

## 文件概述
抛射物是球体通过「吐孢」(E 键) 操作射出的实体。以初速度飞出，逐渐减速，存在约 20 秒后消失。可被任何球体吞食。

## 涉及类
- `EjectBall : public Entity` — 抛射物实体类

## 方法说明

### `EjectBall::EjectBall(QPointF pos, QColor color, qreal dx, qreal dy)`
构造函数。

| 参数 | 说明 |
|------|------|
| pos | 初始位置（吐孢者位置） |
| color | 颜色（与吐孢者同色） |
| dx, dy | 飞行方向向量（已归一化） |

**算法：**
- 半径固定为 8（`GameConstants::EJECTBALL_RADIUS`）
- 初速度 = 方向向量 × 8（像素/帧）

### `void EjectBall::update(qreal dt)`
每帧更新飞行状态。

**算法：**
1. `lifetime -= dt` — 递减生命周期（初始 20 秒）
2. `setPos(pos() + QPointF(vx*dt, vy*dt))` — 按速度移动
3. `vx *= 0.98; vy *= 0.98` — 速度每帧衰减 2%（指数衰减）
4. 若 `lifetime <= 0`，标记死亡

### `void EjectBall::onEaten(Entity* eater)`
被球体吞食时标记死亡。

### `void EjectBall::paint(QPainter*, ...)`
用 `m_color` 绘制实心圆。

### `QRectF EjectBall::boundingRect() const`
返回以中心为原点的 `[-r, -r, 2r, 2r]` 边界矩形。
