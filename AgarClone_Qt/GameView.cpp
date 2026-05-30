// 游戏视图(GameView)实现 — 主窗口、键盘输入、摄像机、状态机
// 管理游戏 5 种状态(Menu/Playing/Paused/GameOver/Victory) 和 HUD 数据聚合
#include "GameView.h"
#include "GameScene.h"
#include "AIController.h"
#include "SoundManager.h"
#include "CameraController.h"
#include "UIManager.h"
#include "Constants.h"
#include "Ball.h"

#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QtMath>
#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>

GameView::GameView(QWidget* parent)
    : QGraphicsView(parent)
{
    // 创建游戏场景
    m_gameScene = new GameScene(this);
    setScene(m_gameScene);

    m_camera = new CameraController(this, m_gameScene);
    m_ui = new UIManager(m_gameScene, this);
    m_ui->createMenuItems();

    // 窗口配置
    setWindowTitle(QStringLiteral("球球大乱斗"));
    resize(GameConstants::Window::WIDTH, GameConstants::Window::HEIGHT);
    setBackgroundBrush(QColor(0, 0, 0));

    // 禁用抗锯齿（提高性能）、隐藏滚动条、全视口更新模式
    setRenderHint(QPainter::Antialiasing, false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // 创建游戏主循环计时器（16ms = 约 60 FPS）
    m_gameTimer = new QTimer(this);
    m_gameTimer->setInterval(GameConstants::Loop::FRAME_INTERVAL_MS);
    connect(m_gameTimer, &QTimer::timeout, this, &GameView::advanceGame);

    initSoundManager();

    {
        QString appDir = QCoreApplication::applicationDirPath();
        QStringList menuPaths;
        menuPaths << appDir + QStringLiteral("/assets/backgrounds/menu.png");
        menuPaths << appDir + QStringLiteral("/../AgarClone_Qt/assets/backgrounds/menu.png");
        menuPaths << appDir + QStringLiteral("/../../AgarClone_Qt/assets/backgrounds/menu.png");
        for (const auto& p : menuPaths) {
            if (QFileInfo::exists(p) && m_menuBgPixmap.load(p)) break;
        }
        if (!m_menuBgPixmap.isNull()) {
            m_menuBgPixmap = m_menuBgPixmap.scaled(
                GameConstants::Window::WIDTH, GameConstants::Window::HEIGHT,
                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
    }

    // 初始显示主菜单
    returnToMenu();

    scanBackgroundFolder();
    selectRandomBackground();

    connectSplitSignal();
}

GameView::~GameView()
{
    if (m_gameTimer) {
        m_gameTimer->stop();
    }
    delete m_camera;
    m_camera = nullptr;
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
                case EffectType::None: break;
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
    if (equivalentRadius >= GameConstants::Gameplay::VICTORY_TOTAL_RADIUS * 0.8f
        && m_soundManager) {
        m_soundManager->preloadVictoryMusic();
    }
    if (equivalentRadius >= GameConstants::Gameplay::VICTORY_TOTAL_RADIUS) {
        victory();
        return;
    }

    m_camera->updateCamera();

    QString effects = effectParts.isEmpty() ? QStringLiteral("无") : effectParts.join(QStringLiteral(", "));

    m_ui->updateHUD(
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
        } else if (key == Qt::Key_W || key == Qt::Key_A || key == Qt::Key_S || key == Qt::Key_D) {
            m_keysPressed.insert(key);
        }
        if (key == Qt::Key_Space && !event->isAutoRepeat()) {
            m_splitRequested = true;
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

void GameView::drawBackground(QPainter* painter, const QRectF& rect)
{
    painter->save();
    painter->setBrush(Qt::black);
    painter->drawRect(rect);

    if (m_currentBgIndex >= 0 && m_currentBgIndex < m_backgroundFiles.size()) {
        if (!m_bgLoaded) {
            if (!m_bgPixmap.load(m_backgroundFiles.at(m_currentBgIndex))) {
                qWarning() << "GameView: failed to load background" << m_backgroundFiles.at(m_currentBgIndex);
            }
            m_bgLoaded = true;
        }
    }
    if (!m_bgPixmap.isNull()) {
        painter->drawTiledPixmap(sceneRect(), m_bgPixmap);
    }
    painter->restore();
}

void GameView::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);
    if (!m_gameScene) return;

    painter->save();
    painter->resetTransform();

    if (m_state == State::Menu) {
        if (!m_menuBgPixmap.isNull()) {
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
            painter->drawPixmap(0, 0, viewport()->width(), viewport()->height(), m_menuBgPixmap);
        } else {
            painter->fillRect(viewport()->rect(), QColor(0, 0, 0, 180));
        }
    } else if (m_state == State::Playing) {
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
    } else if (m_state == State::Paused) {
        const qreal w = width();
        const qreal h = height();

        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->fillRect(0, 0, w, h, QColor(0, 0, 0, 150));

        QFont pauseFont("Arial", 36, QFont::Bold);
        painter->setFont(pauseFont);
        painter->setPen(Qt::white);
        painter->drawText(QRectF(0, 0, w, h * 0.5), Qt::AlignCenter, QStringLiteral("已暂停"));

        QFont hintFont("Arial", 14);
        painter->setFont(hintFont);
        painter->setPen(QColor(200, 200, 200));
        painter->drawText(QRectF(0, h * 0.5, w, h * 0.2), Qt::AlignCenter, QStringLiteral("ESC 继续 / M 回菜单"));
    }

    painter->restore();
}

// 开始游戏：重建场景和 UI，进入 Playing 状态
void GameView::startGame()
{
    for (Ball* ai : m_gameScene->aiBalls) {
        AIController::resetState(ai);
    }
    setScene(nullptr);

    m_ui->clearOverlayRefs();

    m_gameScene->deleteLater();
    m_gameScene = nullptr;

    m_gameScene = new GameScene(this);
    setScene(m_gameScene);
    m_ui->setScene(m_gameScene);
    m_camera->setScene(m_gameScene);
    m_ui->createMenuItems();

    m_camera->initForStart();
    m_keysPressed.clear();
    m_splitRequested = false;

    selectRandomBackground();
    if (m_currentBgIndex >= 0 && m_currentBgIndex < m_backgroundFiles.size()) {
        if (!m_bgPixmap.load(m_backgroundFiles.at(m_currentBgIndex))) {
            qWarning() << "GameView: failed to load background" << m_backgroundFiles.at(m_currentBgIndex);
        }
    }
    m_bgLoaded = true;

    m_state = State::Playing;
    m_gameTimer->start();

    if (m_soundManager) {
        m_soundManager->playGameMusic();
        m_soundManager->playStartGameSound();
    }
    connectSplitSignal();
}

// 暂停游戏：切换状态，停止计时器，显示暂停界面
void GameView::pauseGame()
{
    m_state = State::Paused;
    m_gameTimer->stop();
    if (m_soundManager) m_soundManager->pauseMusic();
    viewport()->update();
}

// 继续游戏：切换状态，启动计时器，隐藏界面
void GameView::resumeGame()
{
    m_state = State::Playing;
    m_gameTimer->start();
    if (m_soundManager) m_soundManager->resumeMusic();
    viewport()->update();
}

// 游戏结束：停止计时器，显示结束界面（分数 + 生存时间）
void GameView::gameOver()
{
    m_state = State::GameOver;
    m_gameTimer->stop();
    m_camera->reset();
    centerOn(GameConstants::Window::WIDTH / 2, GameConstants::Window::HEIGHT / 2);
    m_ui->showGameOver(
        static_cast<int>(m_gameScene->score),
        static_cast<int>(m_gameScene->survivalTime)
    );
    if (m_soundManager) m_soundManager->playGameOverMusic();
}

// 胜利：停止计时器，显示胜利界面
void GameView::victory()
{
    m_state = State::Victory;
    m_gameTimer->stop();
    m_camera->reset();
    centerOn(GameConstants::Window::WIDTH / 2, GameConstants::Window::HEIGHT / 2);
    m_ui->showVictory(
        static_cast<int>(m_gameScene->score),
        static_cast<int>(m_gameScene->survivalTime)
    );
    if (m_soundManager) m_soundManager->playVictoryMusic();
}

// 返回主菜单
void GameView::returnToMenu()
{
    m_state = State::Menu;
    if (m_gameTimer) {
        m_gameTimer->stop();
    }
    m_camera->reset();
    centerOn(GameConstants::Window::WIDTH / 2, GameConstants::Window::HEIGHT / 2);
    m_ui->showMenu();
    if (m_soundManager) m_soundManager->playMenuMusic();
}

// ===== 处理玩家键盘输入 =====
// 从 m_keysPressed 读取 WASD/方向键，计算归一化方向向量
// Space 设置分裂意图
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

    m_gameScene->playerInputDirection = QPointF(dx, dy);

    if (m_splitRequested) {
        m_gameScene->wantSplit = true;
        m_splitRequested = false;
    }
}

void GameView::scanBackgroundFolder()
{
    m_backgroundFiles.clear();

    QStringList searchPaths;
    searchPaths << QCoreApplication::applicationDirPath() + QStringLiteral("/assets/backgrounds/");
    searchPaths << QCoreApplication::applicationDirPath() + QStringLiteral("/../AgarClone_Qt/assets/backgrounds/");
    searchPaths << QCoreApplication::applicationDirPath() + QStringLiteral("/../../AgarClone_Qt/assets/backgrounds/");

    QString bgDirPath;
    for (const auto& path : searchPaths) {
        QDir testDir(path);
        if (testDir.exists()) {
            bgDirPath = path;
            break;
        }
    }

    if (bgDirPath.isEmpty()) return;

    QDir dir(bgDirPath);
    QStringList filters = {QStringLiteral("*.png"), QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.bmp")};
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    for (const auto& fi : files) {
        m_backgroundFiles << fi.absoluteFilePath();
    }
}

void GameView::selectRandomBackground()
{
    if (m_backgroundFiles.isEmpty()) {
        m_currentBgIndex = -1;
        return;
    }
    m_currentBgIndex = QRandomGenerator::global()->bounded(m_backgroundFiles.size());
}

void GameView::initSoundManager()
{
    QStringList searchPaths;
    searchPaths << QCoreApplication::applicationDirPath() + QStringLiteral("/assets/bgm/");
    searchPaths << QCoreApplication::applicationDirPath() + QStringLiteral("/../AgarClone_Qt/assets/bgm/");
    searchPaths << QCoreApplication::applicationDirPath() + QStringLiteral("/../../AgarClone_Qt/assets/bgm/");
    searchPaths << QCoreApplication::applicationDirPath() + QStringLiteral("/../../../AgarClone_Qt/assets/bgm/");

    QString bgmDir;
    for (const auto& path : searchPaths) {
        QDir testDir(path);
        if (testDir.exists()) {
            bgmDir = path;
            break;
        }
    }

    if (bgmDir.isEmpty()) return;

    m_soundManager = new SoundManager(bgmDir, this);
}

void GameView::connectSplitSignal()
{
    connect(m_gameScene, &GameScene::splitOccurred, this, [this]() {
        if (m_soundManager) m_soundManager->playSplitSound();
    });
}
