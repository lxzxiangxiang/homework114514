#pragma once

#include <QGraphicsScene>

#include "SpatialGrid.h"

class Ball;
class Food;
class EffectBall;
class CollisionSystem;

class GameScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit GameScene(QObject* parent = nullptr);
    ~GameScene();

    void updateGame(qreal dt);

    void spawnFood(int count);
    void spawnSkillBall();
    void spawnHazard();
    void spawnAIBall(int targetId = 0);

    void addPlayerBall(Ball* ball);

    QList<Ball*> playerBalls;
    QList<Ball*> aiBalls;
    QList<Food*> foods;
    QList<EffectBall*> effectBalls;

    qreal score = 0;
    qreal survivalTime = 0;
    QPointF playerInputDirection = {0, 0};
    bool wantSplit = false;

    QString hudScoreText;
    QString hudTimeText;
    QString hudRadiusText;
    QString hudAICountText;
    QString hudEffectsText;
    QString hudSplitText;

signals:
    void splitOccurred();

private:
    void movePlayerBalls(qreal dt);
    void processSplitEject();
    QList<Ball*> buildAllBalls() const;
    void updateAIBalls(QList<Ball*>& allBalls, qreal dt);
    void updateAllTimers(const QList<Ball*>& allBalls, qreal dt);
    void updateMagnetEffect(const QList<Ball*>& allBalls, qreal dt);
    void updateProjectiles(qreal dt);
    void removeDeadEntities();

    SpatialGrid m_spatialGrid;
    CollisionSystem* m_collision = nullptr;
    bool m_firstFrame = true;
    qreal m_skillSpawnTimer = 0;
    qreal m_hazardSpawnTimer = 0;
    int m_nextAiId = 1;
};