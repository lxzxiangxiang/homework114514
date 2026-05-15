// 技能球(SkillBall)实体实现 — 散布在地图上的增益实体
// 球体触碰后获得对应技能，每约 5 秒概率生成，最多 8 个，存在 30 秒后消失
#include "SkillBall.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>

SkillBall::SkillBall()
    : Entity(GameConstants::SKILLBALL_RADIUS,
             // 使用 HSV 模式生成鲜艳色彩：色调随机 0~359
             QColor::fromHsv(QRandomGenerator::global()->bounded(360), 200, 255))
    , lifetime(30.0f)
{
    // 从 5 种技能中随机选取一种
    int typeIndex = QRandomGenerator::global()->bounded(5);
    switch (typeIndex) {
    case 0: skillType = SkillType::Speed;     break;
    case 1: skillType = SkillType::Shield;    break;
    case 2: skillType = SkillType::Grow;      break;
    case 3: skillType = SkillType::Invisible; break;
    case 4: skillType = SkillType::Magnet;    break;
    default: skillType = SkillType::None;     break;
    }
}

// 每帧递减生命周期，到期标记死亡
void SkillBall::update(qreal dt)
{
    lifetime -= dt;
    if (lifetime <= 0.0f) {
        setAlive(false);
    }
}

// 被球体触碰后标记死亡，实际技能赋予由 GameScene::checkCollisions() 处理
void SkillBall::onEaten(Entity* eater)
{
    Q_UNUSED(eater);
    setAlive(false);
}

// 绘制脉冲光晕效果：外层半透明圆 + 内层实心圆
void SkillBall::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);

    // 使用正弦波产生呼吸式脉冲效果，pulse 范围 [0.85, 1.15]
    qreal pulse = 1.0f + 0.15f * qSin(lifetime * 3.0f);
    qreal glowRadius = m_radius + 3.0f * pulse;

    // 外层：半透明光晕圆（alpha=60）
    QColor glowColor = m_color;
    glowColor.setAlpha(60);
    painter->setBrush(glowColor);
    painter->drawEllipse(QPointF(0, 0), glowRadius, glowRadius);

    // 内层：实心圆
    painter->setBrush(m_color);
    painter->drawEllipse(QPointF(0, 0), m_radius, m_radius);
}

// 边界矩形比实际半径大 3 像素，为光晕效果留出空间
QRectF SkillBall::boundingRect() const
{
    qreal s = m_radius + 3.0f;
    return QRectF(-s, -s, s * 2.0f, s * 2.0f);
}
