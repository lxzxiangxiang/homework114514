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
        QString picDir = findAssetDir({
            QStringLiteral("assets/gamepicture"),
            QStringLiteral("../AgarClone_Qt/assets/gamepicture"),
            QStringLiteral("../../AgarClone_Qt/assets/gamepicture")
        });

        auto loadPic = [&](QPixmap& pm, const QString& name) {
            if (!picDir.isEmpty()) {
                QString path = picDir + QStringLiteral("/") + name;
                if (QFileInfo::exists(path) && pm.load(path)) {
                    pm = pm.scaled(
                        GameConstants::Window::WIDTH, GameConstants::Window::HEIGHT,
                        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                }
            }
        };

        loadPic(m_menuBgPixmap,    QStringLiteral("menu.jpg"));
        loadPic(m_pauseBgPixmap,   QStringLiteral("pause.jpg"));
        loadPic(m_gameOverBgPixmap, QStringLiteral("gameover.jpg"));
        loadPic(m_victoryBgPixmap,  QStringLiteral("victory.jpg"));
    }

    // 初始显示主菜单
    returnToMenu();

    scanBackgroundFolder();
    for (const auto& path : m_backgroundFiles) {
        QPixmap pm;
        if (pm.load(path)) m_bgCache.insert(path, pm);
    }
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
    if (m_preloadedScene) {
        delete m_preloadedScene;
        m_preloadedScene = nullptr;
    }
}

// ===== 每帧回调：游戏主循环驱动 =====
void GameView::advanceGame()
{
    if (m_state != State::Playing) {
        return;                                         // 非游戏中状态不更新
    }

    if (m_gameJustStarted) {
        m_gameJustStarted = false;
        if (m_soundManager) {
            m_soundManager->playGameMusic();
            m_soundManager->playStartGameSound();
        }
    }

    // 1. 处理玩家键盘输入（WASD/方向键/空格/E）
    processPlayerInput();
    // 2. 更新游戏逻辑
    m_gameScene->updateGame(GameConstants::Loop::FRAME_INTERVAL_MS / 1000.0);

    // 3. 计算玩家总半径并聚合效果数据（一次遍历）
    qreal totalMass = 0;
    bool canSplit = false;
    int activeCount = 0;
    QStringList effectParts;
    QSet<EffectType> seenEffects;

    for (Ball* ball : m_gameScene->m_playerBalls) {
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

    if (m_gameScene->m_playerBalls.isEmpty() || totalMass <= 0) {
        gameOver();
        return;
    }

    qreal equivalentRadius = std::sqrt(totalMass / M_PI);
    if (equivalentRadius >= GameConstants::Gameplay::VICTORY_TOTAL_RADIUS) {
        victory();
        return;
    }

    m_camera->updateCamera();

    QString effects = effectParts.isEmpty() ? QStringLiteral("无") : effectParts.join(QStringLiteral(", "));

    m_ui->updateHUD(
        m_gameScene->m_score,
        m_gameScene->m_survivalTime,
        totalMass,
        m_gameScene->m_aiBalls.size(),
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
                   || key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_Left || key == Qt::Key_Right) {
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
        painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->m_hudScoreText);
        lineRect.translate(0, lineHeight);
        painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->m_hudTimeText);
        lineRect.translate(0, lineHeight);
        painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->m_hudRadiusText);
        lineRect.translate(0, lineHeight);
        painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->m_hudAICountText);
        lineRect.translate(0, lineHeight);
        painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->m_hudEffectsText);
        lineRect.translate(0, lineHeight);
        painter->drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, m_gameScene->m_hudSplitText);
    } else if (m_state == State::Paused) {
        const qreal w = width();
        const qreal h = height();

        painter->setRenderHint(QPainter::SmoothPixmapTransform);
        if (!m_pauseBgPixmap.isNull()) {
            painter->drawPixmap(0, 0, w, h, m_pauseBgPixmap);
        }
    } else if (m_state == State::GameOver || m_state == State::Victory) {
        const qreal w = width();
        const qreal h = height();
        QPixmap& bg = m_isVictory ? m_victoryBgPixmap : m_gameOverBgPixmap;

        painter->setRenderHint(QPainter::SmoothPixmapTransform);

        if (!bg.isNull()) {
            painter->drawPixmap(0, 0, w, h, bg);
        } else {
            painter->fillRect(0, 0, w, h, QColor(0, 0, 0, 200));
        }

        painter->setRenderHint(QPainter::Antialiasing, true);

        int minutes = m_resultSurvivalTime / 60;
        int seconds = m_resultSurvivalTime % 60;

        QFont infoFont("Microsoft YaHei", 22, QFont::Bold);
        painter->setFont(infoFont);
        QString scoreText = QStringLiteral("%1").arg(m_resultScore);
        QString timeText  = QStringLiteral("%1:%2")
            .arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));

        QRectF scoreRect(w * GameConstants::HUD::Result::SCORE_LEFT, h * GameConstants::HUD::Result::SCORE_TOP,
                         w * GameConstants::HUD::Result::ITEM_WIDTH, h * GameConstants::HUD::Result::ITEM_HEIGHT);
        QRectF timeRect (w * GameConstants::HUD::Result::TIME_LEFT, h * GameConstants::HUD::Result::TIME_TOP,
                         w * GameConstants::HUD::Result::ITEM_WIDTH, h * GameConstants::HUD::Result::ITEM_HEIGHT);

        painter->setPen(Qt::white);
        painter->drawText(scoreRect, Qt::AlignLeft | Qt::AlignVCenter, scoreText);
        painter->drawText(timeRect,  Qt::AlignLeft | Qt::AlignVCenter, timeText);

        if (bg.isNull()) {
            QFont hintFont("Microsoft YaHei", 14);
            painter->setFont(hintFont);
            painter->setPen(QColor(200, 200, 200, 220));
            QRectF hintRect(w * 0.30, h * 0.770, w * 0.45, h * 0.055);
            painter->drawText(hintRect, Qt::AlignCenter,
                               QStringLiteral("Enter 重新开始   M 返回菜单"));
        }
    }

    painter->restore();
}

