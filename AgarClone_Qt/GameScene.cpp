// 游戏场景(GameScene)实现 — 游戏核心引擎，管理所有实体和游戏循环
// 每帧执行 9 步流程：移动→分裂/吐孢→AI→抛射物→生成→吸引→碰撞→移除→计时
#include "GameScene.h"

#include "Ball.h"
#include "Food.h"
#include "SkillBall.h"
#include "Hazard.h"
#include "EjectBall.h"
#include "AIController.h"
#include "Constants.h"

#include <QRandomGenerator>

GameScene::GameScene(QObject* parent)
    : QGraphicsScene(parent)
{
    // 设置场景尺寸为地图大小
    setSceneRect(0, 0, GameConstants::MAP_WIDTH, GameConstants::MAP_HEIGHT);

    // 初始生成 100 个豆子、5 个 AI 球体和 1 个玩家球体
    spawnFood(100);

    for (int i = 0; i < 5; ++i) {
        spawnAIBall();
    }

    auto* player = new Ball(15.0, QColor(255, 80, 80), true, 0);
    player->setPos(GameConstants::MAP_WIDTH / 2, GameConstants::MAP_HEIGHT / 2);
    player->invincibleTimer = 0;
    addItem(player);
    playerBalls.append(player);
}

// ===== 主游戏循环：每 16ms 执行一次 =====
void GameScene::updateGame(qreal dt)
{
    // 1. 移动玩家球体：朝输入方向移动
    for (Ball* ball : playerBalls) {
        if (ball->isAlive()) {
            ball->move(playerInputDirection.x(), playerInputDirection.y(), dt);
        }
    }

    // 2. 处理分裂请求（按空格键触发）
    if (wantSplit) {
        QList<Ball*> currentPlayerBalls = playerBalls;
        for (Ball* ball : currentPlayerBalls) {
            if (ball->isAlive() && ball->radius() >= GameConstants::SPLIT_THRESHOLD) {
                Ball* newBall = ball->split(playerInputDirection);
                if (newBall) {
                    addPlayerBall(newBall);
                }
            }
        }
        wantSplit = false;                              // 单次触发，消费标志位
    }

    // 3. 处理吐孢请求（按 E 键触发）
    if (wantEject) {
        for (Ball* ball : playerBalls) {
            if (ball->isAlive() && ball->radius() >= GameConstants::EJECT_THRESHOLD) {
                EjectBall* eb = ball->eject();
                if (eb) {
                    addEjectBall(eb);
                }
            }
        }
        wantEject = false;
    }

    // 4. 合并玩家和 AI 球体列表，供 AIController 做决策参考
    QList<Ball*> allBalls;
    allBalls.append(playerBalls);
    allBalls.append(aiBalls);

    // 5. 更新所有 AI 球体的自主行为
    for (Ball* ai : aiBalls) {
        if (ai->isAlive()) {
            ai->pendingSplitBall = nullptr;
            AIController::updateAI(ai, allBalls, foods, dt);
            if (ai->pendingSplitBall) {
                addItem(ai->pendingSplitBall);
                aiBalls.append(ai->pendingSplitBall);
                ai->pendingSplitBall = nullptr;
            }
        }
    }

    // 6. 更新所有抛射物（飞行 + 衰减 + 生命周期）
    for (EjectBall* eb : ejectBalls) {
        if (eb->isAlive()) {
            eb->update(dt);
        }
    }

    // 7. 技能球生成计时器：每约 5 秒概率生成，最多 MAX_SKILLBALL_COUNT 个
    m_skillSpawnTimer -= dt;
    if (m_skillSpawnTimer <= 0 && skillBalls.size() < GameConstants::MAX_SKILLBALL_COUNT) {
        spawnSkillBall();
        m_skillSpawnTimer = 5.0f + QRandomGenerator::global()->bounded(5.0);
    }

    // 8. 危险物生成计时器：每约 8~15 秒概率生成，最多 MAX_HAZARD_COUNT 个
    m_hazardSpawnTimer -= dt;
    if (m_hazardSpawnTimer <= 0 && hazards.size() < GameConstants::MAX_HAZARD_COUNT) {
        spawnHazard();
        m_hazardSpawnTimer = 8.0f + QRandomGenerator::global()->bounded(7.0);
    }

    // 9. 同源球体吸引力（同玩家/AI 组的球体相互吸引合并）
    applyAttraction(dt);

    // 10. 碰撞检测（球↔食物、球↔技能、球↔危险物、球↔抛射物、球↔球）
    checkCollisions();

    // 11. 移除所有死亡实体
    removeDeadEntities();

    // 12. 补充豆子至 MAX_FOOD_COUNT
    int foodCount = foods.size();
    for (int i = foodCount; i < GameConstants::MAX_FOOD_COUNT; ++i) {
        spawnFood(1);
    }

    // 13. 累加生存时间
    survivalTime += dt;
}

