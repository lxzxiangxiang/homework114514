#include "SpatialGrid.h"
#include "Entity.h"

#include <QtMath>

void SpatialGrid::clear()
{
    m_grid.clear();
}

void SpatialGrid::add(Entity* entity)
{
    int cx = static_cast<int>(entity->pos().x()) / CELL_SIZE;
    int cy = static_cast<int>(entity->pos().y()) / CELL_SIZE;
    m_grid[{cx, cy}].append(entity);
}

void SpatialGrid::remove(Entity* entity)
{
    int cx = static_cast<int>(entity->pos().x()) / CELL_SIZE;
    int cy = static_cast<int>(entity->pos().y()) / CELL_SIZE;
    std::pair<int, int> key = {cx, cy};
    auto it = m_grid.find(key);
    if (it != m_grid.end()) {
        it.value().removeAll(entity);
        if (it.value().isEmpty()) {
            m_grid.erase(it);
        }
    }
}

QList<Entity*> SpatialGrid::nearbyEntities(Entity* entity) const
{
    return nearbyEntities(entity->pos().x(), entity->pos().y(), entity->radius());
}

QList<Entity*> SpatialGrid::nearbyEntities(qreal x, qreal y, qreal radius) const
{
    QList<Entity*> result;

    int cx = static_cast<int>(x) / CELL_SIZE;
    int cy = static_cast<int>(y) / CELL_SIZE;

    qreal searchRadius = std::max(radius * 2, static_cast<qreal>(CELL_SIZE) * 1.5);
    qreal searchRadiusSq = searchRadius * searchRadius;

    int range = static_cast<int>(std::ceil(searchRadius / CELL_SIZE));

    for (int dx = -range; dx <= range; ++dx) {
        for (int dy = -range; dy <= range; ++dy) {
            std::pair<int, int> key = {cx + dx, cy + dy};
            auto it = m_grid.constFind(key);
            if (it != m_grid.constEnd()) {
                for (Entity* other : it.value()) {
                    if (!other->isAlive()) {
                        continue;
                    }
                    qreal dx = other->pos().x() - x;
                    qreal dy = other->pos().y() - y;
                    qreal distSq = dx * dx + dy * dy;
                    if (distSq <= searchRadiusSq) {
                        result.append(other);
                    }
                }
            }
        }
    }

    return result;
}
