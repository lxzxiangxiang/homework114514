#pragma once

#include "Entity.h"
#include "Constants.h"

class Ball : public Entity {
public:
    Ball(qreal radius, QColor color, bool isPlayer, int aiLevel = 0);
    ~Ball() = default;

    EntityType entityType() const override { return EntityType::Ball; }

    void move(qreal dx, qreal dy, qreal dt);
    Ball* split(QPointF direction);
    class EjectBall* eject();
    void eat(Entity* target);
    void applySkill(SkillType skill);
    void applyDebuff(DebuffType debuff);
    bool isInvincible() const;
    bool hasShield() const;

    void update(qreal dt) override;
    void onEaten(Entity* eater) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    QRectF boundingRect() const override;

    qreal vx = 0, vy = 0;
    bool isPlayer;
    int aiLevel;
    int aiId;
    qreal splitTimer = 0;
    qreal mergeTimer = 0;
    qreal invincibleTimer = 0;
    SkillType skill = SkillType::None;
    qreal skillTimer = 0;
    DebuffType debuff = DebuffType::None;
    qreal debuffTimer = 0;
    qreal lastDx = 0, lastDy = 0;

    Ball* pendingSplitBall = nullptr;
};
