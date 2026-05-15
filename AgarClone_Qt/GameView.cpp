// 游戏视图(GameView)实现 — 主窗口、键盘输入、摄像机、状态机
// 管理游戏 5 种状态(Menu/Playing/Paused/GameOver/Victory) 和 HUD 数据聚合
#include "GameView.h"
#include "GameScene.h"
#include "UIManager.h"
#include "Constants.h"
#include "Ball.h"

#include <QApplication>
#include <QPainter>
#include <cmath>
#include <QtMath>

GameView::GameView(QWidget* parent)
    : QGraphicsView(parent)
{
    // 创建游戏场景
    m_gameScene = new GameScene(this);
    setScene(m_gameScene);
    // 创建 UI 管理器
    m_uiManager = new UIManager(m_gameScene, this);

    // 窗口配置
    setWindowTitle(QString::fromUtf8("Agar.io Clone"));
    resize(GameConstants::WINDOW_WIDTH, GameConstants::WINDOW_HEIGHT);
    setBackgroundBrush(Qt::black);

    // 禁用抗锯齿（提高性能）、隐藏滚动条、全视口更新模式
    setRenderHint(QPainter::Antialiasing, false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // 创建游戏主循环计时器（16ms = 约 60 FPS）
    m_gameTimer = new QTimer(this);
    m_gameTimer->setInterval(GameConstants::FRAME_INTERVAL_MS);
    connect(m_gameTimer, &QTimer::timeout, this, &GameView::advanceGame);

    // 初始显示主菜单
    returnToMenu();
}

GameView::~GameView()
{
    if (m_gameTimer) {
        m_gameTimer->stop();
    }
}

// ===== 每帧回调：游戏主循环驱动 =====
void GameView::advanceGame()
{
    if (m_state != State::Playing) {
        return;                                         // 非游戏中状态不更新
    }

    // 1. 处理玩家键盘输入（WASD/方向键/空格/E）
    processPlayerInput();
    // 2. 更新游戏逻辑
    m_gameScene->updateGame(0.016);

    // 3. 计算玩家总半径 = 所有存活玩家球体半径之和
    qreal totalRadius = 0;
    for (Ball* ball : m_gameScene->playerBalls) {
        if (ball->isAlive()) {
            totalRadius += ball->radius();
        }
    }

    // 4. 失败判定：玩家无存活球体或总半径 ≤ 0
    if (m_gameScene->playerBalls.isEmpty() || totalRadius <= 0) {
        gameOver();
        return;
    }

    // 5. 胜利判定：总半径 ≥ 2000
    if (totalRadius >= GameConstants::VICTORY_TOTAL_RADIUS) {
        victory();
        return;
    }

    // 6. 更新动态摄像机
    updateCamera();

    // 7. 聚合 HUD 数据（技能/减益/无敌信息）
    qreal maxBallRadius = 0;
    bool canSplit = false;
    QStringList skillParts;
    QStringList debuffParts;
    QString invincibleInfo;

    for (Ball* ball : m_gameScene->playerBalls) {
        if (!ball->isAlive()) continue;
        qreal r = ball->radius();
        maxBallRadius = qMax(maxBallRadius, r);
        if (r >= GameConstants::SPLIT_THRESHOLD) canSplit = true;

        // 收集当前技能名称
        if (ball->skill != SkillType::None) {
            switch (ball->skill) {
            case SkillType::Speed:     skillParts.append(QString::fromUtf8("Speed")); break;
            case SkillType::Shield:    skillParts.append(QString::fromUtf8("Shield")); break;
            case SkillType::Grow:      skillParts.append(QString::fromUtf8("Grow")); break;
            case SkillType::Invisible: skillParts.append(QString::fromUtf8("Invisible")); break;
            case SkillType::Magnet:    skillParts.append(QString::fromUtf8("Magnet")); break;
            default: break;
            }
        }

        // 收集当前负面效果名称
        if (ball->debuff != DebuffType::None) {
            switch (ball->debuff) {
            case DebuffType::Bomb:   debuffParts.append(QString::fromUtf8("Bomb")); break;
            case DebuffType::Trap:   debuffParts.append(QString::fromUtf8("Trap")); break;
            case DebuffType::Poison: debuffParts.append(QString::fromUtf8("Poison")); break;
            default: break;
            }
        }

        if (ball->isInvincible()) {
            invincibleInfo = QString::fromUtf8("Active");
        }
    }

    QString skillInfo = skillParts.isEmpty() ? QString::fromUtf8("None") : skillParts.join(QString::fromUtf8(", "));
    QString debuffInfo = debuffParts.isEmpty() ? QString::fromUtf8("None") : debuffParts.join(QString::fromUtf8(", "));
    if (invincibleInfo.isEmpty()) invincibleInfo = QString::fromUtf8("None");

    // 计算活跃玩家球体数量和平均半径
    int activeCount = 0;
    for (Ball* ball : m_gameScene->playerBalls) {
        if (ball->isAlive()) activeCount++;
    }
    qreal avgRadius = (activeCount > 0) ? totalRadius / activeCount : 0;

    // 更新 HUD 显示
    m_uiManager->updateHUD(
        m_gameScene->score,
        m_gameScene->survivalTime,
        totalRadius,
        avgRadius,
        m_gameScene->aiBalls.size(),
        canSplit,
        skillInfo,
        debuffInfo,
        invincibleInfo
    );
}

// ===== 状态机键盘处理 =====
void GameView::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();

    switch (m_state) {
    case State::Menu:
        // 菜单界面：Enter 开始游戏
        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            startGame();
        }
        break;

    case State::Playing:
        // 游戏中：ESC 暂停；WASD/Space/E 记录按键
        if (key == Qt::Key_Escape) {
            pauseGame();
        } else if (key == Qt::Key_W || key == Qt::Key_A || key == Qt::Key_S || key == Qt::Key_D
                   || key == Qt::Key_Space || key == Qt::Key_E) {
            m_keysPressed.insert(key);
        }
        break;

    case State::Paused:
        // 暂停：ESC 继续；M 回主菜单
        if (key == Qt::Key_Escape) {
            resumeGame();
        } else if (key == Qt::Key_M) {
            returnToMenu();
        }
        break;

    case State::GameOver:
    case State::Victory:
        // 结束/胜利：Enter 重新开始；M 回主菜单
        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            startGame();
        } else if (key == Qt::Key_M) {
            returnToMenu();
        }
        break;
    }

    QGraphicsView::keyPressEvent(event);
}

