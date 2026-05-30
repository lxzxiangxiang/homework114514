#pragma once

#include <QObject>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>

class GameScene;

class UIManager : public QObject {
    Q_OBJECT

public:
    explicit UIManager(GameScene* scene, QObject* parent = nullptr);
    ~UIManager();

    void createMenuItems();
    void showMenu();
    void showPause(const QRectF& sceneRect);
    void showGameOver(int score, int survivalTime);
    void showVictory(int score, int survivalTime);
    void hideAllUI();
    void updateHUD(qreal score, qreal survivalTime, qreal totalMass,
                   int aiCount, const QString& effects, bool canSplit);
    void clearOverlayRefs();
    void setScene(GameScene* scene);

private:
    void showResultOverlay(QGraphicsRectItem*& overlay, QGraphicsTextItem*& titleItem,
                           const QString& titleText, const QColor& titleColor,
                           int score, int survivalTime);

    GameScene* m_scene;

    QGraphicsTextItem* m_gameOverText = nullptr;
    QGraphicsTextItem* m_victoryText = nullptr;
    QGraphicsRectItem* m_gameOverOverlay = nullptr;
    QGraphicsRectItem* m_victoryOverlay = nullptr;
};