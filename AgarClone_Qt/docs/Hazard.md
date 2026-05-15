# Hazard.cpp — 危险物(Hazard)实体实现

## 文件概述
危险物是散布在地图上的减益实体。球体触碰后（无护盾时）受到对应负面效果。每约 15 秒概率生成，最多 8 个，存在 45 秒后自动消失。

## 涉及类
- `Hazard : public Entity` — 危险物实体类

## 危险物类型（DebuffType 枚举）
| 类型 | 效果 | 持续时间 | 颜色 | 图标 |
|------|------|----------|------|------|
| Bomb (炸弹) | 半径瞬间减少 15% | 瞬时 | 红色 | 圆 + X |
| Trap (陷阱) | 移动速度 ×0.5 | 3 秒 | 棕色 | 六角尖刺 |
| Poison (毒雾) | 每秒减少半径 2 | 4 秒 | 深绿 | 骷髅头 |

## 方法说明

### `Hazard::Hazard()`
构造函数。

**算法：**
- 半径固定为 50（`GameConstants::HAZARD_RADIUS`）
- 随机选择 Bomb/Trap/Poison 三种类型
- 颜色按类型设定：红色(炸弹)、棕色(陷阱)、深绿(毒雾)

### `void Hazard::update(qreal dt)`
每帧递减生命周期（初始 45 秒）。到期后标记死亡。

### `void Hazard::onEaten(Entity* eater)`
被球体触碰时标记死亡。实际减益效果由 `GameScene::checkCollisions()` 处理。

### `void Hazard::paint(QPainter*, ...)`
按危险物类型绘制不同图标：

- **Bomb**：红色圆 + 黑色十字交叉线
- **Trap**：六角星形尖刺（内外交替半径，`QPainterPath` 多边形）
- **Poison**：骷髅头状贝塞尔曲线轮廓（`cubicTo`）+ 眼睛和鼻子的白色椭圆

### `QRectF Hazard::boundingRect() const`
返回以中心为原点的 `[-50, -50, 100, 100]` 边界矩形。
