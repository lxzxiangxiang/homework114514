// AI 控制器(AIController)实现 — 管理非玩家球体的自主行为
// 实现三级 AI 决策系统：威胁检测→猎物追踪→巡逻→分裂决策，以及平滑转向
#include "AIController.h"
#include "Ball.h"
#include "Food.h"
#include "Constants.h"
#include <QRandomGenerator>
#include <cmath>

// 每个 AI 球体独立维护的决策状态表（静态成员）
QHash<Ball*, AIController::AIState> AIController::s_states;

// 限制向量长度不超过 maxLen
static qreal clampLength(const QPointF& vec, qreal maxLen)
{
    qreal len = std::sqrt(vec.x() * vec.x() + vec.y() * vec.y());
    if (len <= maxLen || len < 1e-6)
        return len;
    return maxLen;
}

// 计算向量长度
static qreal length(const QPointF& vec)
{
    return std::sqrt(vec.x() * vec.x() + vec.y() * vec.y());
}

// 向量归一化，长度 < 1e-6 时返回零向量避免除零
static QPointF normalized(const QPointF& vec)
{
    qreal len = length(vec);
    if (len < 1e-6)
        return QPointF(0, 0);
    return QPointF(vec.x() / len, vec.y() / len);
}

// AI 决策主函数：每帧调用一次，驱动单个 AI 球体的自主行为
void AIController::updateAI(Ball* ai, const QList<Ball*>& allBalls, const QList<Food*>& foods, qreal dt)
{
    // 空指针或死亡检查
    if (!ai || !ai->isAlive())
        return;

    // 获取或创建该 AI 的决策状态
    AIState& state = s_states[ai];
    state.decisionTimer -= dt;

    // 决策间隔到期，执行新一轮决策
    if (state.decisionTimer <= 0) {
        int level = ai->aiLevel;
        // 根据 AI 等级重置决策间隔
        switch (level) {
        case 1:
            state.decisionTimer = GameConstants::AI::Level1::DECISION_TIME;
            break;
        case 2:
            state.decisionTimer = GameConstants::AI::Level2::DECISION_TIME;
            break;
        case 3:
        default:
            state.decisionTimer = GameConstants::AI::Level3::DECISION_TIME;
            break;
        }

        qreal myRadius = ai->radius();
        QPointF myPos = ai->pos();

        // ===== 威胁检测 =====
        // 找半径 > 自身×1.1 且在 6×半径范围内的对手，朝反方向逃离
        bool hasThreat = false;
        QPointF fleeDirection(0, 0);

        for (Ball* other : allBalls) {
            if (other == ai || !other->isAlive())
                continue;
            if (other->radius() <= myRadius * GameConstants::EAT_RATIO)
                continue;
            QPointF delta = other->pos() - myPos;
            qreal dist = length(delta);
            if (dist > myRadius * 6.0)
                continue;                                   // 超出威胁感知范围
            hasThreat = true;
            fleeDirection -= delta;                         // 多个威胁矢量叠加
        }

        // ===== 猎物检测 =====
        // 找半径 < 自身×0.9 的球体，按 value = radius/(dist+1) 评分追击最佳猎物
        bool hasPrey = false;
        QPointF chaseDirection(0, 0);

        qreal bestPreyScore = 0;
        qreal bestPreyRadius = 0;
        for (Ball* other : allBalls) {
            if (other == ai || !other->isAlive())
                continue;
            if (other->radius() >= myRadius * 0.9)
                continue;
            QPointF delta = other->pos() - myPos;
            qreal dist = length(delta);
            qreal score = other->radius() / (dist + 1.0);   // 价值评分：半径/距离
            if (score > bestPreyScore) {
                bestPreyScore = score;
                bestPreyRadius = other->radius();
                chaseDirection = delta;
                hasPrey = true;
            }
        }

        // 行为优先级：逃脱 > 追猎 > 巡逻
        if (hasThreat) {
            state.targetDirection = normalized(fleeDirection);
        } else if (hasPrey) {
            state.targetDirection = normalized(chaseDirection);
        } else {
            // ===== 巡逻 =====
            // 无威胁无猎物时，随机选择地图内巡逻点移动
            if (state.patrolTarget.isNull() || length(myPos - state.patrolTarget) < myRadius) {
                qreal rx = QRandomGenerator::global()->generateDouble() * GameConstants::MAP_WIDTH;
                qreal ry = QRandomGenerator::global()->generateDouble() * GameConstants::MAP_HEIGHT;
                state.patrolTarget = QPointF(rx, ry);
            }
            state.targetDirection = normalized(state.patrolTarget - myPos);
        }

        // ===== 分裂决策 (Level ≥ 2) =====
        // 分裂逃生：有威胁且距离 < 自身半径×3，朝逃离方向分裂
        // 分裂猎食：有猎物且距离 < 自身半径×4 且半径 > 猎物×1.5，朝猎物方向分裂
        // Level 3 更激进：阈值更宽
        if (level >= 2 && ai->radius() >= GameConstants::SPLIT_THRESHOLD && ai->splitTimer <= 0) {
            qreal splitDist = (level >= 3) ? 5.0 : 4.0;
            if (hasThreat && length(fleeDirection) < myRadius * 3.0) {
                ai->pendingSplitBall = ai->split(normalized(fleeDirection));
                if (ai->pendingSplitBall) ai->splitTimer = 1.5f;
            } else if (hasPrey && length(chaseDirection) < myRadius * splitDist
                       && myRadius > bestPreyRadius * 1.5) {
                ai->pendingSplitBall = ai->split(normalized(chaseDirection));
                if (ai->pendingSplitBall) ai->splitTimer = 1.5f;
            }
        }
    }

    // ===== 平滑转向 =====
    // 使用 atan2 计算角度差，以 TURN_RATE×dt 为最大步长做线性插值
    qreal turnSpeed = 0;
    int level = ai->aiLevel;
    switch (level) {
    case 1:
        turnSpeed = GameConstants::AI::Level1::TURN_RATE;
        break;
    case 2:
        turnSpeed = GameConstants::AI::Level2::TURN_RATE;
        break;
    case 3:
    default:
        turnSpeed = GameConstants::AI::Level3::TURN_RATE;
        break;
    }

    qreal angleCurrent = std::atan2(state.currentDirection.y(), state.currentDirection.x());
    qreal angleTarget = std::atan2(state.targetDirection.y(), state.targetDirection.x());
    qreal angleDiff = angleTarget - angleCurrent;

    // 将角度差规整到 [-π, π] 范围
    while (angleDiff > M_PI) angleDiff -= 2.0 * M_PI;
    while (angleDiff < -M_PI) angleDiff += 2.0 * M_PI;

    // 限制单帧最大转向角度
    qreal maxTurn = turnSpeed * dt;
    angleDiff = qBound(-maxTurn, angleDiff, maxTurn);
    qreal newAngle = angleCurrent + angleDiff;

    // 根据新角度计算方向向量
    state.currentDirection = QPointF(std::cos(newAngle), std::sin(newAngle));

    // 执行移动
    ai->move(state.currentDirection.x(), state.currentDirection.y(), dt);
}

// 清除指定 AI 的状态（AI 死亡时调用）
void AIController::resetState(Ball* ai)
{
    s_states.remove(ai);
}
