#pragma once

#include <QGraphicsView>
#include <QTimer>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QSet>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>

class GameScene;

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
    qreal m_currentZoom = 1.5f;
    bool m_splitRequested = false;

    QGraphicsRectItem* m_menuBackground = nullptr;
    QGraphicsRectItem* m_pauseOverlay = nullptr;
    QGraphicsTextItem* m_menuTitle = nullptr;
    QGraphicsTextItem* m_menuHint = nullptr;
    QGraphicsTextItem* m_gameOverText = nullptr;
    QGraphicsTextItem* m_victoryText = nullptr;
    QGraphicsRectItem* m_gameOverOverlay = nullptr;
    QGraphicsRectItem* m_victoryOverlay = nullptr;

    void createMenuItems();
    void showMenu();
    void showPause();
    void showGameOver(int score, int survivalTime);
    void showVictory(int score, int survivalTime);
    void hideAllUI();
    void updateHUD(qreal score, qreal survivalTime, qreal totalMass,
                   int aiCount, const QString& effects, bool canSplit);
    void showResultOverlay(QGraphicsRectItem*& overlay, QGraphicsTextItem*& titleItem,
                           const QString& titleText, const QColor& titleColor,
                           int score, int survivalTime);

    void startGame();
    void pauseGame();
    void resumeGame();
    void gameOver();
    void victory();
    void returnToMenu();
    void updateCamera();
    void processPlayerInput();
};
