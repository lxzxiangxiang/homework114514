#pragma once

#include <QList>

#include "SpatialGrid.h"

class Ball;
class Food;
class EffectBall;

class CollisionSystem {
public:
    Q_DISABLE_COPY_MOVE(CollisionSystem)

    CollisionSystem(SpatialGrid& spatialGrid,
                    QList<Food*>& foods, QList<EffectBall*>& effectBalls,
                    qreal& score);

    void checkCollisions(const QList<Ball*>& allBalls);
    void applyAttraction(const QList<Ball*>& allBalls, qreal dt);

    static bool sameOwner(const Ball* a, const Ball* b);

private:
    SpatialGrid& m_spatialGrid;
    SpatialGrid m_foodGrid;
    SpatialGrid m_effectGrid;
    QList<Food*>& m_foods;
    QList<EffectBall*>& m_effectBalls;
    qreal& m_score;
};