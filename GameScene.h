#pragma once

#include <QGraphicsScene>

#include "SpatialGrid.h"

class Ball;
class Food;
class SkillBall;
class Hazard;
class EjectBall;

class GameScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit GameScene(QObject* parent = nullptr);

    void updateGame(qreal dt);

    void spawnFood(int count);
    void spawnSkillBall();
    void spawnHazard();
    void spawnAIBall();

    void addPlayerBall(Ball* ball);
    void addEjectBall(class EjectBall* eb);

    QList<Ball*> playerBalls;
    QList<Ball*> aiBalls;
    QList<Food*> foods;
    QList<SkillBall*> skillBalls;
    QList<Hazard*> hazards;
    QList<EjectBall*> ejectBalls;

    qreal score = 0;
    qreal survivalTime = 0;
    QPointF playerInputDirection = {0, 0};
    bool wantSplit = false;
    bool wantEject = false;

private:
    void checkCollisions();
    void applyAttraction(qreal dt);
    void removeDeadEntities();

    SpatialGrid m_spatialGrid;
    qreal m_skillSpawnTimer = 0;
    qreal m_hazardSpawnTimer = 0;
    int m_nextAiId = 1;
};
