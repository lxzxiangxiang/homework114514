// UI 管理器(UIManager)实现 — 管理所有界面元素：主菜单、暂停、结束、胜利、HUD
// HUD 显示 9 项实时数据(分数/时间/质量/半径/AI/分裂/技能/减益/无敌)
#include "UIManager.h"
#include "Constants.h"

#include <QFont>
#include <QColor>
#include <QBrush>
#include <QPen>

const int hudz=110;

UIManager::UIManager(QGraphicsScene* scene, QGraphicsView* view, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_view(view)
{
    createHUDItems();
    createMenuItems();
}

// ===== 创建 HUD 文本项 =====
// 创建 9 个 QGraphicsTextItem（白色 Arial 12, z=hudz），默认隐藏
// ItemIgnoresTransformations 确保 HUD 不受摄像机变换影响
void UIManager::createHUDItems()
{
    QFont hudFont("Arial", 12);
    QColor hudColor(Qt::white);

    m_hudScore = new QGraphicsTextItem();
    m_hudScore->setFont(hudFont);
    m_hudScore->setDefaultTextColor(hudColor);
    m_hudScore->setZValue(hudz);
    m_hudScore->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    m_hudScore->setVisible(false);
    m_scene->addItem(m_hudScore);

    m_hudTime = new QGraphicsTextItem();
    m_hudTime->setFont(hudFont);
    m_hudTime->setDefaultTextColor(hudColor);
    m_hudTime->setZValue(hudz);
    m_hudTime->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    m_hudTime->setVisible(false);
    m_scene->addItem(m_hudTime);

    m_hudMass = new QGraphicsTextItem();
    m_hudMass->setFont(hudFont);
    m_hudMass->setDefaultTextColor(hudColor);
    m_hudMass->setZValue(hudz);
    m_hudMass->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    m_hudMass->setVisible(false);
    m_scene->addItem(m_hudMass);

    m_hudRadius = new QGraphicsTextItem();
    m_hudRadius->setFont(hudFont);
    m_hudRadius->setDefaultTextColor(hudColor);
    m_hudRadius->setZValue(hudz);
    m_hudRadius->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    m_hudRadius->setVisible(false);
    m_scene->addItem(m_hudRadius);

    m_hudAICount = new QGraphicsTextItem();
    m_hudAICount->setFont(hudFont);
    m_hudAICount->setDefaultTextColor(hudColor);
    m_hudAICount->setZValue(hudz);
    m_hudAICount->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    m_hudAICount->setVisible(false);
    m_scene->addItem(m_hudAICount);

    m_hudSplitStatus = new QGraphicsTextItem();
    m_hudSplitStatus->setFont(hudFont);
    m_hudSplitStatus->setDefaultTextColor(hudColor);
    m_hudSplitStatus->setZValue(hudz);
    m_hudSplitStatus->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    m_hudSplitStatus->setVisible(false);
    m_scene->addItem(m_hudSplitStatus);

    m_hudSkill = new QGraphicsTextItem();
    m_hudSkill->setFont(hudFont);
    m_hudSkill->setDefaultTextColor(hudColor);
    m_hudSkill->setZValue(hudz);
    m_hudSkill->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    m_hudSkill->setVisible(false);
    m_scene->addItem(m_hudSkill);

    m_hudDebuff = new QGraphicsTextItem();
    m_hudDebuff->setFont(hudFont);
    m_hudDebuff->setDefaultTextColor(hudColor);
    m_hudDebuff->setZValue(hudz);
    m_hudDebuff->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    m_hudDebuff->setVisible(false);
    m_scene->addItem(m_hudDebuff);

    m_hudInvincible = new QGraphicsTextItem();
    m_hudInvincible->setFont(hudFont);
    m_hudInvincible->setDefaultTextColor(hudColor);
    m_hudInvincible->setZValue(hudz);
    m_hudInvincible->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    m_hudInvincible->setVisible(false);
    m_scene->addItem(m_hudInvincible);
}

// ===== 创建主菜单元素 =====
// 半透明黑背景 + 标题 "Agar.io Clone" + 操作说明
void UIManager::createMenuItems()
{
    const qreal w = GameConstants::WINDOW_WIDTH;
    const qreal h = GameConstants::WINDOW_HEIGHT;

    // 半透明黑色背景
    m_menuBackground = new QGraphicsRectItem(0, 0, w, h);
    m_menuBackground->setBrush(QBrush(QColor(0, 0, 0, 180)));
    m_menuBackground->setPen(Qt::NoPen);
    m_menuBackground->setZValue(200);                   // 高于 HUD 的 z 值
    m_menuBackground->setVisible(false);
    m_scene->addItem(m_menuBackground);

    // 标题
    QFont titleFont("Arial", 48, QFont::Bold);
    m_menuTitle = new QGraphicsTextItem(QString::fromUtf8("Agar.io Clone"));
    m_menuTitle->setFont(titleFont);
    m_menuTitle->setDefaultTextColor(Qt::white);
    m_menuTitle->setZValue(201);
    m_menuTitle->setVisible(false);
    m_scene->addItem(m_menuTitle);

    QRectF titleRect = m_menuTitle->boundingRect();
    m_menuTitle->setPos((w - titleRect.width()) / 2.0, h * 0.2);

    // 操作说明
    QFont hintFont("Arial", 14);
    QString hintText = QString::fromUtf8(
        "WASD / Mouse - Move\n"
        "Space - Split\n"
        "E - Eject Mass\n"
        "ESC - Pause\n\n"
        "Press Enter to Start");
    m_menuHint = new QGraphicsTextItem(hintText);
    m_menuHint->setFont(hintFont);
    m_menuHint->setDefaultTextColor(QColor(200, 200, 200));
    m_menuHint->setZValue(201);
    m_menuHint->setVisible(false);
    m_scene->addItem(m_menuHint);

    QRectF hintRect = m_menuHint->boundingRect();
    m_menuHint->setPos((w - hintRect.width()) / 2.0, h * 0.45);
}

// 显示主菜单：显示菜单元素，隐藏其他界面
void UIManager::showMenu()
{
    m_menuBackground->setVisible(true);
    m_menuTitle->setVisible(true);
    m_menuHint->setVisible(true);

    // 隐藏暂停/结束/胜利界面
    if (m_pauseOverlay) {
        m_pauseOverlay->setVisible(false);
    }
    if (m_gameOverText) {
        m_gameOverText->setVisible(false);
    }
    if (m_gameOverOverlay) {
        m_gameOverOverlay->setVisible(false);
    }
    if (m_victoryText) {
        m_victoryText->setVisible(false);
    }
    if (m_victoryOverlay) {
        m_victoryOverlay->setVisible(false);
    }

    // 隐藏 HUD
    m_hudScore->setVisible(false);
    m_hudTime->setVisible(false);
    m_hudMass->setVisible(false);
    m_hudRadius->setVisible(false);
    m_hudAICount->setVisible(false);
    m_hudSplitStatus->setVisible(false);
    m_hudSkill->setVisible(false);
    m_hudDebuff->setVisible(false);
    m_hudInvincible->setVisible(false);
}

// 显示暂停界面：惰性创建半透明遮罩 + "PAUSED" + 提示
void UIManager::showPause()
{
    if (!m_pauseOverlay) {
        const qreal w = GameConstants::WINDOW_WIDTH;
        const qreal h = GameConstants::WINDOW_HEIGHT;

        // 半透明黑色遮罩
        m_pauseOverlay = new QGraphicsRectItem(0, 0, w, h);
        m_pauseOverlay->setBrush(QBrush(QColor(0, 0, 0, 150)));
        m_pauseOverlay->setPen(Qt::NoPen);
        m_pauseOverlay->setZValue(250);
        m_scene->addItem(m_pauseOverlay);

        // "PAUSED" 标签
        QFont pauseFont("Arial", 36, QFont::Bold);
        QGraphicsTextItem* pauseLabel = new QGraphicsTextItem("PAUSED", m_pauseOverlay);
        pauseLabel->setFont(pauseFont);
        pauseLabel->setDefaultTextColor(Qt::white);
        QRectF rect = pauseLabel->boundingRect();
        pauseLabel->setPos((w - rect.width()) / 2.0, h * 0.35);

        // 操作提示
        QFont smallFont("Arial", 14);
        QGraphicsTextItem* pauseHint = new QGraphicsTextItem(
            QString::fromUtf8("Press ESC to Continue\nPress M for Menu"),
            m_pauseOverlay);
        pauseHint->setFont(smallFont);
        pauseHint->setDefaultTextColor(QColor(200, 200, 200));
        QRectF hintRect = pauseHint->boundingRect();
        pauseHint->setPos((w - hintRect.width()) / 2.0, h * 0.5);
    }

    m_pauseOverlay->setVisible(true);
}

// 显示游戏结束界面：红色 "GAME OVER" + 分数/时间 + 操作提示
void UIManager::showGameOver(int score, int survivalTime)
{
    // 删除已有 overlay（避免重复创建）
    if (m_gameOverOverlay) {
        m_scene->removeItem(m_gameOverOverlay);
        delete m_gameOverOverlay;
        m_gameOverOverlay = nullptr;
        m_gameOverText = nullptr;
    }

    const qreal w = GameConstants::WINDOW_WIDTH;
    const qreal h = GameConstants::WINDOW_HEIGHT;

    m_gameOverOverlay = new QGraphicsRectItem(0, 0, w, h);
    m_gameOverOverlay->setBrush(QBrush(QColor(0, 0, 0, 180)));
    m_gameOverOverlay->setPen(Qt::NoPen);
    m_gameOverOverlay->setZValue(300);
    m_scene->addItem(m_gameOverOverlay);

    QFont titleFont("Arial", 42, QFont::Bold);
    m_gameOverText = new QGraphicsTextItem("GAME OVER", m_gameOverOverlay);
    m_gameOverText->setFont(titleFont);
    m_gameOverText->setDefaultTextColor(QColor(255, 80, 80));
    QRectF titleRect = m_gameOverText->boundingRect();
    m_gameOverText->setPos((w - titleRect.width()) / 2.0, h * 0.2);

    QFont infoFont("Arial", 18);
    int minutes = survivalTime / 60;
    int seconds = survivalTime % 60;
    QString infoText = QString("Final Score: %1\nSurvival Time: %2:%3")
        .arg(score)
        .arg(minutes)
        .arg(seconds, 2, 10, QLatin1Char('0'));
    QGraphicsTextItem* info = new QGraphicsTextItem(infoText, m_gameOverOverlay);
    info->setFont(infoFont);
    info->setDefaultTextColor(Qt::white);
    QRectF infoRect = info->boundingRect();
    info->setPos((w - infoRect.width()) / 2.0, h * 0.4);

    QFont hintFont("Arial", 14);
    QGraphicsTextItem* hint = new QGraphicsTextItem(
        QString::fromUtf8("Press Enter to Restart\nPress M for Menu"),
        m_gameOverOverlay);
    hint->setFont(hintFont);
    hint->setDefaultTextColor(QColor(200, 200, 200));
    QRectF hintRect = hint->boundingRect();
    hint->setPos((w - hintRect.width()) / 2.0, h * 0.6);
}

// 显示胜利界面：绿色 "VICTORY!" + 分数/时间 + 操作提示
void UIManager::showVictory(int score, int survivalTime)
{
    if (m_victoryOverlay) {
        m_scene->removeItem(m_victoryOverlay);
        delete m_victoryOverlay;
        m_victoryOverlay = nullptr;
        m_victoryText = nullptr;
    }

    const qreal w = GameConstants::WINDOW_WIDTH;
    const qreal h = GameConstants::WINDOW_HEIGHT;

    m_victoryOverlay = new QGraphicsRectItem(0, 0, w, h);
    m_victoryOverlay->setBrush(QBrush(QColor(0, 0, 0, 180)));
    m_victoryOverlay->setPen(Qt::NoPen);
    m_victoryOverlay->setZValue(300);
    m_scene->addItem(m_victoryOverlay);

    QFont titleFont("Arial", 42, QFont::Bold);
    m_victoryText = new QGraphicsTextItem("VICTORY!", m_victoryOverlay);
    m_victoryText->setFont(titleFont);
    m_victoryText->setDefaultTextColor(QColor(80, 255, 80));
    QRectF titleRect = m_victoryText->boundingRect();
    m_victoryText->setPos((w - titleRect.width()) / 2.0, h * 0.2);

    QFont infoFont("Arial", 18);
    int minutes = survivalTime / 60;
    int seconds = survivalTime % 60;
    QString infoText = QString("Final Score: %1\nSurvival Time: %2:%3")
        .arg(score)
        .arg(minutes)
        .arg(seconds, 2, 10, QLatin1Char('0'));
    QGraphicsTextItem* info = new QGraphicsTextItem(infoText, m_victoryOverlay);
    info->setFont(infoFont);
    info->setDefaultTextColor(Qt::white);
    QRectF infoRect = info->boundingRect();
    info->setPos((w - infoRect.width()) / 2.0, h * 0.4);

    QFont hintFont("Arial", 14);
    QGraphicsTextItem* hint = new QGraphicsTextItem(
        QString::fromUtf8("Press Enter to Restart\nPress M for Menu"),
        m_victoryOverlay);
    hint->setFont(hintFont);
    hint->setDefaultTextColor(QColor(200, 200, 200));
    QRectF hintRect = hint->boundingRect();
    hint->setPos((w - hintRect.width()) / 2.0, h * 0.6);
}

// 隐藏所有 UI 元素
void UIManager::hideAll()
{
    m_menuBackground->setVisible(false);
    m_menuTitle->setVisible(false);
    m_menuHint->setVisible(false);

    if (m_pauseOverlay) {
        m_pauseOverlay->setVisible(false);
    }
    if (m_gameOverText) {
        m_gameOverText->setVisible(false);
    }
    if (m_gameOverOverlay) {
        m_gameOverOverlay->setVisible(false);
    }
    if (m_victoryText) {
        m_victoryText->setVisible(false);
    }
    if (m_victoryOverlay) {
        m_victoryOverlay->setVisible(false);
    }

    m_hudScore->setVisible(false);
    m_hudTime->setVisible(false);
    m_hudMass->setVisible(false);
    m_hudRadius->setVisible(false);
    m_hudAICount->setVisible(false);
    m_hudSplitStatus->setVisible(false);
    m_hudSkill->setVisible(false);
    m_hudDebuff->setVisible(false);
    m_hudInvincible->setVisible(false);
}

// 更新 HUD 显示：设置文本并按 18px 行高垂直排列
void UIManager::updateHUD(qreal score, qreal survivalTime, qreal totalMass, qreal avgRadius,
                          int aiCount, bool canSplit, const QString& skillInfo,
                          const QString& debuffInfo, const QString& invincibleInfo)
{
    int minutes = static_cast<int>(survivalTime) / 60;
    int seconds = static_cast<int>(survivalTime) % 60;

    // 设置各 HUD 项的文本
    m_hudScore->setPlainText(QString("Score: %1").arg(score, 0, 'f', 1));
    m_hudTime->setPlainText(QString("Time: %1:%2")
        .arg(minutes)
        .arg(seconds, 2, 10, QLatin1Char('0')));
    m_hudMass->setPlainText(QString("Mass: %1").arg(totalMass, 0, 'f', 1));
    m_hudRadius->setPlainText(QString("Radius: %1").arg(avgRadius, 0, 'f', 1));
    m_hudAICount->setPlainText(QString("AI Count: %1").arg(aiCount));
    m_hudSplitStatus->setPlainText(canSplit
        ? "Split: Ready"
        : "Split: Cooldown");
    m_hudSkill->setPlainText("Skill: " + skillInfo);
    m_hudDebuff->setPlainText("Debuff: " + debuffInfo);
    m_hudInvincible->setPlainText("Invincible: " + invincibleInfo);

    // 垂直排列：动态获取视口左上角场景坐标作为 HUD 原点
    const qreal lineHeight = 18.0;
    const qreal margin = 10.0;

    QPointF hudOrigin = m_view->mapToScene(0, 0);

    m_hudScore->setPos(hudOrigin.x() + margin, hudOrigin.y() + margin);
    m_hudScore->setVisible(true);

    m_hudTime->setPos(hudOrigin.x() + margin, hudOrigin.y() + margin + lineHeight * 1);
    m_hudTime->setVisible(true);

    m_hudMass->setPos(hudOrigin.x() + margin, hudOrigin.y() + margin + lineHeight * 2);
    m_hudMass->setVisible(true);

    m_hudRadius->setPos(hudOrigin.x() + margin, hudOrigin.y() + margin + lineHeight * 3);
    m_hudRadius->setVisible(true);

    m_hudAICount->setPos(hudOrigin.x() + margin, hudOrigin.y() + margin + lineHeight * 4);
    m_hudAICount->setVisible(true);

    m_hudSplitStatus->setPos(hudOrigin.x() + margin, hudOrigin.y() + margin + lineHeight * 5);
    m_hudSplitStatus->setVisible(true);

    m_hudSkill->setPos(hudOrigin.x() + margin, hudOrigin.y() + margin + lineHeight * 6);
    m_hudSkill->setVisible(true);

    m_hudDebuff->setPos(hudOrigin.x() + margin, hudOrigin.y() + margin + lineHeight * 7);
    m_hudDebuff->setVisible(true);

    m_hudInvincible->setPos(hudOrigin.x() + margin, hudOrigin.y() + margin + lineHeight * 8);
    m_hudInvincible->setVisible(true);
}
