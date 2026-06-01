#pragma once

#include <QGraphicsView>
#include <QTimer>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QSet>
#include <QStringList>
#include <QPixmap>

class GameScene;
class SoundManager;
class CameraController;
class UIManager;

class GameView : public QGraphicsView {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(GameView)

public:
    enum State : uint8_t { Menu, Playing, Paused, GameOver, Victory, };

    explicit GameView(QWidget* parent = nullptr);
    ~GameView();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private slots:
    void advanceGame();

private:
    GameScene* m_gameScene = nullptr;
    QTimer* m_gameTimer = nullptr;
    State m_state = State::Menu;
    QSet<int> m_keysPressed;
    bool m_splitRequested = false;

    CameraController* m_camera = nullptr;
    UIManager* m_ui = nullptr;
    SoundManager* m_soundManager = nullptr;

    void startGame();
    void pauseGame();
    void resumeGame();
    void gameOver();
    void victory();
    void returnToMenu();
    void processPlayerInput();

    void scanBackgroundFolder();
    void selectRandomBackground();

    void initSoundManager();
    void connectSplitSignal();

    void preloadAndSwitchScene();

    static QString findAssetDir(const QStringList& relativePaths);

    QStringList m_backgroundFiles;
    int m_currentBgIndex = -1;
    QPixmap m_bgPixmap;
    QHash<QString, QPixmap> m_bgCache;

    bool m_gameJustStarted = false;

    QPixmap m_menuBgPixmap;
    QPixmap m_pauseBgPixmap;
    QPixmap m_gameOverBgPixmap;
    QPixmap m_victoryBgPixmap;
    int m_resultScore = 0;
    int m_resultSurvivalTime = 0;
    bool m_isVictory = false;
    GameScene* m_preloadedScene = nullptr;
};