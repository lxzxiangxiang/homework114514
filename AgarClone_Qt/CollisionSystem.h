#pragma once

#include <QList>
#include <QGraphicsScene>

#include "SpatialGrid.h"

class Ball;
class Food;
class EffectBall;

class CollisionSystem {
public:
    CollisionSystem(QGraphicsScene* scene, SpatialGrid& spatialGrid,
                    QList<Ball*>& playerBalls, QList<Ball*>& aiBalls,
                    QList<Food*>& foods, QList<EffectBall*>& effectBalls,
                    qreal& score);

    void checkCollisions(const QList<Ball*>& allBalls);
    void applyAttraction(const QList<Ball*>& allBalls, qreal dt);

    static bool sameOwner(const Ball* a, const Ball* b);

private:
    QGraphicsScene* m_scene;
    SpatialGrid& m_spatialGrid;
    QList<Ball*>& m_playerBalls;
    QList<Ball*>& m_aiBalls;
    QList<Food*>& m_foods;
    QList<EffectBall*>& m_effectBalls;
    qreal& m_score;
};