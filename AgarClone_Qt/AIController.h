#pragma once

#include <QHash>
#include <QPointF>
#include <QList>

class Ball;

class AIController {
public:
    struct AIState {
        QPointF targetDirection;
        QPointF currentDirection;
        qreal decisionTimer = 0;
        QPointF patrolTarget;
    };

    static void updateAI(Ball* ai, const QList<Ball*>& allBalls, qreal dt);
    static QPointF lastDirection(Ball* ai);
    static void resetState(Ball* ai);

private:
    static QHash<Ball*, AIState> s_states;
};
