#pragma once

#include <QGraphicsEllipseItem>
#include <QColor>
#include <QPen>
#include <QBrush>

#include "Constants.h"

class Entity : public QGraphicsEllipseItem {
public:
    Q_DISABLE_COPY_MOVE(Entity)

    Entity(qreal mass, QColor color, QGraphicsItem* parent = nullptr)
        : QGraphicsEllipseItem(parent)
        , m_mass(mass)
        , m_color(color)
        , m_alive(true)
    {
        qreal r = radius();
        setRect(-r, -r, r * 2, r * 2);
        setBrush(QBrush(m_color));
        setPen(QPen(Qt::NoPen));
    }

    virtual ~Entity() = default;

    qreal radius() const { return std::sqrt(m_mass / M_PI); }
    qreal mass() const { return m_mass; }
    void setMass(qreal mass) {
        Q_ASSERT(mass >= 0);
        if (mass < 0) mass = 0;
        m_mass = mass;
        qreal r = radius();
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
    virtual void onEaten(Entity* eater) { Q_UNUSED(eater); }

protected:
    qreal m_mass;
    QColor m_color;
    bool m_alive;
};
