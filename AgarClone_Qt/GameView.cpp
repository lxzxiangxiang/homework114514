// 游戏视图(GameView)实现 — 主窗口、键盘输入、摄像机、状态机
// 管理游戏 5 种状态(Menu/Playing/Paused/GameOver/Victory) 和 HUD 数据聚合
#include "GameView.h"
#include "GameScene.h"
#include "AIController.h"
#include "Constants.h"
#include "Ball.h"

#include <QApplication>
#include <QPainter>
#include <QtMath>

GameView::GameView(QWidget* parent)
    : QGraphicsView(parent)
{
    // 创建游戏场景
    m_gameScene = new GameScene(this);
    setScene(m_gameScene);
    createMenuItems();

    // 窗口配置
    setWindowTitle(QStringLiteral("球球大乱斗"));
    resize(GameConstants::Window::WIDTH, GameConstants::Window::HEIGHT);
    setBackgroundBrush(QColor(30, 30, 30));

    // 禁用抗锯齿（提高性能）、隐藏滚动条、全视口更新模式
    setRenderHint(QPainter::Antialiasing, false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // 创建游戏主循环计时器（16ms = 约 60 FPS）
    m_gameTimer = new QTimer(this);
    m_gameTimer->setInterval(GameConstants::Loop::FRAME_INTERVAL_MS);
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

    // 3. 计算玩家总半径并聚合效果数据（一次遍历）
    qreal totalMass = 0;
    bool canSplit = false;
    int activeCount = 0;
    QStringList effectParts;
    QSet<EffectType> seenEffects;

    for (Ball* ball : m_gameScene->playerBalls) {
        if (!ball->isAlive()) continue;
        totalMass += ball->mass();
        activeCount++;
        if (ball->radius() >= GameConstants::Ball::SPLIT_THRESHOLD) canSplit = true;

        for (const auto& ae : ball->effects()) {
            if (ae.type != EffectType::None && !seenEffects.contains(ae.type)) {
                seenEffects.insert(ae.type);
                QString name;
                switch (ae.type) {
                case EffectType::Speed:     name = QStringLiteral("加速"); break;
                case EffectType::Shield:    name = QStringLiteral("护盾"); break;
                case EffectType::Grow:      name = QStringLiteral("巨大"); break;
                case EffectType::Invisible: name = QStringLiteral("隐身"); break;
                case EffectType::Magnet:    name = QStringLiteral("磁力"); break;
                case EffectType::Bomb:      name = QStringLiteral("炸弹"); break;
                case EffectType::Trap:      name = QStringLiteral("陷阱"); break;
                case EffectType::Poison:    name = QStringLiteral("中毒"); break;
                default: break;
                }
                effectParts.append(name + QString("(%1s)").arg(ae.timer, 0, 'f', 1));
            }
        }
    }

    if (m_gameScene->playerBalls.isEmpty() || totalMass <= 0) {
        gameOver();
        return;
    }

    qreal equivalentRadius = std::sqrt(totalMass / M_PI);
    if (equivalentRadius >= GameConstants::Gameplay::VICTORY_TOTAL_RADIUS) {
        victory();
        return;
    }

    updateCamera();

    QString effects = effectParts.isEmpty() ? QStringLiteral("无") : effectParts.join(QStringLiteral(", "));

    updateHUD(
        m_gameScene->score,
        m_gameScene->survivalTime,
        totalMass,
        m_gameScene->aiBalls.size(),
        effects,
        canSplit
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

void GameView::wheelEvent(QWheelEvent* event)
{
    Q_UNUSED(event);
}

void GameView::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);
    if (!m_gameScene || m_state != State::Playing) return;

    painter->save();
    painter->resetTransform();

    painter->setRenderHint(QPainter::Antialiasing, true);
    QFont font("Arial", GameConstants::HUD::FONT_SIZE);
    painter->setFont(font);
    painter->setPen(Qt::white);

    const int margin = GameConstants::HUD::MARGIN;
    const int lineHeight = GameConstants::HUD::LINE_HEIGHT;
    const int maxWidth = GameConstants::HUD::MAX_WIDTH;

    QRectF lineRect(margin, margin, maxWidth, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->hudScoreText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->hudTimeText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->hudRadiusText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->hudAICountText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->hudEffectsText);
    lineRect.translate(0, lineHeight);
    painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->hudSplitText);

    painter->restore();
}

