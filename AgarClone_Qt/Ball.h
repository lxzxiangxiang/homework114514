#pragma once

#include "Entity.h"
#include <QVector>

class Ball : public Entity {
public:
    Q_DISABLE_COPY_MOVE(Ball)
    struct ActiveEffect {
        EffectType type;
        qreal timer;
        qreal growOriginalMass = 0;
    };

    struct SplitAnim {
        bool active = false;
        qreal progress = 0.0;
        qreal duration = 0.25;
        QPointF startPos;
        QPointF targetPos;
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
    bool isInSplitAnim() const { return m_splitAnim.active; }
    QPointF splitTargetPos() const { return m_splitAnim.targetPos; }
    qreal mergeTimer() const { return m_mergeTimer; }

    void update(qreal dt) override;
    void onEaten(Entity* eater) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    QRectF boundingRect() const override;

    bool m_isPlayer;
    int m_aiLevel;
    int m_aiId;

    qreal m_lastDx = 0, m_lastDy = 0;
    qreal m_shieldFlash[2] = {0, 0};

    Ball* m_pendingSplitBall = nullptr;

private:
    void removeEffect(EffectType t);
    QVector<ActiveEffect> m_effects;
    uint16_t m_effectFlags = 0;
    SplitAnim m_splitAnim;
    qreal m_mergeTimer = 0;
};
