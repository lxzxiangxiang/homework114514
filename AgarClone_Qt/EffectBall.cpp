#include "EffectBall.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>

static QColor colorForEffect(EffectType type)
{
    switch (type) {
    case EffectType::Speed:     return QColor(255, 220, 0);
    case EffectType::Shield:    return QColor(0, 200, 255);
    case EffectType::Grow:      return QColor(180, 0, 255);
    case EffectType::Invisible: return QColor(180, 180, 180);
    case EffectType::Magnet:    return QColor(255, 150, 0);
    case EffectType::Bomb:      return QColor(255, 80, 80);
    case EffectType::Trap:      return QColor(139, 90, 43);
    case EffectType::Poison:    return QColor(0, 200, 0);
    default:                    return Qt::white;
    }
}

EffectBall::EffectBall(QPointF pos, EffectType type)
    : Entity(M_PI * GameConstants::EntityRadius::EFFECTBALL * GameConstants::EntityRadius::EFFECTBALL, Qt::white)
    , m_type(type)
    , m_maxLifetime(isBuff(type) ? 30.0 : 45.0)
{
    setPos(pos);
    m_color = colorForEffect(type);
    setBrush(m_color);

    if (type == EffectType::Trap) {
        m_moveSpeed = 0.5;
        m_moveAngle = QRandomGenerator::global()->bounded(2.0 * M_PI);
    } else if (type == EffectType::Poison) {
        m_moveSpeed = 0.3;
        m_moveAngle = QRandomGenerator::global()->bounded(2.0 * M_PI);
    }
}

void EffectBall::update(qreal dt)
{
    m_pulseTimer += dt * 3;
    m_rotation += dt * 2;
    m_flashTimer += dt * 5;
    m_lifetime += dt;

    if (m_lifetime >= m_maxLifetime) {
        setAlive(false);
        return;
    }

    if (m_moveSpeed > 0) {
        qreal r = radius();
        qreal nx = pos().x() + std::cos(m_moveAngle) * m_moveSpeed * dt * 60;
        qreal ny = pos().y() + std::sin(m_moveAngle) * m_moveSpeed * dt * 60;
        if (nx <= r) { nx = r; m_moveAngle = M_PI - m_moveAngle; }
        else if (nx >= GameConstants::World::MAP_WIDTH - r) { nx = GameConstants::World::MAP_WIDTH - r; m_moveAngle = M_PI - m_moveAngle; }
        if (ny <= r) { ny = r; m_moveAngle = -m_moveAngle; }
        else if (ny >= GameConstants::World::MAP_HEIGHT - r) { ny = GameConstants::World::MAP_HEIGHT - r; m_moveAngle = -m_moveAngle; }
        setPos(nx, ny);
        if (m_type == EffectType::Poison && QRandomGenerator::global()->generateDouble() < 0.01)
            m_moveAngle += QRandomGenerator::global()->generateDouble() * 1.0 - 0.5;
    }
}

void EffectBall::onEaten(Entity* eater)
{
    Q_UNUSED(eater);
    setAlive(false);
}

