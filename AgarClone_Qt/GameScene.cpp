// 游戏场景(GameScene)实现 — 游戏核心引擎，管理所有实体和游戏循环
#include "GameScene.h"

#include "Ball.h"
#include "Food.h"
#include "EffectBall.h"
#include "AIController.h"
#include "Constants.h"

#include <QRandomGenerator>
#include <QSet>
#include <QPainter>
#include <functional>
#include <cmath>

GameScene::GameScene(QObject* parent)
    : QGraphicsScene(parent)
{
    setSceneRect(0, 0, GameConstants::World::MAP_WIDTH, GameConstants::World::MAP_HEIGHT);
    setItemIndexMethod(QGraphicsScene::NoIndex);

    spawnFood(200);

    for (int i = 0; i < GameConstants::Spawning::AIBALL_COUNT; ++i) {
        spawnAIBall();
    }

    auto* player = new Ball(M_PI * 15.0 * 15.0, QColor(255, 80, 80), true, 0);
    player->setPos(GameConstants::World::MAP_WIDTH / 2, GameConstants::World::MAP_HEIGHT / 2);
    player->addInitialEffect(EffectType::Shield, 3.0f);
    addItem(player);
    playerBalls.append(player);
}

// ===== 主游戏循环：每 16ms 执行一次 =====
void GameScene::updateGame(qreal dt)
{
    movePlayerBalls(dt);
    processSplitEject();

    QList<Ball*> allBalls = buildAllBalls();
    updateAIBalls(allBalls, dt);
    updateAllTimers(allBalls, dt);
    updateMagnetEffect(allBalls, dt);
    updateProjectiles(dt);

    m_skillSpawnTimer -= dt;
    if (m_skillSpawnTimer <= 0 && effectBalls.size() < GameConstants::Spawning::MAX_EFFECT) {
        spawnSkillBall();
        m_skillSpawnTimer = 2.0f + QRandomGenerator::global()->bounded(2.0);
    }

    m_hazardSpawnTimer -= dt;
    int debuffCount = 0;
    for (EffectBall* eb : effectBalls) { if (eb->isAlive() && isDebuff(eb->effectType())) debuffCount++; }
    if (m_hazardSpawnTimer <= 0 && debuffCount < GameConstants::Spawning::MAX_HAZARD) {
        spawnHazard();
        m_hazardSpawnTimer = 3.0f + QRandomGenerator::global()->bounded(3.0);
    }

    applyAttraction(allBalls, dt);
    checkCollisions(allBalls);
    removeDeadEntities();

    int foodCount = foods.size();
    for (int i = foodCount; i < GameConstants::Spawning::MAX_FOOD; ++i) {
        spawnFood(1);
    }

    QSet<int> aliveAiIds;
    for (Ball* ai : aiBalls) { if (ai->isAlive()) aliveAiIds.insert(ai->aiId); }
    for (int i = 1; i <= GameConstants::Spawning::AIBALL_COUNT; ++i) {
        if (!aliveAiIds.contains(i)) {
            spawnAIBall(i);
        }
    }

    survivalTime += dt;
}

void GameScene::movePlayerBalls(qreal dt)
{
    for (Ball* ball : playerBalls) {
        if (ball->isAlive()) {
            ball->move(playerInputDirection.x(), playerInputDirection.y(), dt);
        }
    }
}

void GameScene::processSplitEject()
{
    if (wantSplit) {
        QList<Ball*> currentPlayerBalls = playerBalls;
        for (Ball* ball : currentPlayerBalls) {
            if (ball->isAlive() && ball->radius() >= GameConstants::Ball::SPLIT_THRESHOLD) {
                Ball* newBall = ball->split(playerInputDirection);
                if (newBall) {
                    addPlayerBall(newBall);
                }
            }
        }
        wantSplit = false;
    }
}

QList<Ball*> GameScene::buildAllBalls() const
{
    QList<Ball*> allBalls;
    allBalls.reserve(playerBalls.size() + aiBalls.size());
    allBalls.append(playerBalls);
    allBalls.append(aiBalls);
    return allBalls;
}

void GameScene::updateAIBalls(QList<Ball*>& allBalls, qreal dt)
{
    QSet<int> processedIds;
    QList<Ball*> newAiBalls;
    for (Ball* ai : aiBalls) {
        if (!ai->isAlive() || processedIds.contains(ai->aiId)) continue;
        processedIds.insert(ai->aiId);
        ai->pendingSplitBall = nullptr;
        AIController::updateAI(ai, allBalls, dt);

        QPointF dir = AIController::getLastDirection(ai);
        for (Ball* b : aiBalls) {
            if (b->isAlive() && b->aiId == ai->aiId) {
                b->move(dir.x(), dir.y(), dt);
            }
        }

        if (ai->pendingSplitBall) {
            addItem(ai->pendingSplitBall);
            newAiBalls.append(ai->pendingSplitBall);
            allBalls.append(ai->pendingSplitBall);
            ai->pendingSplitBall = nullptr;
        }
    }
    aiBalls.append(newAiBalls);
}