// 开始游戏：重建场景和 UI，进入 Playing 状态
void GameView::startGame()
{
    AIController::resetAll();
    m_ui->clearOverlayRefs();

    m_camera->initForStart();
    m_keysPressed.clear();
    m_splitRequested = false;

    selectRandomBackground();
    if (m_currentBgIndex >= 0 && m_currentBgIndex < m_backgroundFiles.size()) {
        const auto& path = m_backgroundFiles.at(m_currentBgIndex);
        auto it = m_bgCache.find(path);
        if (it != m_bgCache.end()) {
            m_bgPixmap = it.value();
        } else if (!m_bgPixmap.load(path)) {
            qWarning() << "GameView: failed to load background" << path;
            m_bgPixmap = QPixmap();
        }
    }

    m_state = State::Playing;
    m_gameJustStarted = true;
    m_gameTimer->start();

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
    enterResultState(State::GameOver, false);
}

void GameView::victory()
{
    enterResultState(State::Victory, true);
}

void GameView::enterResultState(State targetState, bool isVictory)
{
    m_state = targetState;
    m_gameTimer->stop();
    m_camera->reset();
    centerOn(GameConstants::Window::WIDTH / 2, GameConstants::Window::HEIGHT / 2);
    m_resultScore = static_cast<int>(m_gameScene->m_score);
    m_resultSurvivalTime = static_cast<int>(m_gameScene->m_survivalTime);
    m_isVictory = isVictory;
    if (m_soundManager) {
        if (isVictory) {
            m_soundManager->playVictoryMusic();
        } else {
            m_soundManager->playGameOverMusic();
        }
    }
    preloadAndSwitchScene();
}

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

    preloadAndSwitchScene();
}

void GameView::preloadAndSwitchScene()
{
    if (!m_preloadedScene) {
        m_preloadedScene = new GameScene(this);

        setUpdatesEnabled(false);
        if (m_gameScene && m_gameScene != m_preloadedScene) {
            m_gameScene->deleteLater();
        }
        m_gameScene = m_preloadedScene;
        m_preloadedScene = nullptr;
        setScene(m_gameScene);
        m_ui->setScene(m_gameScene);
        m_camera->setScene(m_gameScene);
        setUpdatesEnabled(true);
    }
}

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

    m_gameScene->m_playerInputDirection = QPointF(dx, dy);

    if (m_splitRequested) {
        m_gameScene->m_wantSplit = true;
        m_splitRequested = false;
    }
}

void GameView::scanBackgroundFolder()
{
    m_backgroundFiles.clear();

    QString bgDirPath = findAssetDir({
        QStringLiteral("assets/backgrounds"),
        QStringLiteral("../AgarClone_Qt/assets/backgrounds"),
        QStringLiteral("../../AgarClone_Qt/assets/backgrounds")
    });

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
    QString bgmDir = findAssetDir({
        QStringLiteral("assets/bgm"),
        QStringLiteral("../AgarClone_Qt/assets/bgm"),
        QStringLiteral("../../AgarClone_Qt/assets/bgm"),
        QStringLiteral("../../../AgarClone_Qt/assets/bgm")
    });

    if (bgmDir.isEmpty()) return;

    m_soundManager = new SoundManager(bgmDir, this);
}

void GameView::connectSplitSignal()
{
    connect(m_gameScene, &GameScene::splitOccurred, this, [this]() {
        if (m_soundManager) m_soundManager->playSplitSound();
    }, Qt::UniqueConnection);
}

QString GameView::findAssetDir(const QStringList& relativePaths)
{
    QString appDir = QCoreApplication::applicationDirPath();
    for (const auto& rel : relativePaths) {
        QString full = appDir + QStringLiteral("/") + rel;
        if (QDir(full).exists()) return full;
    }
    return {};
}