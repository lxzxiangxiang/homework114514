// 空间网格(SpatialGrid)实现 — 碰撞检测优化
// 地图划分为 200×200 像素单元格，碰撞检测只查询 3×3 邻域，将 O(N²) 降为 O(N)
#include "SpatialGrid.h"
#include "Entity.h"

#include <QtMath>

// 清空整个网格，通常在每帧碰撞检测前调用
void SpatialGrid::clear()
{
    m_grid.clear();
}

// 将实体注册到网格：根据坐标计算单元格索引并存入对应 cell
void SpatialGrid::add(Entity* entity)
{
    int cx = static_cast<int>(entity->pos().x()) / CELL_SIZE;
    int cy = static_cast<int>(entity->pos().y()) / CELL_SIZE;
    m_grid[{cx, cy}].append(entity);
}

// 从网格中移除实体，若 cell 为空则删除 hash 条目以节省内存
void SpatialGrid::remove(Entity* entity)
{
    int cx = static_cast<int>(entity->pos().x()) / CELL_SIZE;
    int cy = static_cast<int>(entity->pos().y()) / CELL_SIZE;
    QPair<int, int> key = {cx, cy};
    auto it = m_grid.find(key);
    if (it != m_grid.end()) {
        it.value().removeAll(entity);
        if (it.value().isEmpty()) {
            m_grid.erase(it);
        }
    }
}

// 基于实体对象的查询重载，委托给坐标版本
QList<Entity*> SpatialGrid::getNearby(Entity* entity) const
{
    return getNearby(entity->pos().x(), entity->pos().y(), entity->radius());
}

// 查询指定位置周围半径 ×2 范围内的所有存活实体
// 算法：遍历中心 cell 及周围 3×3 邻域，用平方距离过滤
QList<Entity*> SpatialGrid::getNearby(qreal x, qreal y, qreal radius) const
{
    QList<Entity*> result;

    // 计算中心单元格索引
    int cx = static_cast<int>(x) / CELL_SIZE;
    int cy = static_cast<int>(y) / CELL_SIZE;

    // 搜索范围为半径 ×2，使用平方距离避免开方运算
    qreal searchRadius = radius * 2;
    qreal searchRadiusSq = searchRadius * searchRadius;

    // 遍历 3×3 邻域（共 9 个 cell）
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            QPair<int, int> key = {cx + dx, cy + dy};
            auto it = m_grid.constFind(key);
            if (it != m_grid.constEnd()) {
                for (Entity* other : it.value()) {
                    if (!other->isAlive()) {
                        continue;                           // 跳过已死亡实体
                    }
                    qreal dx = other->pos().x() - x;
                    qreal dy = other->pos().y() - y;
                    qreal distSq = dx * dx + dy * dy;       // 平方距离，避免 sqrt
                    if (distSq <= searchRadiusSq) {
                        result.append(other);
                    }
                }
            }
        }
    }

    return result;
}
