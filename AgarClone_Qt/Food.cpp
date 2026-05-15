// 豆子(Food)实体实现 — 地图上散布的可吞食实体
// 半径随机 3~8，颜色随机，被吞食后标记死亡并重生
#include "Food.h"
#include <QPainter>
#include <QRandomGenerator>

Food::Food(qreal radius)
    : Entity(radius < 0 ? QRandomGenerator::global()->bounded(3, 9) : radius,
             // RGB 各通道独立随机 0~255
             QColor(QRandomGenerator::global()->bounded(256),
                    QRandomGenerator::global()->bounded(256),
                    QRandomGenerator::global()->bounded(256)))
{
}

// 豆子为静态实体，无需更新
void Food::update(qreal dt)
{
    Q_UNUSED(dt);
}

// 被球体吞食时标记死亡，由 GameScene::removeDeadEntities() 在帧末统一移除
void Food::onEaten(Entity* eater)
{
    Q_UNUSED(eater);
    m_alive = false;
}

// 绘制实心圆
void Food::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setBrush(m_color);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QPointF(0, 0), m_radius, m_radius);
}

// 返回以中心为原点的边界矩形
QRectF Food::boundingRect() const
{
    return QRectF(-m_radius, -m_radius, m_radius * 2, m_radius * 2);
}
