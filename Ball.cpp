// 球体(Ball)实体实现 — 游戏中最核心的实体，玩家和 AI 共用此类
// 支持移动(速度与半径反比)、分裂、吐孢、吞食，以及技能/负面效果系统
#include "Ball.h"
#include "EjectBall.h"
#include <QPainter>
#include <cmath>

Ball::Ball(qreal radius, QColor color, bool isPlayer, int aiLevel)
    : Entity(radius, color)
    , isPlayer(isPlayer)
    , aiLevel(aiLevel)
    , aiId(0)
{
    setAcceptHoverEvents(true);
}

// 移动球体：速度与半径成反比，位置 clamp 到地图边界
// 速度公式: speed = BASE_SPEED × sqrt(MIN_RADIUS / radius)
void Ball::move(qreal dx, qreal dy, qreal dt)
{
    qreal speed = GameConstants::BASE_SPEED * std::sqrt(GameConstants::MIN_RADIUS / m_radius);
    if (debuff == DebuffType::Trap && debuffTimer > 0) {
        speed *= GameConstants::HazardEffect::TRAP_SPEED_MULTIPLIER;
    }
    vx = dx * speed;
    vy = dy * speed;
    lastDx = dx;
    lastDy = dy;

    setPos(pos() + QPointF(vx * dt, vy * dt));

    // 边界限制，防止球体移出地图
    qreal x = pos().x();
    qreal y = pos().y();
    if (x < 0) x = 0;
    if (x > GameConstants::MAP_WIDTH) x = GameConstants::MAP_WIDTH;
    if (y < 0) y = 0;
    if (y > GameConstants::MAP_HEIGHT) y = GameConstants::MAP_HEIGHT;
    setPos(x, y);
}

// 分裂：半径 ≥ 18 时，朝移动方向分裂为两个球体
// 新球半径 = 原半径 / √2，自身半径同步缩小，返回新球指针供 GameScene 注册
Ball* Ball::split(QPointF direction)
{
    if (m_radius < GameConstants::SPLIT_THRESHOLD || splitTimer > 0)
        return nullptr;

    qreal newRadius = m_radius / std::sqrt(2.0);
    setRadius(newRadius);

    auto* newBall = new Ball(newRadius, m_color, isPlayer, aiLevel);
    newBall->aiId = aiId;
    newBall->splitTimer = 1.5f;
    newBall->mergeTimer = 1.5f;
    newBall->invincibleTimer = 3.0f;

    QPointF offset = direction * (newRadius + newRadius * 0.1);
    newBall->setPos(pos() + offset);

    splitTimer = 1.5f;
    mergeTimer = 1.5f;

    return newBall;
}

// 吐孢：半径 ≥ 25 时，吐出一个孢子（半径 8），自身半径减少 3
EjectBall* Ball::eject()
{
    if (m_radius < GameConstants::EJECT_THRESHOLD)
        return nullptr;

    setRadius(m_radius - 3);

    QPointF dir(lastDx, lastDy);
    if (std::abs(lastDx) < 1e-6 && std::abs(lastDy) < 1e-6) {
        dir = QPointF(1, 0);
    }
    return new EjectBall(pos(), m_color, dir.x(), dir.y());
}

// 吞食目标实体：质量合并，新半径 = √(自身半径² + 目标半径²)
void Ball::eat(Entity* target)
{
    qreal newRadius = std::sqrt(m_radius * m_radius + target->radius() * target->radius());
    setRadius(newRadius);
    target->onEaten(this);
}

// 应用技能效果：设置技能类型和对应持续时间的计时器
void Ball::applySkill(SkillType skillType)
{
    skill = skillType;
    switch (skillType) {
    case SkillType::Speed:
        skillTimer = GameConstants::SkillDuration::SPEED;
        break;
    case SkillType::Shield:
        skillTimer = GameConstants::SkillDuration::SHIELD;
        break;
    case SkillType::Grow:
        skillTimer = GameConstants::SkillDuration::GROW;
        break;
    case SkillType::Invisible:
        skillTimer = GameConstants::SkillDuration::INVISIBLE;
        break;
    case SkillType::Magnet:
        skillTimer = GameConstants::SkillDuration::MAGNET;
        break;
    default:
        skillTimer = 0;
        break;
    }
}

