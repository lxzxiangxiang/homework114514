# Ball.cpp — 球体(Ball)实体实现

## 文件概述
Ball 是游戏中最核心的实体类，玩家球体和 AI 球体共用此类。通过 `isPlayer` 标志区分。支持移动、分裂、吐孢、吞食，以及技能/负面效果系统。

## 涉及类
- `Ball : public Entity` — 球体实体类

## 方法说明

### `Ball::Ball(qreal radius, QColor color, bool isPlayer, int aiLevel)`
构造函数。初始化玩家/AI 标志、AI 等级、分裂/合并/无敌等计时器。

### `void Ball::move(qreal dx, qreal dy, qreal dt)`
移动球体。

**算法：**
- **速度公式**：`speed = BASE_SPEED × sqrt(MIN_RADIUS / radius)` — 半径越大速度越慢
- 更新 vx/vy 并记录方向(laxDx/lastDy)
- 位置 clamp 到地图边界 [0, MAP_WIDTH] × [0, MAP_HEIGHT]

### `void Ball::split(QPointF direction)`
分裂为两个球体。半径 ≥ 18 时可用。新球半径 = 原半径 / √2，朝移动方向弹出。**预留待实现。**

### `void Ball::eject()`
吐出孢子（半径 8），自身半径减少 3。**预留待实现。**

### `void Ball::eat(Entity* target)`
吞食目标实体。**预留待实现。**

### `void Ball::applySkill(SkillType skillType)`
应用技能效果。根据枚举设置 `skill` 和对应持续时间的 `skillTimer`。

### `void Ball::applyDebuff(DebuffType debuffType)`
应用负面效果。**预留待实现。**

### `bool Ball::isInvincible() const`
无敌判定：`invincibleTimer > 0`

### `bool Ball::hasShield() const`
护盾判定：`skill == Shield && skillTimer > 0`

### `void Ball::update(qreal dt)`
每帧递减所有计时器：
- splitTimer、mergeTimer、invincibleTimer
- skillTimer → 到期清空 skill
- debuffTimer → 到期清空 debuff
- vx/vy 衰减（×0.95）

### `void Ball::onEaten(Entity* eater)`
被吞食时标记死亡。

### `void Ball::paint(QPainter*, ...)`
绘制球体及各种特效：

| 状态 | 视觉表现 |
|------|----------|
| 护盾 (Shield) | 蓝色半透明外圈 (+4px) |
| 无敌 (Invincible) | 闪烁（alpha 在 0.4/1.0 间交替） |
| 隐身 (Invisible) | alpha = 0.3 |
| 加速 (Speed) | 颜色高亮 130% |
| 磁力 (Magnet) | 颜色高亮 150% |
| 中毒 (Poison) | 绿色调 |
| 陷阱 (Trap) | 棕色半透明边框 (+2px) |

### `QRectF Ball::boundingRect() const`
护盾时额外 +4 像素半径。
