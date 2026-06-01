// 游戏场景(GameScene)实现 — 游戏核心引擎，管理所有实体和游戏循环
#include "GameScene.h"

#include "Ball.h"
#include "Food.h"
#include "EffectBall.h"
#include "CollisionSystem.h"
#include "AIController.h"
#include "Constants.h"

#include <QRandomGenerator>
#include <QSet>
#include <QPainter>
#include <cmath>

GameScene::GameScene(QObject* parent)
    : QGraphicsScene(parent)
{
    setSceneRect(0, 0, GameConstants::World::MAP_WIDTH, GameConstants::World::MAP_HEIGHT);
    setItemIndexMethod(QGraphicsScene::NoIndex);

    m_collision = new CollisionSystem(m_spatialGrid, m_foods, m_effectBalls, m_score);

    spawnFood(GameConstants::Spawning::MAX_FOOD);
    for (int i = 0; i < GameConstants::Spawning::AIBALL_COUNT; ++i) {
        spawnAIBall();
    }
    m_spawningDone = true;

    auto* player = new Ball(M_PI * GameConstants::EntityRadius::PLAYER_INITIAL * GameConstants::EntityRadius::PLAYER_INITIAL, QColor(255, 80, 80), true, 0);
    player->setPos(GameConstants::World::MAP_WIDTH / 2, GameConstants::World::MAP_HEIGHT / 2);
    player->addInitialEffect(EffectType::Shield, GameConstants::EffectDuration::INITIAL_SHIELD);
    addItem(player);
    m_playerBalls.append(player);
}

GameScene::~GameScene()
{
    delete m_collision;
}

// ===== 主游戏循环：每 16ms 执行一次 =====
void GameScene::updateGame(qreal dt)
{
    movePlayerBalls(dt);
    processSplitEject();

    buildAllBalls();

    if (m_firstFrame) {
        m_firstFrame = false;
        updateAllTimers(m_allBalls, dt);
        m_spatialGrid.clear();
        for (Ball* b : m_allBalls) {
            if (b->isAlive()) m_spatialGrid.add(b);
        }
        return;
    }

    updateAIBalls(m_allBalls, dt);
    updateAllTimers(m_allBalls, dt);
    updateMagnetEffect(m_allBalls, dt);
    updateProjectiles(dt);

    m_skillSpawnTimer -= dt;
    if (m_skillSpawnTimer <= 0 && m_effectBalls.size() < GameConstants::Spawning::MAX_EFFECT) {
        spawnSkillBall();
        m_skillSpawnTimer = GameConstants::Spawning::SKILL_INTERVAL_MIN + QRandomGenerator::global()->bounded(GameConstants::Spawning::SKILL_INTERVAL_RANGE);
    }

    m_hazardSpawnTimer -= dt;
    int debuffCount = 0;
    for (EffectBall* eb : m_effectBalls) { if (eb->isAlive() && isDebuff(eb->effectType())) debuffCount++; }
    if (m_hazardSpawnTimer <= 0 && debuffCount < GameConstants::Spawning::MAX_HAZARD) {
        spawnHazard();
        m_hazardSpawnTimer = GameConstants::Spawning::HAZARD_INTERVAL_MIN + QRandomGenerator::global()->bounded(GameConstants::Spawning::HAZARD_INTERVAL_RANGE);
    }

    m_collision->applyAttraction(m_allBalls, dt);
    m_collision->checkCollisions(m_allBalls);
    removeDeadEntities();

    if (m_spawningDone) {
        int foodCount = m_foods.size();
        for (int i = foodCount; i < GameConstants::Spawning::MAX_FOOD; ++i) {
            spawnFood(1);
        }

        QSet<int> aliveAiIds;
        for (Ball* ai : m_aiBalls) { if (ai->isAlive()) aliveAiIds.insert(ai->m_aiId); }
        for (int i = 1; i <= GameConstants::Spawning::AIBALL_COUNT; ++i) {
            if (!aliveAiIds.contains(i)) {
                spawnAIBall(i);
            }
        }
    }

    m_survivalTime += dt;
}

void GameScene::movePlayerBalls(qreal dt)
{
    for (Ball* ball : m_playerBalls) {
        if (ball->isAlive()) {
            ball->move(m_playerInputDirection.x(), m_playerInputDirection.y(), dt);
        }
    }
}

void GameScene::processSplitEject()
{
    if (m_wantSplit) {
        QList<Ball*> currentPlayerBalls = m_playerBalls;
        for (Ball* ball : currentPlayerBalls) {
            if (ball->isAlive() && ball->radius() >= GameConstants::Ball::SPLIT_THRESHOLD) {
                Ball* newBall = ball->split(m_playerInputDirection);
                if (newBall) {
                    addPlayerBall(newBall);
                    emit splitOccurred();
                }
            }
        }
        m_wantSplit = false;
    }
}

void GameScene::buildAllBalls()
{
    m_allBalls.clear();
    m_allBalls.reserve(m_playerBalls.size() + m_aiBalls.size());
    m_allBalls.append(m_playerBalls);
    m_allBalls.append(m_aiBalls);
}

