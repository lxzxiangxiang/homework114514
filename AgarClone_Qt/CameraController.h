#pragma once

#include <QGraphicsView>

class GameScene;

class CameraController {
public:
    explicit CameraController(QGraphicsView* view, GameScene* scene);

    void updateCamera();
    void reset();
    void initForStart();
    void setScene(GameScene* scene);

private:
    struct ViewTarget { QPointF center; qreal zoom; };
    ViewTarget computeViewTarget() const;

    QGraphicsView* m_view;
    GameScene* m_scene;
    qreal m_currentZoom = 1.5f;
};