void EffectBall::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    if (!isAlive()) return;
    painter->setRenderHint(QPainter::Antialiasing);

    qreal r = radius();
    qreal pulse = 1.0 + 0.2 * std::sin(m_pulseTimer);
    int fadeAlpha = 255;
    if (m_lifetime > m_maxLifetime - 5) {
        fadeAlpha = static_cast<int>(255 * (1.0 - (m_lifetime - (m_maxLifetime - 5)) / 5.0));
        fadeAlpha = std::max(50, fadeAlpha);
    }

    // Glow
    if (isBuff(m_type)) {
        qreal glowR = r * 1.5 * pulse;
        QColor glow(m_color.red(), m_color.green(), m_color.blue(), 80 * fadeAlpha / 255);
        painter->setBrush(glow);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(QPointF(0, 0), glowR, glowR);
    }

    // Body
    QColor drawColor(m_color.red() * fadeAlpha / 255, m_color.green() * fadeAlpha / 255, m_color.blue() * fadeAlpha / 255);
    painter->setBrush(drawColor);

    if (isDebuff(m_type)) {
        painter->setPen(QPen(Qt::black, 2));
        // Hazard warning circle for Bomb
        if (m_type == EffectType::Bomb) {
            qreal wr = r * (1.5 + 0.3 * std::sin(m_flashTimer * 2));
            painter->setPen(QPen(QColor(255, 0, 0, 100), 2));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(QPointF(0, 0), wr, wr);
            painter->setBrush(drawColor);
            painter->setPen(QPen(Qt::black, 2));
        }
        // Trap stripes
        if (m_type == EffectType::Trap) {
            for (int i = 0; i < 8; ++i) {
                double angle = i * 2 * M_PI / 8 + m_flashTimer * 0.5;
                QColor stripe = (i % 2 == 0) ? QColor(255, 255, 0) : QColor(0, 0, 0);
                painter->setPen(QPen(stripe, 2));
                painter->drawLine(QPointF(std::cos(angle) * r * 0.5, std::sin(angle) * r * 0.5),
                                  QPointF(std::cos(angle) * r * 1.2, std::sin(angle) * r * 1.2));
            }
            painter->setPen(QPen(Qt::black, 2));
        }
        // Poison bubbles
        if (m_type == EffectType::Poison) {
            for (int i = 0; i < 3; ++i) {
                double bo = i * 2 * M_PI / 3 + m_flashTimer;
                painter->setBrush(QColor(0, 255, 0));
                painter->setPen(Qt::NoPen);
                painter->drawEllipse(QPointF(std::cos(bo) * r * 0.7, std::sin(bo) * r * 0.7), r * 0.2, r * 0.2);
            }
            painter->setBrush(drawColor);
            painter->setPen(QPen(Qt::black, 2));
        }
    } else {
        painter->setPen(Qt::NoPen);
    }
    painter->drawEllipse(QPointF(0, 0), r * pulse, r * pulse);

    // Inner white dot (skill balls)
    if (isBuff(m_type)) {
        painter->setBrush(Qt::white);
        painter->drawEllipse(QPointF(0, 0), r * pulse * 0.4, r * pulse * 0.4);
    }

    drawIcon(painter, 0, 0, r * pulse * 0.55);
}

void EffectBall::drawIcon(QPainter* painter, qreal x, qreal y, qreal s)
{
    painter->setPen(QPen(Qt::white, 2));
    painter->setBrush(Qt::NoBrush);

    switch (m_type) {
    case EffectType::Speed: {
        QPointF pts[] = { {x + s*0.2, y-s}, {x + s*0.6, y-s*0.1}, {x, y}, {x + s*0.6, y+s*0.1}, {x + s*0.2, y+s} };
        painter->drawPolyline(pts, 5);
        break;
    }
    case EffectType::Shield: {
        painter->drawArc(QRectF(x-s, y-s*0.8, s*2, s*2), 30*16, 120*16);
        painter->drawLine(QPointF(x, y), QPointF(x, y+s*0.4));
        break;
    }
    case EffectType::Grow: {
        painter->drawLine(QPointF(x, y-s), QPointF(x, y+s));
        painter->drawLine(QPointF(x-s*0.5, y-s*0.3), QPointF(x, y-s));
        painter->drawLine(QPointF(x+s*0.5, y-s*0.3), QPointF(x, y-s));
        break;
    }
    case EffectType::Invisible: {
        painter->drawEllipse(QPointF(x, y), s*0.5, s*0.3);
        painter->setBrush(Qt::black);
        painter->drawEllipse(QPointF(x, y), s*0.15, s*0.15);
        break;
    }
    case EffectType::Magnet: {
        painter->drawArc(QRectF(x-s*0.4, y-s*0.7, s*0.8, s*1.4), 200*16, 140*16);
        painter->drawLine(QPointF(x-s*0.4, y-s*0.14), QPointF(x+s*0.4, y-s*0.14));
        painter->drawLine(QPointF(x-s*0.4, y+s*0.14), QPointF(x+s*0.4, y+s*0.14));
        break;
    }
    case EffectType::Bomb: {
        painter->setPen(QPen(QColor(255, 165, 0), 2));
        for (int i = 0; i < 8; ++i) {
            double a = i * M_PI / 4;
            painter->drawLine(QPointF(x + std::cos(a) * s * 0.3, y + std::sin(a) * s * 0.3),
                              QPointF(x + std::cos(a) * s, y + std::sin(a) * s));
        }
        break;
    }
    case EffectType::Trap: {
        painter->setPen(QPen(Qt::white, 3));
        painter->drawLine(QPointF(x-s, y-s), QPointF(x+s, y+s));
        painter->drawLine(QPointF(x+s, y-s), QPointF(x-s, y+s));
        break;
    }
    case EffectType::Poison: {
        painter->setBrush(Qt::white);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(QPointF(x, y-s*0.3), s*0.5, s*0.5);
        painter->drawEllipse(QPointF(x, y+s*0.2), s*0.3, s*0.4);
        break;
    }
    default: break;
    }
}

QRectF EffectBall::boundingRect() const
{
    qreal r = radius();
    qreal s = r * 1.8;
    return QRectF(-s, -s, s * 2, s * 2);
}