// 按键释放：从记录集中移除按键
void GameView::keyReleaseEvent(QKeyEvent* event)
{
    m_keysPressed.remove(event->key());
    QGraphicsView::keyReleaseEvent(event);
}

// 开始游戏：重建场景和 UI，进入 Playing 状态
void GameView::startGame()
{
    // 清理旧场景
    setScene(nullptr);
    delete m_gameScene;
    m_gameScene = nullptr;
    delete m_uiManager;
    m_uiManager = nullptr;

    // 创建新场景
    m_gameScene = new GameScene(this);
    setScene(m_gameScene);
    m_uiManager = new UIManager(m_gameScene, this);

    m_keysPressed.clear();
    m_state = State::Playing;
    m_gameTimer->start();
}

// 暂停游戏：切换状态，停止计时器，显示暂停界面
void GameView::pauseGame()
{
    m_state = State::Paused;
    m_gameTimer->stop();
    m_uiManager->showPause();
}

// 继续游戏：切换状态，启动计时器，隐藏界面
void GameView::resumeGame()
{
    m_state = State::Playing;
    m_gameTimer->start();
    m_uiManager->hideAll();
}

// 游戏结束：停止计时器，显示结束界面（分数 + 生存时间）
void GameView::gameOver()
{
    m_state = State::GameOver;
    m_gameTimer->stop();
    resetTransform();
    centerOn(GameConstants::WINDOW_WIDTH / 2, GameConstants::WINDOW_HEIGHT / 2);
    m_uiManager->showGameOver(
        static_cast<int>(m_gameScene->score),
        static_cast<int>(m_gameScene->survivalTime)
    );
}