// 在地图随机位置生成指定数量的豆子
void GameScene::spawnFood(int count)
{
    for (int i = 0; i < count; ++i) {
        auto* food = new Food();
        qreal x = QRandomGenerator::global()->bounded(GameConstants::MAP_WIDTH);
        qreal y = QRandomGenerator::global()->bounded(GameConstants::MAP_HEIGHT);
        food->setPos(x, y);
        addItem(food);                                  // 注册到 QGraphicsScene
        foods.append(food);
    }
}

// 在随机位置生成一个技能球（5 种技能随机一种）
void GameScene::spawnSkillBall()
{
    auto* sb = new SkillBall();
    qreal x = QRandomGenerator::global()->bounded(GameConstants::MAP_WIDTH);
    qreal y = QRandomGenerator::global()->bounded(GameConstants::MAP_HEIGHT);
    sb->setPos(x, y);
    addItem(sb);
    skillBalls.append(sb);
}

// 在随机位置生成一个危险物（3 种类型随机一种）
void GameScene::spawnHazard()
{
    auto* hazard = new Hazard();
    qreal x = QRandomGenerator::global()->bounded(GameConstants::MAP_WIDTH);
    qreal y = QRandomGenerator::global()->bounded(GameConstants::MAP_HEIGHT);
    hazard->setPos(x, y);
    addItem(hazard);
    hazards.append(hazard);
}

