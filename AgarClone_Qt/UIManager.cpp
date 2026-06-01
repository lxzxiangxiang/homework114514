#include "UIManager.h"
#include "GameScene.h"

UIManager::UIManager(GameScene* scene, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
{
}

void UIManager::updateHUD(qreal score, qreal survivalTime, qreal totalMass,
                          int aiCount, const QString& effects, bool canSplit)
{
    int minutes = static_cast<int>(survivalTime) / 60;
    int seconds = static_cast<int>(survivalTime) % 60;

    m_scene->m_hudScoreText   = QStringLiteral("分数: %1").arg(score, 0, 'f', 1);
    m_scene->m_hudTimeText    = QStringLiteral("时间: %1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
    m_scene->m_hudRadiusText  = QStringLiteral("总质量: %1").arg(totalMass, 0, 'f', 1);
    m_scene->m_hudAICountText = QStringLiteral("AI: %1").arg(aiCount);
    m_scene->m_hudEffectsText = QStringLiteral("效果: ") + effects;
    m_scene->m_hudSplitText   = canSplit ? QStringLiteral("分裂: 可") : QStringLiteral("分裂: 不可");
}

void UIManager::setScene(GameScene* scene)
{
    m_scene = scene;
}
