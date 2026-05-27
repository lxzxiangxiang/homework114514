#pragma once

#include <QGraphicsScene>

#include "SpatialGrid.h"

class Ball;
class Food;
class EffectBall;
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
    QList<EffectBall*> effectBalls;
    QList<EjectBall*> ejectBalls;

    qreal score = 0;
    qreal survivalTime = 0;
    QPointF playerInputDirection = {0, 0};
    bool wantSplit = false;
    bool wantEject = false;

    QString hudScoreText;
    QString hudTimeText;
    QString hudRadiusText;
    QString hudAICountText;
    QString hudEffectsText;
    QString hudSplitText;

private:
    void movePlayerBalls(qreal dt);
    void processSplitEject();
    QList<Ball*> buildAllBalls() const;
    void updateAIBalls(QList<Ball*>& allBalls, qreal dt);
    void updateAllTimers(const QList<Ball*>& allBalls, qreal dt);
    void updateMagnetEffect(const QList<Ball*>& allBalls, qreal dt);
    void updateProjectiles(qreal dt);
    void checkCollisions(const QList<Ball*>& allBalls);
    void applyAttraction(const QList<Ball*>& allBalls, qreal dt);
    void removeDeadEntities();
    static bool sameOwner(const Ball* a, const Ball* b);

    SpatialGrid m_spatialGrid;
    qreal m_skillSpawnTimer = 0;
    qreal m_hazardSpawnTimer = 0;
    int m_nextAiId = 1;
};