// 胜利：停止计时器，显示胜利界面
void GameView::victory()
{
    m_state = State::Victory;
    m_gameTimer->stop();
    resetTransform();
    centerOn(GameConstants::WINDOW_WIDTH / 2, GameConstants::WINDOW_HEIGHT / 2);
    m_uiManager->showVictory(
        static_cast<int>(m_gameScene->score),
        static_cast<int>(m_gameScene->survivalTime)
    );
}

// 返回主菜单
void GameView::returnToMenu()
{
    m_state = State::Menu;
    if (m_gameTimer) {
        m_gameTimer->stop();
    }
    resetTransform();
    centerOn(GameConstants::WINDOW_WIDTH / 2, GameConstants::WINDOW_HEIGHT / 2);
    m_uiManager->showMenu();
}

// ===== 动态摄像机 =====
// 跟随玩家球体质量加权中心点，根据最大球半径动态缩放
void GameView::updateCamera()
{
    const auto& balls = m_gameScene->playerBalls;
    if (balls.isEmpty()) return;

    // 1. 计算质量加权中心点（半径作为权重）
    qreal totalWeight = 0;
    qreal cx = 0, cy = 0;

    for (const Ball* ball : balls) {
        if (!ball->isAlive()) continue;
        qreal weight = ball->radius();
        cx += ball->x() * weight;
        cy += ball->y() * weight;
        totalWeight += weight;
    }

    if (totalWeight <= 0) return;

    cx /= totalWeight;
    cy /= totalWeight;

    // 2. 平滑插值：lerp(0.1) 使摄像机平滑跟随
    QPointF targetCenter(cx, cy);
    QPointF currentCenter = mapToScene(viewport()->rect().center());
    qreal lerpFactor = 0.1;
    QPointF smoothCenter = currentCenter + (targetCenter - currentCenter) * lerpFactor;

    // 3. 动态缩放：zoom = CAMERA_ZOOM_MAX × (MIN_RADIUS / maxRadius)
    qreal maxRadius = GameConstants::MIN_RADIUS;
    for (const Ball* ball : balls) {
        if (ball->isAlive()) {
            maxRadius = qMax(maxRadius, ball->radius());
        }
    }

    qreal zoom = GameConstants::CAMERA_ZOOM_MAX * (GameConstants::MIN_RADIUS / maxRadius);
    zoom = qBound(GameConstants::CAMERA_ZOOM_MIN, zoom, GameConstants::CAMERA_ZOOM_MAX);

    // 4. 应用变换
    resetTransform();
    scale(zoom, zoom);
    centerOn(smoothCenter);
}

// ===== 处理玩家键盘输入 =====
// 从 m_keysPressed 读取 WASD/方向键，计算归一化方向向量
// Space 设置分裂意图，E 设置吐孢意图
void GameView::processPlayerInput()
{
    qreal dx = 0, dy = 0;

    if (m_keysPressed.contains(Qt::Key_W) || m_keysPressed.contains(Qt::Key_Up))    dy -= 1;
    if (m_keysPressed.contains(Qt::Key_S) || m_keysPressed.contains(Qt::Key_Down))  dy += 1;
    if (m_keysPressed.contains(Qt::Key_A) || m_keysPressed.contains(Qt::Key_Left))  dx -= 1;
    if (m_keysPressed.contains(Qt::Key_D) || m_keysPressed.contains(Qt::Key_Right)) dx += 1;

    // 归一化方向向量
    qreal len = std::sqrt(dx * dx + dy * dy);
    if (len > 0) {
        dx /= len;
        dy /= len;
    }

    // 将输入写入 GameScene
    m_gameScene->playerInputDirection = QPointF(dx, dy);
    m_gameScene->wantSplit = m_keysPressed.contains(Qt::Key_Space);
    m_gameScene->wantEject = m_keysPressed.contains(Qt::Key_E);
}