// 开始游戏：重建场景和 UI，进入 Playing 状态
void GameView::startGame()
{
    for (Ball* ai : m_gameScene->aiBalls) {
        AIController::resetState(ai);
    }
    setScene(nullptr);

    m_pauseOverlay    = nullptr;
    m_gameOverOverlay = nullptr;
    m_gameOverText    = nullptr;
    m_victoryOverlay  = nullptr;
    m_victoryText     = nullptr;
    m_menuBackground  = nullptr;
    m_menuTitle       = nullptr;
    m_menuHint        = nullptr;

    delete m_gameScene;
    m_gameScene = nullptr;

    m_gameScene = new GameScene(this);
    setScene(m_gameScene);
    createMenuItems();

    resetTransform();
    m_keysPressed.clear();
    m_state = State::Playing;
    m_gameTimer->start();
}

// 暂停游戏：切换状态，停止计时器，显示暂停界面
void GameView::pauseGame()
{
    m_state = State::Paused;
    m_gameTimer->stop();
    resetTransform();
    centerOn(GameConstants::Window::WIDTH / 2, GameConstants::Window::HEIGHT / 2);
    showPause();
}

// 继续游戏：切换状态，启动计时器，隐藏界面
void GameView::resumeGame()
{
    m_state = State::Playing;
    m_gameTimer->start();
    hideAllUI();
}

