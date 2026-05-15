// 抛射物(EjectBall)实体实现 — 球体吐孢射出的孢子
// 初速度 8 像素/帧，逐渐减速(每帧 ×0.98)，存在 20 秒后消失
#include "EjectBall.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <cmath>

EjectBall::EjectBall(QPointF pos, QColor color, qreal dx, qreal dy)
    : Entity(8.0, color)    // 半径固定为 8，颜色与吐孢者相同
{
    setPos(pos);
    // 初速度 = 方向向量 × 8 像素/帧
    vx = dx * 8.0;
    vy = dy * 8.0;
}

// 每帧更新：递减生命周期、移动位置、速度衰减
void EjectBall::update(qreal dt)
{
    lifetime -= dt;                                         // 递减生命周期（初始 20 秒）
    setPos(pos() + QPointF(vx * dt, vy * dt));              // 按速度移动
    vx *= 0.98;                                             // 每帧速度衰减 2%
    vy *= 0.98;
    if (lifetime <= 0) {
        m_alive = false;                                    // 生命周期耗尽，标记死亡
    }
}

// 被任何球体吞食时标记死亡
void EjectBall::onEaten(Entity* eater)
{
    Q_UNUSED(eater);
    m_alive = false;
}

// 绘制实心圆
void EjectBall::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setBrush(m_color);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(boundingRect());
}

QRectF EjectBall::boundingRect() const
{
    return QRectF(-m_radius, -m_radius, m_radius * 2, m_radius * 2);
}
