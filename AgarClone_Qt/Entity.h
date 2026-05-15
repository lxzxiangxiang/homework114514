#pragma once

#include <QGraphicsEllipseItem>
#include <QColor>
#include <QPointF>
#include <QPen>
#include <QBrush>

enum class EntityType {
    Ball,
    Food,
    SkillBall,
    Hazard,
    EjectBall
};

class Entity : public QGraphicsEllipseItem {
public:
    Entity(qreal radius, QColor color, QGraphicsItem* parent = nullptr)
        : QGraphicsEllipseItem(parent)
        , m_radius(radius)
        , m_color(color)
        , m_alive(true)
    {
        setRect(-radius, -radius, radius * 2, radius * 2);
        setBrush(QBrush(m_color));
        setPen(QPen(Qt::NoPen));
    }

    virtual ~Entity() = default;

    qreal radius() const { return m_radius; }
    void setRadius(qreal r) {
        m_radius = r;
        setRect(-r, -r, r * 2, r * 2);
    }
    QColor color() const { return m_color; }
    void setColor(const QColor& c) {
        m_color = c;
        setBrush(QBrush(m_color));
    }
    bool isAlive() const { return m_alive; }
    void setAlive(bool alive) { m_alive = alive; }

    virtual void update(qreal dt) { Q_UNUSED(dt); }
    virtual void onEaten(class Entity* eater) { Q_UNUSED(eater); }

    virtual EntityType entityType() const = 0;

protected:
    qreal m_radius;
    QColor m_color;
    bool m_alive;
};