void GameScene::updateAllTimers(const QList<Ball*>& allBalls, qreal dt)
{
    for (Ball* ball : allBalls) {
        if (ball->isAlive()) { ball->update(dt); }
    }
}

void GameScene::updateMagnetEffect(const QList<Ball*>& allBalls, qreal dt)
{
    for (Ball* ball : allBalls) {
        if (!ball->isAlive() || !ball->hasEffect(EffectType::Magnet)) continue;
        for (Food* food : foods) {
            if (!food->isAlive()) continue;
            qreal dx = ball->x() - food->x();
            qreal dy = ball->y() - food->y();
            qreal dist = std::sqrt(dx * dx + dy * dy);
            qreal range = ball->radius() * GameConstants::Physics::MAGNET_RANGE_MULTIPLIER;
            if (dist < range && dist > 1e-6) {
                qreal force = GameConstants::Physics::MAGNET_FORCE * dt / dist;
                qreal fx = food->x() + dx * force;
                qreal fy = food->y() + dy * force;
                food->setPos(qBound(0.0, fx, (qreal)GameConstants::World::MAP_WIDTH),
                             qBound(0.0, fy, (qreal)GameConstants::World::MAP_HEIGHT));
            }
        }
    }
}

void GameScene::updateProjectiles(qreal dt)
{
    for (EffectBall* eb : effectBalls) {
        if (eb->isAlive()) { eb->update(dt); }
    }
}

bool GameScene::sameOwner(const Ball* a, const Ball* b)
{
    return (a->isPlayer && b->isPlayer)
        || (a->aiId > 0 && a->aiId == b->aiId);
}

// 在地图随机位置生成指定数量的豆子
void GameScene::spawnFood(int count)
{
    for (int i = 0; i < count; ++i) {
        qreal r = GameConstants::EntityRadius::FOOD_MIN + QRandomGenerator::global()->generateDouble() * (GameConstants::EntityRadius::FOOD_MAX - GameConstants::EntityRadius::FOOD_MIN);
        auto* food = new Food(r);
        qreal x = static_cast<qreal>(GameConstants::World::MAP_WIDTH) * QRandomGenerator::global()->generateDouble();
        qreal y = static_cast<qreal>(GameConstants::World::MAP_HEIGHT) * QRandomGenerator::global()->generateDouble();
        food->setPos(x, y);
        addItem(food);
        foods.append(food);
    }
}

void GameScene::spawnSkillBall()
{
    static const EffectType buffs[] = {
        EffectType::Speed, EffectType::Shield, EffectType::Grow,
        EffectType::Invisible, EffectType::Magnet
    };
    EffectType type = buffs[QRandomGenerator::global()->bounded(5)];
    qreal x = static_cast<qreal>(GameConstants::World::MAP_WIDTH) * QRandomGenerator::global()->generateDouble();
    qreal y = static_cast<qreal>(GameConstants::World::MAP_HEIGHT) * QRandomGenerator::global()->generateDouble();
    auto* eb = new EffectBall(QPointF(x, y), type);
    addItem(eb);
    effectBalls.append(eb);
}

void GameScene::spawnHazard()
{
    static const EffectType debuffs[] = {
        EffectType::Bomb, EffectType::Trap, EffectType::Poison
    };
    EffectType type = debuffs[QRandomGenerator::global()->bounded(3)];
    qreal x = static_cast<qreal>(GameConstants::World::MAP_WIDTH) * QRandomGenerator::global()->generateDouble();
    qreal y = static_cast<qreal>(GameConstants::World::MAP_HEIGHT) * QRandomGenerator::global()->generateDouble();
    auto* eb = new EffectBall(QPointF(x, y), type);
    addItem(eb);
    effectBalls.append(eb);
}

void GameScene::spawnAIBall(int targetId)
{
    QColor color(QRandomGenerator::global()->bounded(256),
                 QRandomGenerator::global()->bounded(256),
                 QRandomGenerator::global()->bounded(256));
    qreal radius = 15.0 + QRandomGenerator::global()->bounded(46.0);
    int aiLevel = 1 + QRandomGenerator::global()->bounded(3);

    auto* ai = new Ball(M_PI * radius * radius, color, false, aiLevel);
    ai->aiId = targetId ? targetId : m_nextAiId++;
    ai->addInitialEffect(EffectType::Shield, 3.0f);
    qreal x = static_cast<qreal>(GameConstants::World::MAP_WIDTH) * QRandomGenerator::global()->generateDouble();
    qreal y = static_cast<qreal>(GameConstants::World::MAP_HEIGHT) * QRandomGenerator::global()->generateDouble();
    ai->setPos(x, y);
    addItem(ai);
    aiBalls.append(ai);
}

// 注册玩家分裂产生的新球体到场景和列表
void GameScene::addPlayerBall(Ball* ball)
{
    addItem(ball);
    playerBalls.append(ball);
}

