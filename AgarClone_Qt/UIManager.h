#pragma once

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QTimer>

class UIManager : public QObject {
    Q_OBJECT

public:
    explicit UIManager(QGraphicsScene* scene, QGraphicsView* view, QObject* parent = nullptr);

    void showMenu();
    void showPause();
    void showGameOver(int score, int survivalTime);
    void showVictory(int score, int survivalTime);
    void hideAll();
    void updateHUD(qreal score, qreal survivalTime, qreal totalMass, qreal avgRadius,
                   int aiCount, bool canSplit, const QString& skillInfo,
                   const QString& debuffInfo, const QString& invincibleInfo);

private:
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;

    QGraphicsRectItem* m_menuBackground = nullptr;
    QGraphicsRectItem* m_pauseOverlay = nullptr;
    QGraphicsTextItem* m_menuTitle = nullptr;
    QGraphicsTextItem* m_menuHint = nullptr;
    QGraphicsTextItem* m_hudScore = nullptr;
    QGraphicsTextItem* m_hudTime = nullptr;
    QGraphicsTextItem* m_hudMass = nullptr;
    QGraphicsTextItem* m_hudRadius = nullptr;
    QGraphicsTextItem* m_hudAICount = nullptr;
    QGraphicsTextItem* m_hudSplitStatus = nullptr;
    QGraphicsTextItem* m_hudSkill = nullptr;
    QGraphicsTextItem* m_hudDebuff = nullptr;
    QGraphicsTextItem* m_hudInvincible = nullptr;
    QGraphicsTextItem* m_gameOverText = nullptr;
    QGraphicsTextItem* m_victoryText = nullptr;
    QGraphicsRectItem* m_gameOverOverlay = nullptr;
    QGraphicsRectItem* m_victoryOverlay = nullptr;

    void createHUDItems();
    void createMenuItems();
};