// 在随机位置生成一个 AI 球体：随机颜色、半径 15~60、等级 1~3
void GameScene::spawnAIBall()
{
    QColor color(QRandomGenerator::global()->bounded(256),
                 QRandomGenerator::global()->bounded(256),
                 QRandomGenerator::global()->bounded(256));
    qreal radius = 15.0 + QRandomGenerator::global()->bounded(46.0);
    int aiLevel = 1 + QRandomGenerator::global()->bounded(3);

    auto* ai = new Ball(radius, color, false, aiLevel);
    ai->aiId = m_nextAiId++;
    qreal x = QRandomGenerator::global()->bounded(GameConstants::MAP_WIDTH);
    qreal y = QRandomGenerator::global()->bounded(GameConstants::MAP_HEIGHT);
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

// 注册吐孢产生的孢子到场景和列表
void GameScene::addEjectBall(EjectBall* eb)
{
    addItem(eb);
    ejectBalls.append(eb);
}

// ===== 碰撞检测 =====
// 使用空间网格优化，按顺序处理 5 种碰撞
// 碰撞判定：圆心距离 ≤ 半径之和，使用平方距离避免开方
void GameScene::checkCollisions()
{
    QList<Ball*> allBalls;
    allBalls.append(playerBalls);
    allBalls.append(aiBalls);

    // 构建空间网格（粗筛阶段）
    m_spatialGrid.clear();
    for (Ball* b : allBalls) {
        if (b->isAlive()) m_spatialGrid.add(b);
    }

    // 6.1 Ball ↔ Food 碰撞
    for (Ball* ball : allBalls) {
        if (!ball->isAlive()) continue;
        for (Food* food : foods) {
            if (!food->isAlive()) continue;
            qreal dx = ball->x() - food->x();
            qreal dy = ball->y() - food->y();
            qreal distSq = dx * dx + dy * dy;
            qreal contactDist = ball->radius() + food->radius();
            if (distSq > contactDist * contactDist) continue;
            if (ball->radius() > food->radius() * GameConstants::EAT_RATIO) {
                ball->eat(food);
                if (ball->isPlayer) score += 1;
            }
        }
    }

    // 6.2 Ball ↔ SkillBall 碰撞
    for (Ball* ball : allBalls) {
        if (!ball->isAlive()) continue;
        for (SkillBall* sb : skillBalls) {
            if (!sb->isAlive()) continue;
            qreal dx = ball->x() - sb->x();
            qreal dy = ball->y() - sb->y();
            qreal distSq = dx * dx + dy * dy;
            qreal contactDist = ball->radius() + sb->radius();
            if (distSq > contactDist * contactDist) continue;
            ball->applySkill(sb->skillType);
            sb->onEaten(ball);
        }
    }

    // 6.3 Ball ↔ Hazard 碰撞
    for (Ball* ball : allBalls) {
        if (!ball->isAlive()) continue;
        if (ball->hasShield()) continue;
        for (Hazard* hazard : hazards) {
            if (!hazard->isAlive()) continue;
            qreal dx = ball->x() - hazard->x();
            qreal dy = ball->y() - hazard->y();
            qreal distSq = dx * dx + dy * dy;
            qreal contactDist = ball->radius() + hazard->radius();
            if (distSq > contactDist * contactDist) continue;
            ball->applyDebuff(hazard->hazardType);
            hazard->onEaten(ball);
        }
    }

    // 6.4 Ball ↔ EjectBall 碰撞
    for (Ball* ball : allBalls) {
        if (!ball->isAlive()) continue;
        for (EjectBall* eb : ejectBalls) {
            if (!eb->isAlive()) continue;
            qreal dx = ball->x() - eb->x();
            qreal dy = ball->y() - eb->y();
            qreal distSq = dx * dx + dy * dy;
            qreal contactDist = ball->radius() + eb->radius();
            if (distSq > contactDist * contactDist) continue;
            if (ball->radius() > eb->radius() * GameConstants::EAT_RATIO) {
                ball->eat(eb);
                if (ball->isPlayer) score += 0.5;
            }
        }
    }

    // 6.5 Ball ↔ Ball 碰撞（同源合并 + 吞食）
    for (Ball* ball1 : allBalls) {
        if (!ball1->isAlive()) continue;
        QList<Entity*> nearby = m_spatialGrid.getNearby(ball1);
        for (Entity* e : nearby) {
            // 只处理球体之间的碰撞
            if (e->entityType() != EntityType::Ball) continue;
            Ball* ball2 = static_cast<Ball*>(e);
            if (ball2 == ball1 || !ball2->isAlive()) continue;
            // 避免重复处理（只处理 ball1 < ball2 的组合）
            if (ball1 > ball2) continue;

            qreal dx = ball1->x() - ball2->x();
            qreal dy = ball1->y() - ball2->y();
            qreal distSq = dx * dx + dy * dy;
            qreal contactDist = ball1->radius() + ball2->radius();
            if (distSq > contactDist * contactDist) continue;

            bool sameOwner = (ball1->isPlayer && ball2->isPlayer)
                          || (ball1->aiId > 0 && ball1->aiId == ball2->aiId);

            if (sameOwner && ball1->mergeTimer <= 0 && ball2->mergeTimer <= 0) {
                // 同源合并：大球吃小球
                if (ball1->radius() >= ball2->radius()) {
                    ball1->eat(ball2);
                } else {
                    ball2->eat(ball1);
                }
            } else if (!sameOwner) {
                // 吞食：半径 > 目标×1.1 且目标无护盾
                if (ball1->radius() > ball2->radius() * GameConstants::EAT_RATIO && !ball2->hasShield()) {
                    ball1->eat(ball2);
                    if (ball1->isPlayer) score += ball2->radius() * 0.5;
                } else if (ball2->radius() > ball1->radius() * GameConstants::EAT_RATIO && !ball1->hasShield()) {
                    ball2->eat(ball1);
                    if (ball2->isPlayer) score += ball1->radius() * 0.5;
                }
            }
        }
    }
}

// ===== 同源球体吸引力 =====
// 同玩家/AI 组的球体之间产生吸引力，随时间增加逐渐靠近合并
void GameScene::applyAttraction(qreal dt)
{
    QList<Ball*> allBalls;
    allBalls.append(playerBalls);
    allBalls.append(aiBalls);

    for (int i = 0; i < allBalls.size(); ++i) {
        Ball* b1 = allBalls[i];
        if (!b1->isAlive() || b1->splitTimer > 0) continue;
        for (int j = i + 1; j < allBalls.size(); ++j) {
            Ball* b2 = allBalls[j];
            if (!b2->isAlive() || b2->splitTimer > 0) continue;

            bool sameOwner = (b1->isPlayer && b2->isPlayer)
                          || (b1->aiId > 0 && b1->aiId == b2->aiId);
            if (!sameOwner) continue;

            qreal dx = b2->x() - b1->x();
            qreal dy = b2->y() - b1->y();
            qreal dist = std::sqrt(dx * dx + dy * dy);
            if (dist < 1e-6) continue;

            // 吸引力 = 常数引力 + 距离引力 + 时间衰减引力
            qreal attraction = 50.0 + 0.001 * dist * dist
                             + 500.0 / (b1->mergeTimer + b2->mergeTimer + 1.0);

            qreal nx = dx / dist;
            qreal ny = dy / dist;
            qreal moveStep = attraction * dt / 2;

            b1->setPos(b1->x() + nx * moveStep, b1->y() + ny * moveStep);
            b2->setPos(b2->x() - nx * moveStep, b2->y() - ny * moveStep);
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

    removeFromList(playerBalls);
    removeFromList(aiBalls);
    removeFromList(foods);
    removeFromList(skillBalls);
    removeFromList(hazards);
    removeFromList(ejectBalls);
}
