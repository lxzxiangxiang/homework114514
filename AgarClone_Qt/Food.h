#pragma once

#include "ResBall.h"

class Food : public ResBall {
public:
    Food(qreal radius = -1);

    void update(qreal dt) override;
    void onEaten(Entity* eater) override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    QRectF boundingRect() const override;
};
