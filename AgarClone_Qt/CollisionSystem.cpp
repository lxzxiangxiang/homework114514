#include "CollisionSystem.h"
#include "Ball.h"
#include "Food.h"
#include "EffectBall.h"
#include "Constants.h"
#include <cmath>

CollisionSystem::CollisionSystem(QGraphicsScene* scene, SpatialGrid& spatialGrid,
                                 QList<Ball*>& playerBalls, QList<Ball*>& aiBalls,
                                 QList<Food*>& foods, QList<EffectBall*>& effectBalls,
                                 qreal& score)
    : m_scene(scene)
    , m_spatialGrid(spatialGrid)
    , m_playerBalls(playerBalls)
    , m_aiBalls(aiBalls)
    , m_foods(foods)
    , m_effectBalls(effectBalls)
    , m_score(score)
{
}

bool CollisionSystem::sameOwner(const Ball* a, const Ball* b)
{
    return (a->isPlayer && b->isPlayer)
        || (a->aiId > 0 && a->aiId == b->aiId);
}

void CollisionSystem::checkCollisions(const QList<Ball*>& allBalls)
{
    m_spatialGrid.clear();
    for (Ball* b : allBalls) {
        if (b->isAlive()) m_spatialGrid.add(b);
    }

    for (Ball* ball : allBalls) {
        if (!ball->isAlive()) continue;

        for (Food* food : m_foods) {
            if (!food->isAlive()) continue;
            qreal dx = ball->x() - food->x();
            qreal dy = ball->y() - food->y();
            qreal distSq = dx * dx + dy * dy;
            qreal contactDist = ball->radius() + food->radius();
            if (distSq > contactDist * contactDist) continue;
            if (ball->radius() > food->radius() * GameConstants::Ball::EAT_RATIO) {
                ball->eat(food);
                if (ball->isPlayer) m_score += 1;
            }
        }

        for (EffectBall* eb : m_effectBalls) {
            if (!eb->isAlive()) continue;
            qreal dx = ball->x() - eb->x();
            qreal dy = ball->y() - eb->y();
            qreal distSq = dx * dx + dy * dy;
            qreal contactDist = ball->radius() + eb->radius();
            if (distSq > contactDist * contactDist) continue;
            ball->applyEffect(eb->effectType(), allBalls);
            eb->onEaten(ball);
        }
    }

    for (Ball* ball1 : allBalls) {
        if (!ball1->isAlive()) continue;
        QList<Entity*> nearby = m_spatialGrid.nearbyEntities(ball1);
        for (Entity* e : nearby) {
            Ball* ball2 = dynamic_cast<Ball*>(e);
            if (!ball2 || ball2 == ball1 || !ball2->isAlive()) continue;
            if (std::less<Ball*>()(ball1, ball2)) continue;

            qreal dx = ball1->x() - ball2->x();
            qreal dy = ball1->y() - ball2->y();
            qreal distSq = dx * dx + dy * dy;
            qreal contactDist = (std::max)(ball1->radius(), ball2->radius());
            if (distSq > contactDist * contactDist) continue;

            bool same = sameOwner(ball1, ball2);

            if (same) {
                if (ball1->radius() >= ball2->radius()) {
                    ball1->eat(ball2);
                } else {
                    ball2->eat(ball1);
                    break;
                }
            } else {
                if (ball1->radius() > ball2->radius() * GameConstants::Ball::EAT_RATIO && !ball2->hasShield()) {
                    ball1->eat(ball2);
                    if (ball1->isPlayer) m_score += ball2->radius() * 0.5;
                } else if (ball2->radius() > ball1->radius() * GameConstants::Ball::EAT_RATIO && !ball1->hasShield()) {
                    ball2->eat(ball1);
                    if (ball2->isPlayer) m_score += ball1->radius() * 0.5;
                    break;
                }
            }
        }
    }
}

void CollisionSystem::applyAttraction(const QList<Ball*>& allBalls, qreal dt)
{
    QHash<int, QList<Ball*>> groups;
    for (Ball* b : allBalls) {
        if (!b->isAlive() || b->isInSplitAnim()) continue;
        if (b->isPlayer)
            groups[0].append(b);
        else if (b->aiId > 0)
            groups[b->aiId].append(b);
    }

    for (auto it = groups.begin(); it != groups.end(); ++it) {
        const QList<Ball*>& group = it.value();
        for (int i = 0; i < group.size(); ++i) {
            Ball* b1 = group[i];
            for (int j = i + 1; j < group.size(); ++j) {
                Ball* b2 = group[j];

                qreal dx = b2->x() - b1->x();
                qreal dy = b2->y() - b1->y();
                qreal dist = std::sqrt(dx * dx + dy * dy);
                if (dist < 1e-6) continue;

                qreal attraction = GameConstants::Physics::ATTRACTION_BASE
                    + GameConstants::Physics::ATTRACTION_DIST_FACTOR * std::pow(dist, GameConstants::Physics::ATTRACTION_DIST_EXPONENT)
                    + GameConstants::Physics::ATTRACTION_TIME_FACTOR * std::pow((std::min)(b1->mergeTimer(), b2->mergeTimer()), 2.0);

                qreal nx = dx / dist;
                qreal ny = dy / dist;
                qreal moveStep = attraction * dt / 2;

                b1->setPos(std::clamp(b1->x() + nx * moveStep, (qreal)b1->radius(), (qreal)GameConstants::World::MAP_WIDTH - b1->radius()),
                           std::clamp(b1->y() + ny * moveStep, (qreal)b1->radius(), (qreal)GameConstants::World::MAP_HEIGHT - b1->radius()));
                b2->setPos(std::clamp(b2->x() - nx * moveStep, (qreal)b2->radius(), (qreal)GameConstants::World::MAP_WIDTH - b2->radius()),
                           std::clamp(b2->y() - ny * moveStep, (qreal)b2->radius(), (qreal)GameConstants::World::MAP_HEIGHT - b2->radius()));
            }
        }
    }
}