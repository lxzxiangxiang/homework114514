#pragma once

#include <QHash>
#include <QVector>
#include <QList>
#include <utility>

class Entity;
class Ball;

class SpatialGrid {
public:
    static constexpr int CELL_SIZE = 200;

    void clear();
    void add(Entity* entity);
    void remove(Entity* entity);
    QList<Entity*> getNearby(Entity* entity) const;
    QList<Entity*> getNearby(qreal x, qreal y, qreal radius) const;

private:
    QHash<std::pair<int, int>, QVector<Entity*>> m_grid;
};
