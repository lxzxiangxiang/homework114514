#pragma once

#include "Entity.h"
#include <QVector>

class Ball : public Entity {
public:
    struct ActiveEffect {
        EffectType type;
        qreal timer;
        qreal growOriginalMass = 0;
    };

    Ball(qreal mass, QColor color, bool isPlayer, int aiLevel = 0);

    void move(qreal dx, qreal dy, qreal dt);
    qreal speed() const;
    Ball* split(QPointF direction);
    void eat(Entity* target);
    void applyEffect(EffectType effect, const QList<Ball*>& allBalls);
    void addInitialEffect(EffectType t, qreal timer);
    bool hasShield() const;
    bool hasEffect(EffectType t) const;
    const QVector<ActiveEffect>& effects() const { return m_effects; }

    void update(qreal dt) override;
    void onEaten(Entity* eater) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    QRectF boundingRect() const override;

    bool isPlayer;
    int aiLevel;
    int aiId;

    qreal lastDx = 0, lastDy = 0;

    Ball* pendingSplitBall = nullptr;

private:
    void removeEffect(EffectType t);
    QVector<ActiveEffect> m_effects;
};
