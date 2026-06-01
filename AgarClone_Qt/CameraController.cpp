#include "CameraController.h"
#include "GameScene.h"
#include "Ball.h"
#include "Constants.h"

#include <QtMath>

CameraController::CameraController(QGraphicsView* view, GameScene* scene)
    : m_view(view)
    , m_scene(scene)
{
}

CameraController::ViewTarget CameraController::computeViewTarget() const
{
    const auto& balls = m_scene->m_playerBalls;

    qreal totalWeight = 0;
    qreal cx = 0, cy = 0;
    qreal totalMass = 0;

    for (const Ball* ball : balls) {
        if (!ball->isAlive()) continue;
        qreal weight = ball->radius();
        if (ball->isInSplitAnim()) {
            QPointF tp = ball->splitTargetPos();
            cx += tp.x() * weight;
            cy += tp.y() * weight;
        } else {
            cx += ball->x() * weight;
            cy += ball->y() * weight;
        }
        totalWeight += weight;
        totalMass += weight * weight;
    }

    if (totalWeight <= 0) return {{}, 1.5f};

    cx /= totalWeight;
    cy /= totalWeight;

    qreal equivalentRadius = (totalMass > 0) ? std::sqrt(totalMass) : GameConstants::World::MIN_RADIUS;
    equivalentRadius = (std::max)(equivalentRadius, static_cast<qreal>(GameConstants::World::MIN_RADIUS));

    qreal targetZoom = GameConstants::Camera::ZOOM_MAX * (GameConstants::World::MIN_RADIUS / equivalentRadius);
    targetZoom = std::clamp(targetZoom,
        static_cast<qreal>(GameConstants::Camera::ZOOM_MIN),
        static_cast<qreal>(GameConstants::Camera::ZOOM_MAX));

    return {QPointF(cx, cy), targetZoom};
}

void CameraController::updateCamera()
{
    ViewTarget target = computeViewTarget();
    if (target.zoom <= 0) return;

    QPointF currentCenter = m_view->mapToScene(m_view->viewport()->rect().center());
    QPointF smoothCenter = currentCenter + (target.center - currentCenter) * GameConstants::Camera::CENTER_LERP;

    m_currentZoom += (target.zoom - m_currentZoom) * GameConstants::Camera::ZOOM_LERP;

    m_view->resetTransform();
    m_view->scale(m_currentZoom, m_currentZoom);
    m_view->centerOn(smoothCenter);
}

void CameraController::reset()
{
    m_view->resetTransform();
    m_currentZoom = 1.5f;
}

void CameraController::initForStart()
{
    ViewTarget target = computeViewTarget();

    m_currentZoom = target.zoom;

    m_view->resetTransform();
    m_view->scale(m_currentZoom, m_currentZoom);
    m_view->centerOn(target.center);
}

void CameraController::setScene(GameScene* scene)
{
    m_scene = scene;
}