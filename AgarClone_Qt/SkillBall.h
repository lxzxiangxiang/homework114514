#pragma once

#include "Entity.h"
#include "Constants.h"

class SkillBall : public Entity {
public:
    qreal lifetime = 30.0f;
    SkillType skillType = SkillType::None;

    SkillBall();

    EntityType entityType() const override { return EntityType::SkillBall; }

    void update(qreal dt) override;
    void onEaten(Entity* eater) override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    QRectF boundingRect() const override;
};