// ===== 碰撞检测 =====
// 使用空间网格优化，按顺序处理 5 种碰撞
// 碰撞判定：圆心距离 ≤ 半径之和，使用平方距离避免开方
void GameScene::checkCollisions(const QList<Ball*>& allBalls)
{
    // 构建空间网格（粗筛阶段）
    m_spatialGrid.clear();
    for (Ball* b : allBalls) {
        if (b->isAlive()) m_spatialGrid.add(b);
    }

    // 单次遍历处理 3 种实体碰撞（Food/SkillBall/Hazard）
    for (Ball* ball : allBalls) {
        if (!ball->isAlive()) continue;
        bool hasShield = ball->hasShield();

        for (Food* food : foods) {
            if (!food->isAlive()) continue;
            qreal dx = ball->x() - food->x();
            qreal dy = ball->y() - food->y();
            qreal distSq = dx * dx + dy * dy;
            qreal contactDist = ball->radius() + food->radius();
            if (distSq > contactDist * contactDist) continue;
            if (ball->radius() > food->radius() * GameConstants::Ball::EAT_RATIO) {
                ball->eat(food);
                if (ball->isPlayer) score += 1;
            }
        }

        for (EffectBall* eb : effectBalls) {
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

    // 6.5 Ball ↔ Ball 碰撞（同源合并 + 吞食）
    for (Ball* ball1 : allBalls) {
        if (!ball1->isAlive()) continue;
        QList<Entity*> nearby = m_spatialGrid.getNearby(ball1);
        for (Entity* e : nearby) {
            Ball* ball2 = dynamic_cast<Ball*>(e);
            if (!ball2 || ball2 == ball1 || !ball2->isAlive()) continue;
            // 避免重复处理（只处理 ball1 < ball2 的组合）
            if (std::less<Ball*>()(ball1, ball2)) continue;

            qreal dx = ball1->x() - ball2->x();
            qreal dy = ball1->y() - ball2->y();
            qreal distSq = dx * dx + dy * dy;
            qreal contactDist = qMax(ball1->radius(), ball2->radius());
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
                    if (ball1->isPlayer) score += ball2->radius() * 0.5;
                } else if (ball2->radius() > ball1->radius() * GameConstants::Ball::EAT_RATIO && !ball1->hasShield()) {
                    ball2->eat(ball1);
                    if (ball2->isPlayer) score += ball1->radius() * 0.5;
                    break;
                }
            }
        }
    }
}

// ===== 同源球体吸引力 =====
// 同玩家/AI 组的球体之间产生吸引力，随时间增加逐渐靠近合并
void GameScene::applyAttraction(const QList<Ball*>& allBalls, qreal dt)
{
    for (int i = 0; i < allBalls.size(); ++i) {
        Ball* b1 = allBalls[i];
        if (!b1->isAlive()) continue;
        for (int j = i + 1; j < allBalls.size(); ++j) {
            Ball* b2 = allBalls[j];
            if (!b2->isAlive()) continue;

            if (!sameOwner(b1, b2)) continue;
            if (b1->isInSplitAnim() || b2->isInSplitAnim()) continue;

            qreal dx = b2->x() - b1->x();
            qreal dy = b2->y() - b1->y();
            qreal dist = std::sqrt(dx * dx + dy * dy);
            if (dist < 1e-6) continue;

            // 吸引力 = 常数 + 距离^指数 + 时间²增量
            qreal attraction = GameConstants::Physics::ATTRACTION_BASE
                + GameConstants::Physics::ATTRACTION_DIST_FACTOR * std::pow(dist, GameConstants::Physics::ATTRACTION_DIST_EXPONENT)
                + GameConstants::Physics::ATTRACTION_TIME_FACTOR * std::pow(std::min(b1->mergeTimer(), b2->mergeTimer()), 2.0);

            qreal nx = dx / dist;
            qreal ny = dy / dist;
            qreal moveStep = attraction * dt / 2;

            b1->setPos(qBound(0.0, b1->x() + nx * moveStep, (qreal)GameConstants::World::MAP_WIDTH),
                       qBound(0.0, b1->y() + ny * moveStep, (qreal)GameConstants::World::MAP_HEIGHT));
            b2->setPos(qBound(0.0, b2->x() - nx * moveStep, (qreal)GameConstants::World::MAP_WIDTH),
                       qBound(0.0, b2->y() - ny * moveStep, (qreal)GameConstants::World::MAP_HEIGHT));
        }
    }
}

// 从所有实体列表中移除已死亡实体并释放内存
void GameScene::removeDeadEntities()
{
    // 模板 lambda：逆向遍历列表，移除死亡实体
    auto removeFromList = [this](auto& list) {
        for (int i = list.size() - 1; i >= 0; --i) {
            if (!list[i]->isAlive()) {
                removeItem(list[i]);                    // 从 QGraphicsScene 移除
                delete list[i];                         // 释放内存
                list.removeAt(i);                       // 从列表中移除
            }
        }
    };

    // 清理已死亡 AI 的状态，防止 s_states 内存泄漏
    for (Ball* ai : aiBalls) {
        if (!ai->isAlive()) AIController::resetState(ai);
    }

    removeFromList(playerBalls);
    removeFromList(aiBalls);
    removeFromList(foods);
    removeFromList(effectBalls);
}
