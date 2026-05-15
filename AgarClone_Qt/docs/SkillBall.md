# SkillBall.cpp — 技能球(SkillBall)实体实现

## 文件概述
技能球是散布在地图上的增益实体。球体触碰后获得对应技能效果。每约 5 秒概率生成，最多 8 个，存在 30 秒后自动消失。

## 涉及类
- `SkillBall : public Entity` — 技能球实体类

## 技能类型（SkillType 枚举）
| 技能 | 颜色建议 | 效果 | 持续 |
|------|----------|------|------|
| Speed (加速) | 黄色 | 移动速度 ×1.5 | 5 秒 |
| Shield (护盾) | 青色 | 免疫吞食与危险物 | 3 秒 |
| Grow (巨化) | 紫色 | 半径 ×1.3 | 4 秒 |
| Invisible (隐身) | 灰色 | 敌人无法看到/攻击 | 6 秒 |
| Magnet (磁力) | 橙色 | 自动吸引附近豆子 | 5 秒 |

## 方法说明

### `SkillBall::SkillBall()`
构造函数。

**算法：**
- 半径固定为 12（`GameConstants::SKILLBALL_RADIUS`）
- 颜色使用 HSV 模式：色调随机 0~359，饱和度 200，明度 255（鲜艳色彩）
- 从 5 种技能中随机选取一种：`bounded(5)` 返回 0~4

### `void SkillBall::update(qreal dt)`
每帧递减生命周期（初始 30 秒）。到期后标记死亡。

### `void SkillBall::onEaten(Entity* eater)`
被球体触碰时标记死亡。实际技能赋予由 `GameScene::checkCollisions()` 处理。

### `void SkillBall::paint(QPainter*, ...)`
绘制脉冲光晕效果。

**算法：**
1. `pulse = 1.0 + 0.15 × sin(lifetime × 3.0)` — 使用正弦波产生呼吸式脉冲
2. 外层：半透明光晕圆（alpha=60），半径 = `radius + 3 × pulse`
3. 内层：实心圆，半径 = `radius`

### `QRectF SkillBall::boundingRect() const`
返回 `[-r-3, -r-3, 2(r+3), 2(r+3)]` — 比实际半径大 3 像素以容纳光晕。