// 应用负面效果：Bomb 瞬间减半径，Trap 减速，Poison 持续掉血
void Ball::applyDebuff(DebuffType debuffType)
{
    debuff = debuffType;
    switch (debuffType) {
    case DebuffType::Bomb:
        setRadius(m_radius * GameConstants::HazardEffect::BOMB_RADIUS_RATIO);
        debuffTimer = 0;
        break;
    case DebuffType::Trap:
        debuffTimer = GameConstants::HazardEffect::TRAP_DURATION;
        break;
    case DebuffType::Poison:
        debuffTimer = GameConstants::HazardEffect::POISON_DURATION;
        break;
    default:
        debuffTimer = 0;
        break;
    }
}

// 无敌判定：新生成球体前 3 秒无敌
bool Ball::isInvincible() const
{
    return invincibleTimer > 0;
}

// 护盾判定：技能为 Shield 且计时器未耗尽
bool Ball::hasShield() const
{
    return skill == SkillType::Shield && skillTimer > 0;
}

// 每帧递减所有计时器：分裂、合并、无敌、技能、减益
// 速度自然衰减（摩擦效果）
void Ball::update(qreal dt)
{
    if (splitTimer > 0) splitTimer -= dt;
    if (mergeTimer > 0) mergeTimer -= dt;
    if (invincibleTimer > 0) invincibleTimer -= dt;

    // 技能计时器递减，到期清除技能
    if (skillTimer > 0) {
        skillTimer -= dt;
        if (skillTimer <= 0) skill = SkillType::None;
    }

    // 减益计时器递减，到期清除减益
    if (debuffTimer > 0) {
        debuffTimer -= dt;
        if (debuff == DebuffType::Poison) {
            setRadius(m_radius - GameConstants::HazardEffect::POISON_RADIUS_PER_SEC * dt);
        }
        if (debuffTimer <= 0) debuff = DebuffType::None;
    }

    // 速度摩擦衰减
    vx *= 0.95f;
    vy *= 0.95f;
}

// 被吞食时标记死亡
void Ball::onEaten(Entity* eater)
{
    Q_UNUSED(eater);
    m_alive = false;
}

// 绘制球体及各种状态特效
void Ball::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setPen(Qt::NoPen);

    // 护盾效果：蓝色半透明外圈（+4px）
    if (hasShield()) {
        QColor shieldColor(100, 150, 255, 150);
        painter->setBrush(shieldColor);
        painter->drawEllipse(QPointF(0, 0), m_radius + 4, m_radius + 4);
    }

    QColor drawColor = m_color;
    qreal alpha = 1.0;

    // 无敌闪烁效果：alpha 在 0.4 和 1.0 之间交替
    if (isInvincible()) {
        int flash = static_cast<int>(invincibleTimer * 10) % 2;
        alpha = flash ? 0.4 : 1.0;
    }

    // 隐身效果：半透明
    if (skill == SkillType::Invisible && skillTimer > 0) {
        alpha = 0.3;
    }

    // 加速效果：颜色高亮 130%
    if (skill == SkillType::Speed && skillTimer > 0) {
        drawColor = drawColor.lighter(130);
    }

    // 磁力效果：颜色高亮 150%
    if (skill == SkillType::Magnet && skillTimer > 0) {
        drawColor = drawColor.lighter(150);
    }

    // 中毒效果：绿色调
    if (debuff == DebuffType::Poison && debuffTimer > 0) {
        drawColor = QColor(100, 200, 50);
    }

    // 陷阱效果：棕色半透明边框（+2px）
    if (debuff == DebuffType::Trap && debuffTimer > 0) {
        painter->setBrush(QColor(139, 69, 19, 120));
        painter->drawEllipse(QPointF(0, 0), m_radius + 2, m_radius + 2);
    }

    drawColor.setAlphaF(alpha);
    painter->setBrush(drawColor);
    painter->drawEllipse(QPointF(0, 0), m_radius, m_radius);
}

// 边界矩形：护盾时额外 +4 像素以容纳外圈
QRectF Ball::boundingRect() const
{
    qreal extra = hasShield() ? 4 : 0;
    qreal r = m_radius + extra;
    return QRectF(-r, -r, r * 2, r * 2);
}
