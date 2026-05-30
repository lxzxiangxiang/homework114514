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

public:
    enum State : uint8_t { Menu, Playing, Paused, GameOver, Victory };

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

    QStringList m_backgroundFiles;
    int m_currentBgIndex = -1;
    QPixmap m_bgPixmap;
    bool m_bgLoaded = false;

    QPixmap m_menuBgPixmap;
};