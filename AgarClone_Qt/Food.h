#pragma once

#include "Entity.h"

class Food : public Entity {
public:
    Q_DISABLE_COPY_MOVE(Food)
    Food(qreal radius = -1);

    void update(qreal dt) override;
    void onEaten(Entity* eater) override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    QRectF boundingRect() const override;
};
