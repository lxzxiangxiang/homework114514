#include "UIManager.h"
#include "GameScene.h"
#include "Constants.h"

UIManager::UIManager(GameScene* scene, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
{
}

UIManager::~UIManager()
{
}

void UIManager::createMenuItems()
{
}

void UIManager::showMenu()
{
    if (m_gameOverText)     m_gameOverText->setVisible(false);
    if (m_gameOverOverlay)  m_gameOverOverlay->setVisible(false);
    if (m_victoryText)      m_victoryText->setVisible(false);
    if (m_victoryOverlay)   m_victoryOverlay->setVisible(false);
}

void UIManager::showPause(const QRectF& sceneRect)
{
    Q_UNUSED(sceneRect);
}

void UIManager::hideAllUI()
{
    if (m_gameOverText)     m_gameOverText->setVisible(false);
    if (m_gameOverOverlay)  m_gameOverOverlay->setVisible(false);
    if (m_victoryText)      m_victoryText->setVisible(false);
    if (m_victoryOverlay)   m_victoryOverlay->setVisible(false);
}

void UIManager::showResultOverlay(QGraphicsRectItem*& overlay, QGraphicsTextItem*& titleItem,
                                  const QString& titleText, const QColor& titleColor,
                                  int score, int survivalTime)
{
    if (overlay) { m_scene->removeItem(overlay); delete overlay; overlay = nullptr; titleItem = nullptr; }

    const qreal w = GameConstants::Window::WIDTH;
    const qreal h = GameConstants::Window::HEIGHT;

    overlay = new QGraphicsRectItem(0, 0, w, h);
    overlay->setBrush(QBrush(QColor(0, 0, 0, 180)));
    overlay->setPen(Qt::NoPen);
    overlay->setZValue(300);
    m_scene->addItem(overlay);

    QFont titleFont("Arial", 42, QFont::Bold);
    titleItem = new QGraphicsTextItem(titleText, overlay);
    titleItem->setFont(titleFont);
    titleItem->setDefaultTextColor(titleColor);
    QRectF tr = titleItem->boundingRect();
    titleItem->setPos((w - tr.width()) / 2.0, h * 0.2);

    QFont infoFont("Arial", 18);
    int minutes = survivalTime / 60;
    int seconds = survivalTime % 60;
    QString infoText = QStringLiteral("最终分数: %1\n生存时间: %2:%3")
        .arg(score).arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
    auto* info = new QGraphicsTextItem(infoText, overlay);
    info->setFont(infoFont);
    info->setDefaultTextColor(Qt::white);
    QRectF ir = info->boundingRect();
    info->setPos((w - ir.width()) / 2.0, h * 0.4);

    QFont hintFont("Arial", 14);
    auto* hint = new QGraphicsTextItem(QStringLiteral("Enter 重新开始 / M 回菜单"), overlay);
    hint->setFont(hintFont);
    hint->setDefaultTextColor(QColor(200, 200, 200));
    QRectF hr = hint->boundingRect();
    hint->setPos((w - hr.width()) / 2.0, h * 0.6);
}

void UIManager::showGameOver(int score, int survivalTime)
{
    showResultOverlay(m_gameOverOverlay, m_gameOverText,
                      QStringLiteral("游戏结束"), QColor(255, 80, 80), score, survivalTime);
}

void UIManager::showVictory(int score, int survivalTime)
{
    showResultOverlay(m_victoryOverlay, m_victoryText,
                      QStringLiteral("胜利!"), QColor(80, 255, 80), score, survivalTime);
}

void UIManager::updateHUD(qreal score, qreal survivalTime, qreal totalMass,
                          int aiCount, const QString& effects, bool canSplit)
{
    int minutes = static_cast<int>(survivalTime) / 60;
    int seconds = static_cast<int>(survivalTime) % 60;

    m_scene->hudScoreText   = QStringLiteral("分数: %1").arg(score, 0, 'f', 1);
    m_scene->hudTimeText    = QStringLiteral("时间: %1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
    m_scene->hudRadiusText  = QStringLiteral("总质量: %1").arg(totalMass, 0, 'f', 1);
    m_scene->hudAICountText = QStringLiteral("AI: %1").arg(aiCount);
    m_scene->hudEffectsText = QStringLiteral("效果: ") + effects;
    m_scene->hudSplitText   = canSplit ? QStringLiteral("分裂: 可") : QStringLiteral("分裂: 不可");
}

void UIManager::clearOverlayRefs()
{
    m_gameOverOverlay = nullptr;
    m_gameOverText    = nullptr;
    m_victoryOverlay  = nullptr;
    m_victoryText     = nullptr;
}

void UIManager::setScene(GameScene* scene)
{
    m_scene = scene;
}