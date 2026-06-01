#pragma once

#include <QGraphicsScene>

#include "SpatialGrid.h"
#include "Constants.h"

class Ball;
class Food;
class EffectBall;
class CollisionSystem;

class GameScene : public QGraphicsScene {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(GameScene)

public:
    explicit GameScene(QObject* parent = nullptr);
    ~GameScene();

    void updateGame(qreal dt);

    void spawnFood(int count);
    void spawnSkillBall();
    void spawnHazard();
    void spawnEffectBall(const EffectType* types, int count);
    void spawnAIBall(int targetId = 0);

    void addPlayerBall(Ball* ball);

    QList<Ball*> m_playerBalls;
    QList<Ball*> m_aiBalls;
    QList<Food*> m_foods;
    QList<EffectBall*> m_effectBalls;

    qreal m_score = 0;
    qreal m_survivalTime = 0;
    QPointF m_playerInputDirection = {0, 0};
    bool m_wantSplit = false;

    QString m_hudScoreText;
    QString m_hudTimeText;
    QString m_hudRadiusText;
    QString m_hudAICountText;
    QString m_hudEffectsText;
    QString m_hudSplitText;

signals:
    void splitOccurred();

private:
    void movePlayerBalls(qreal dt);
    void processSplitEject();
    void buildAllBalls();
    void updateAIBalls(QList<Ball*>& allBalls, qreal dt);
    void updateAllTimers(const QList<Ball*>& allBalls, qreal dt);
    void updateMagnetEffect(const QList<Ball*>& allBalls, qreal dt);
    void updateProjectiles(qreal dt);
    void removeDeadEntities();

    SpatialGrid m_spatialGrid;
    CollisionSystem* m_collision = nullptr;
    QList<Ball*> m_allBalls;
    bool m_firstFrame = true;
    bool m_spawningDone = false;
    qreal m_skillSpawnTimer = 0;
    qreal m_hazardSpawnTimer = 0;
    int m_nextAiId = 1;
};