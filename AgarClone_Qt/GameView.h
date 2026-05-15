#pragma once

#include <QGraphicsView>
#include <QTimer>
#include <QKeyEvent>
#include <QSet>

class GameScene;
class UIManager;

class GameView : public QGraphicsView {
    Q_OBJECT

public:
    enum State { Menu, Playing, Paused, GameOver, Victory };

    explicit GameView(QWidget* parent = nullptr);
    ~GameView();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void advanceGame();

private:
    GameScene* m_gameScene = nullptr;
    UIManager* m_uiManager = nullptr;
    QTimer* m_gameTimer = nullptr;
    State m_state = State::Menu;
    QSet<int> m_keysPressed;

    void startGame();
    void pauseGame();
    void resumeGame();
    void gameOver();
    void victory();
    void returnToMenu();
    void updateCamera();
    void processPlayerInput();
};
