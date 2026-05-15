// 危险物(Hazard)实体实现 — 散布在地图上的减益实体
// 球体触碰后(无护盾)受到对应负面效果，每约 15 秒概率生成，最多 8 个，存在 45 秒后消失
#include "Hazard.h"
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QtMath>

Hazard::Hazard()
    : Entity(GameConstants::HAZARD_RADIUS, Qt::red, nullptr)
{
    // 随机选择三种危险物类型
    int pick = QRandomGenerator::global()->bounded(3);
    switch (pick) {
    case 0:
        hazardType = DebuffType::Bomb;
        m_color = Qt::red;          // 炸弹: 红色
        break;
    case 1:
        hazardType = DebuffType::Trap;
        m_color = QColor(139, 69, 19);  // 陷阱: 棕色
        break;
    case 2:
        hazardType = DebuffType::Poison;
        m_color = Qt::darkGreen;    // 毒雾: 深绿
        break;
    default:
        hazardType = DebuffType::Bomb;
        m_color = Qt::red;
        break;
    }
    setBrush(m_color);
}

// 每帧递减生命周期（初始 45 秒），到期标记死亡
void Hazard::update(qreal dt)
{
    lifetime -= dt;
    if (lifetime <= 0.0f) {
        m_alive = false;
    }
}

// 被球体触碰时标记死亡，减益效果由 GameScene::checkCollisions() 施加
void Hazard::onEaten(Entity* eater)
{
    Q_UNUSED(eater);
    m_alive = false;
}

// 按危险物类型绘制不同图标
void Hazard::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF r = boundingRect();
    painter->setBrush(m_color);
    painter->setPen(Qt::NoPen);

    switch (hazardType) {
    case DebuffType::Bomb: {
        // 炸弹: 红色圆 + 黑色 X 标记
        painter->drawEllipse(r);
        painter->setPen(QPen(Qt::black, 4));
        qreal s = m_radius * 0.5f;
        painter->drawLine(QPointF(-s, -s), QPointF(s, s));
        painter->drawLine(QPointF(s, -s), QPointF(-s, s));
        break;
    }
    case DebuffType::Trap: {
        // 陷阱: 六角星形尖刺（内外半径交替的多边形）
        QPainterPath path;
        int spikes = 6;
        qreal outerR = m_radius * 0.9f;
        qreal innerR = m_radius * 0.4f;
        for (int i = 0; i < spikes * 2; ++i) {
            qreal angle = (i * M_PI / spikes) - M_PI / 2.0;
            qreal r = (i % 2 == 0) ? outerR : innerR;
            QPointF pt(r * qCos(angle), r * qSin(angle));
            if (i == 0)
                path.moveTo(pt);
            else
                path.lineTo(pt);
        }
        path.closeSubpath();
        painter->drawPath(path);
        break;
    }
    case DebuffType::Poison: {
        // 毒雾: 骷髅头状贝塞尔曲线轮廓 + 眼睛和鼻子
        QPainterPath path;
        qreal r = m_radius * 0.85f;
        // 使用 cubicTo 绘制骷髅头轮廓
        path.moveTo(0, -r);
        path.cubicTo(r * 1.2f, -r, r, -r * 0.3f, r * 0.6f, 0);
        path.cubicTo(r, r * 0.5f, r * 0.3f, r * 0.2f, r * 0.3f, r * 0.6f);
        path.cubicTo(r * 0.3f, r, 0, r * 0.5f, 0, r);
        path.cubicTo(0, r * 0.5f, -r * 0.3f, r, -r * 0.3f, r * 0.6f);
        path.cubicTo(-r * 0.3f, r * 0.2f, -r, r * 0.5f, -r * 0.6f, 0);
        path.cubicTo(-r, -r * 0.3f, -r * 1.2f, -r, 0, -r);
        painter->drawPath(path);
        // 眼睛和鼻子
        painter->setPen(QPen(Qt::black, 2));
        painter->setBrush(Qt::white);
        painter->drawEllipse(QPointF(-r * 0.35f, -r * 0.25f), r * 0.15f, r * 0.15f);
        painter->drawEllipse(QPointF(r * 0.35f, -r * 0.25f), r * 0.15f, r * 0.15f);
        painter->drawEllipse(QPointF(0, r * 0.25f), r * 0.1f, r * 0.15f);
        break;
    }
    default:
        painter->drawEllipse(r);
        break;
    }
}

QRectF Hazard::boundingRect() const
{
    return QRectF(-m_radius, -m_radius, m_radius * 2, m_radius * 2);
}
