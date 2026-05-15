#pragma once

#include "Entity.h"

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

class EjectBall : public Entity {
public:
    qreal vx = 0;
    qreal vy = 0;
    qreal lifetime = 20.0f;

    EjectBall(QPointF pos, QColor color, qreal dx, qreal dy);

    EntityType entityType() const override { return EntityType::EjectBall; }

    void update(qreal dt) override;
    void onEaten(Entity* eater) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    QRectF boundingRect() const override;
};
