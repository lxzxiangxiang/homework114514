#pragma once

#include "Entity.h"
#include "Constants.h"

class Hazard : public Entity {
public:
    qreal lifetime = 45.0f;
    DebuffType hazardType = DebuffType::None;

    Hazard();

    EntityType entityType() const override { return EntityType::Hazard; }
    void update(qreal dt) override;
    void onEaten(Entity* eater) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    QRectF boundingRect() const override;
};