// 游戏结束：停止计时器，显示结束界面（分数 + 生存时间）
void GameView::gameOver()
{
    m_state = State::GameOver;
    m_gameTimer->stop();
    resetTransform();
    centerOn(GameConstants::Window::WIDTH / 2, GameConstants::Window::HEIGHT / 2);
    showGameOver(
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
    centerOn(GameConstants::Window::WIDTH / 2, GameConstants::Window::HEIGHT / 2);
    showVictory(
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
    centerOn(GameConstants::Window::WIDTH / 2, GameConstants::Window::HEIGHT / 2);
    showMenu();
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
    qreal lerpFactor = GameConstants::Camera::CENTER_LERP;
    QPointF smoothCenter = currentCenter + (targetCenter - currentCenter) * lerpFactor;

    // 3. 缓动缩放：依据总质量
    qreal totalMass = 0;
    for (const Ball* ball : balls) {
        if (ball->isAlive()) totalMass += ball->radius() * ball->radius();
    }
    qreal equivalentRadius = (totalMass > 0) ? std::sqrt(totalMass) : GameConstants::World::MIN_RADIUS;
    equivalentRadius = qMax(equivalentRadius, GameConstants::World::MIN_RADIUS);

    qreal targetZoom = GameConstants::Camera::ZOOM_MAX * (GameConstants::World::MIN_RADIUS / equivalentRadius);
    targetZoom = qBound(GameConstants::Camera::ZOOM_MIN, targetZoom, GameConstants::Camera::ZOOM_MAX);
    m_currentZoom += (targetZoom - m_currentZoom) * GameConstants::Camera::ZOOM_LERP;

    // 4. 应用变换
    resetTransform();
    scale(m_currentZoom, m_currentZoom);
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

    bool spaceDown = m_keysPressed.contains(Qt::Key_Space);
    bool eDown = m_keysPressed.contains(Qt::Key_E);

    if (spaceDown && !m_splitFired) {
        m_gameScene->wantSplit = true;
        m_splitFired = true;
    }
    if (!spaceDown) m_splitFired = false;

    if (eDown && !m_ejectFired) {
        m_gameScene->wantEject = true;
        m_ejectFired = true;
    }
    if (!eDown) m_ejectFired = false;
}

void GameView::createMenuItems()
{
    const qreal w = GameConstants::Window::WIDTH;
    const qreal h = GameConstants::Window::HEIGHT;

    m_menuBackground = new QGraphicsRectItem(0, 0, w, h);
    m_menuBackground->setBrush(QBrush(QColor(0, 0, 0, 180)));
    m_menuBackground->setPen(Qt::NoPen);
    m_menuBackground->setZValue(200);
    m_menuBackground->setVisible(false);
    m_gameScene->addItem(m_menuBackground);

    QFont titleFont("Arial", 48, QFont::Bold);
    m_menuTitle = new QGraphicsTextItem(QStringLiteral("球球大乱斗"));
    m_menuTitle->setFont(titleFont);
    m_menuTitle->setDefaultTextColor(Qt::white);
    m_menuTitle->setZValue(201);
    m_menuTitle->setVisible(false);
    m_gameScene->addItem(m_menuTitle);
    QRectF titleRect = m_menuTitle->boundingRect();
    m_menuTitle->setPos((w - titleRect.width()) / 2.0, h * 0.2);

    QFont hintFont("Arial", 14);
    QString hintText = QStringLiteral(
        "WASD - 移动\n"
        "空格 - 分裂\n"
        "E - 吐孢\n"
        "ESC - 暂停\n\n"
        "按 Enter 开始");
    m_menuHint = new QGraphicsTextItem(hintText);
    m_menuHint->setFont(hintFont);
    m_menuHint->setDefaultTextColor(QColor(200, 200, 200));
    m_menuHint->setZValue(201);
    m_menuHint->setVisible(false);
    m_gameScene->addItem(m_menuHint);
    QRectF hintRect = m_menuHint->boundingRect();
    m_menuHint->setPos((w - hintRect.width()) / 2.0, h * 0.45);
}

void GameView::showMenu()
{
    m_menuBackground->setVisible(true);
    m_menuTitle->setVisible(true);
    m_menuHint->setVisible(true);
    if (m_pauseOverlay)     m_pauseOverlay->setVisible(false);
    if (m_gameOverText)     m_gameOverText->setVisible(false);
    if (m_gameOverOverlay)  m_gameOverOverlay->setVisible(false);
    if (m_victoryText)      m_victoryText->setVisible(false);
    if (m_victoryOverlay)   m_victoryOverlay->setVisible(false);
}

void GameView::showPause()
{
    if (!m_pauseOverlay) {
        const qreal w = GameConstants::Window::WIDTH;
        const qreal h = GameConstants::Window::HEIGHT;
        m_pauseOverlay = new QGraphicsRectItem(0, 0, w, h);
        m_pauseOverlay->setBrush(QBrush(QColor(0, 0, 0, 150)));
        m_pauseOverlay->setPen(Qt::NoPen);
        m_pauseOverlay->setZValue(250);
        m_gameScene->addItem(m_pauseOverlay);

        QFont pauseFont("Arial", 36, QFont::Bold);
        auto* pauseLabel = new QGraphicsTextItem(QStringLiteral("已暂停"), m_pauseOverlay);
        pauseLabel->setFont(pauseFont);
        pauseLabel->setDefaultTextColor(Qt::white);
        QRectF rect = pauseLabel->boundingRect();
        pauseLabel->setPos((w - rect.width()) / 2.0, h * 0.35);

        QFont smallFont("Arial", 14);
        auto* pauseHint = new QGraphicsTextItem(QStringLiteral("ESC 继续 / M 回菜单"), m_pauseOverlay);
        pauseHint->setFont(smallFont);
        pauseHint->setDefaultTextColor(QColor(200, 200, 200));
        QRectF hintRect = pauseHint->boundingRect();
        pauseHint->setPos((w - hintRect.width()) / 2.0, h * 0.5);
    }
    m_pauseOverlay->setVisible(true);
}

void GameView::hideAllUI()
{
    m_menuBackground->setVisible(false);
    m_menuTitle->setVisible(false);
    m_menuHint->setVisible(false);
    if (m_pauseOverlay)     m_pauseOverlay->setVisible(false);
    if (m_gameOverText)     m_gameOverText->setVisible(false);
    if (m_gameOverOverlay)  m_gameOverOverlay->setVisible(false);
    if (m_victoryText)      m_victoryText->setVisible(false);
    if (m_victoryOverlay)   m_victoryOverlay->setVisible(false);
}

void GameView::showResultOverlay(QGraphicsRectItem*& overlay, QGraphicsTextItem*& titleItem,
                                  const QString& titleText, const QColor& titleColor,
                                  int score, int survivalTime)
{
    if (overlay) { m_gameScene->removeItem(overlay); delete overlay; overlay = nullptr; titleItem = nullptr; }

    const qreal w = GameConstants::Window::WIDTH;
    const qreal h = GameConstants::Window::HEIGHT;

    overlay = new QGraphicsRectItem(0, 0, w, h);
    overlay->setBrush(QBrush(QColor(0, 0, 0, 180)));
    overlay->setPen(Qt::NoPen);
    overlay->setZValue(300);
    m_gameScene->addItem(overlay);

    QFont titleFont("Arial", 42, QFont::Bold);
    titleItem = new QGraphicsTextItem(titleText, overlay);
    titleItem->setFont(titleFont);
    titleItem->setDefaultTextColor(titleColor);
    QRectF tr = titleItem->boundingRect();
    titleItem->setPos((w - tr.width()) / 2.0, h * 0.2);

    QFont infoFont("Arial", 18);
    int minutes = survivalTime / 60;
    int seconds = survivalTime % 60;
    QString infoText = QStringLiteral("最终分数: %1\n生存时间: %2:%3")
        .arg(score).arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
    auto* info = new QGraphicsTextItem(infoText, overlay);
    info->setFont(infoFont);
    info->setDefaultTextColor(Qt::white);
    QRectF ir = info->boundingRect();
    info->setPos((w - ir.width()) / 2.0, h * 0.4);

    QFont hintFont("Arial", 14);
    auto* hint = new QGraphicsTextItem(QStringLiteral("Enter 重新开始 / M 回菜单"), overlay);
    hint->setFont(hintFont);
    hint->setDefaultTextColor(QColor(200, 200, 200));
    QRectF hr = hint->boundingRect();
    hint->setPos((w - hr.width()) / 2.0, h * 0.6);
}

void GameView::showGameOver(int score, int survivalTime)
{
    showResultOverlay(m_gameOverOverlay, m_gameOverText,
                      QStringLiteral("游戏结束"), QColor(255, 80, 80), score, survivalTime);
}

void GameView::showVictory(int score, int survivalTime)
{
    showResultOverlay(m_victoryOverlay, m_victoryText,
                      QStringLiteral("胜利!"), QColor(80, 255, 80), score, survivalTime);
}

void GameView::updateHUD(qreal score, qreal survivalTime, qreal totalMass,
                          int aiCount, const QString& effects, bool canSplit)
{
    int minutes = static_cast<int>(survivalTime) / 60;
    int seconds = static_cast<int>(survivalTime) % 60;

    m_gameScene->hudScoreText   = QStringLiteral("分数: %1").arg(score, 0, 'f', 1);
    m_gameScene->hudTimeText    = QStringLiteral("时间: %1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
    m_gameScene->hudRadiusText  = QStringLiteral("总质量: %1").arg(totalMass, 0, 'f', 1);
    m_gameScene->hudAICountText = QStringLiteral("AI: %1").arg(aiCount);
    m_gameScene->hudEffectsText = QStringLiteral("效果: ") + effects;
    m_gameScene->hudSplitText   = canSplit ? QStringLiteral("分裂: 可") : QStringLiteral("分裂: 不可");
}
