#pragma once

#include <QObject>

class GameScene;

class UIManager : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(UIManager)

public:
    explicit UIManager(GameScene* scene, QObject* parent = nullptr);

    void showMenu() {}
    void updateHUD(qreal score, qreal survivalTime, qreal totalMass,
                   int aiCount, const QString& effects, bool canSplit);
    void setScene(GameScene* scene);

private:
    GameScene* m_scene;
};
