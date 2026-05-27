#pragma once

#include "Entity.h"

class Ball : public Entity {
public:
    Ball(qreal mass, QColor color, bool isPlayer, int aiLevel = 0);

    void move(qreal dx, qreal dy, qreal dt);
    qreal speed() const;
    Ball* split(QPointF direction);
    class EjectBall* eject();
    void eat(Entity* target);
    void applyEffect(EffectType effect, const QList<Ball*>& allBalls);
    bool hasShield() const;

    void update(qreal dt) override;
    void onEaten(Entity* eater) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    QRectF boundingRect() const override;

    bool isPlayer;
    int aiLevel;
    int aiId;

    EffectType effect = EffectType::None;
    qreal effectTimer = 0;

    qreal lastDx = 0, lastDy = 0;
    qreal growOriginalMass = 0;

    Ball* pendingSplitBall = nullptr;
};