void GameScene::updateAIBalls(QList<Ball*>& allBalls, qreal dt)
{
    QSet<int> processedIds;
    QList<Ball*> newAiBalls;
    for (Ball* ai : m_aiBalls) {
        if (!ai->isAlive() || processedIds.contains(ai->m_aiId)) continue;
        processedIds.insert(ai->m_aiId);
        ai->m_pendingSplitBall = nullptr;
        AIController::updateAI(ai, allBalls, dt);

        QPointF dir = AIController::lastDirection(ai);
        for (Ball* b : m_aiBalls) {
            if (b->isAlive() && b->m_aiId == ai->m_aiId) {
                b->move(dir.x(), dir.y(), dt);
            }
        }

        if (ai->m_pendingSplitBall) {
            addItem(ai->m_pendingSplitBall);
            newAiBalls.append(ai->m_pendingSplitBall);
            allBalls.append(ai->m_pendingSplitBall);
            ai->m_pendingSplitBall = nullptr;
        }
    }
    m_aiBalls.append(newAiBalls);
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
        qreal range = ball->radius() * GameConstants::Physics::MAGNET_RANGE_MULTIPLIER;
        qreal bx = ball->x(), by = ball->y();
        for (Food* food : m_foods) {
            if (!food->isAlive()) continue;
            qreal dx = bx - food->x();
            qreal dy = by - food->y();
            if (std::abs(dx) > range || std::abs(dy) > range) continue;
            qreal dist = std::sqrt(dx * dx + dy * dy);
            if (dist < range && dist > 1e-6) {
                qreal force = GameConstants::Physics::MAGNET_FORCE * dt / dist;
                qreal fx = food->x() + dx * force;
                qreal fy = food->y() + dy * force;
                food->setPos(std::clamp(fx, 0.0, (qreal)GameConstants::World::MAP_WIDTH),
                             std::clamp(fy, 0.0, (qreal)GameConstants::World::MAP_HEIGHT));
            }
        }
    }
}

void GameScene::updateProjectiles(qreal dt)
{
    for (EffectBall* eb : m_effectBalls) {
        if (eb->isAlive()) { eb->update(dt); }
    }
}

// 在地图随机位置生成指定数量的豆子
void GameScene::spawnFood(int count)
{
    for (int i = 0; i < count; ++i) {
        qreal r = GameConstants::EntityRadius::FOOD_MIN + QRandomGenerator::global()->generateDouble() * (GameConstants::EntityRadius::FOOD_MAX - GameConstants::EntityRadius::FOOD_MIN);
        auto* food = new Food(r);
        qreal x = r + static_cast<qreal>(GameConstants::World::MAP_WIDTH - 2 * r) * QRandomGenerator::global()->generateDouble();
        qreal y = r + static_cast<qreal>(GameConstants::World::MAP_HEIGHT - 2 * r) * QRandomGenerator::global()->generateDouble();
        food->setPos(x, y);
        addItem(food);
        m_foods.append(food);
    }
}

void GameScene::spawnSkillBall()
{
    static const EffectType buffs[] = {
        EffectType::Speed, EffectType::Shield, EffectType::Grow,
        EffectType::Invisible, EffectType::Magnet
    };
    spawnEffectBall(buffs, 5);
}

void GameScene::spawnHazard()
{
    static const EffectType debuffs[] = {
        EffectType::Bomb, EffectType::Trap, EffectType::Poison
    };
    spawnEffectBall(debuffs, 3);
}

void GameScene::spawnEffectBall(const EffectType* types, int count)
{
    EffectType type = types[QRandomGenerator::global()->bounded(count)];
    qreal r = GameConstants::EntityRadius::EFFECTBALL;
    qreal x = r + static_cast<qreal>(GameConstants::World::MAP_WIDTH - 2 * r) * QRandomGenerator::global()->generateDouble();
    qreal y = r + static_cast<qreal>(GameConstants::World::MAP_HEIGHT - 2 * r) * QRandomGenerator::global()->generateDouble();
    auto* eb = new EffectBall(QPointF(x, y), type);
    addItem(eb);
    m_effectBalls.append(eb);
}

void GameScene::spawnAIBall(int targetId)
{
    QColor color(QRandomGenerator::global()->bounded(256),
                 QRandomGenerator::global()->bounded(256),
                 QRandomGenerator::global()->bounded(256));
    qreal radius = GameConstants::EntityRadius::AI_INITIAL_MIN + QRandomGenerator::global()->bounded(GameConstants::EntityRadius::AI_INITIAL_RANGE);
    int aiLevel = 1 + QRandomGenerator::global()->bounded(3);

    auto* ai = new Ball(M_PI * radius * radius, color, false, aiLevel);
    ai->m_aiId = targetId ? targetId : m_nextAiId++;
    ai->addInitialEffect(EffectType::Shield, GameConstants::EffectDuration::INITIAL_SHIELD);
    qreal x = radius + static_cast<qreal>(GameConstants::World::MAP_WIDTH - 2 * radius) * QRandomGenerator::global()->generateDouble();
    qreal y = radius + static_cast<qreal>(GameConstants::World::MAP_HEIGHT - 2 * radius) * QRandomGenerator::global()->generateDouble();
    ai->setPos(x, y);
    addItem(ai);
    m_aiBalls.append(ai);
}

// 注册玩家分裂产生的新球体到场景和列表
void GameScene::addPlayerBall(Ball* ball)
{
    addItem(ball);
    m_playerBalls.append(ball);
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
    for (Ball* ai : m_aiBalls) {
        if (!ai->isAlive()) AIController::resetState(ai);
    }

    removeFromList(m_playerBalls);
    removeFromList(m_aiBalls);
    removeFromList(m_foods);
    removeFromList(m_effectBalls);
}