#pragma once

#include "Entity.h"

class EffectBall : public Entity {
public:
    EffectBall(QPointF pos, EffectType type);

    EffectType effectType() const { return m_type; }
    void update(qreal dt) override;
    void onEaten(Entity* eater) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    QRectF boundingRect() const override;

private:
    void drawIcon(QPainter* painter, qreal x, qreal y, qreal r);

    EffectType m_type;
    qreal m_pulseTimer = 0;
    qreal m_lifetime = 0;
    qreal m_maxLifetime = 30;
    qreal m_rotation = 0;
    qreal m_moveSpeed = 0;
    qreal m_moveAngle = 0;
    qreal m_flashTimer = 0;
};
