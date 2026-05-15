#pragma once

#include "Entity.h"

class Food : public Entity {
public:
    Food(qreal radius = -1);

    EntityType entityType() const override { return EntityType::Food; }

    void update(qreal dt) override;
    void onEaten(Entity* eater) override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    QRectF boundingRect() const override;
};
