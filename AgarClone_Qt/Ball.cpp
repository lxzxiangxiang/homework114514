#include "Ball.h"
#include "EjectBall.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QRandomGenerator>
#include <QtMath>

Ball::Ball(qreal mass, QColor color, bool isPlayer, int aiLevel)
    : Entity(mass, color)
    , isPlayer(isPlayer)
    , aiLevel(aiLevel)
    , aiId(0)
{
}

qreal Ball::speed() const
{
    qreal r = radius();
    qreal safeR = qMax(r, GameConstants::World::MIN_RADIUS);
    qreal base = GameConstants::Ball::BASE_SPEED * std::sqrt(GameConstants::World::MIN_RADIUS / safeR);
    if (effect == EffectType::Speed && effectTimer > 0)
        base *= GameConstants::Ball::SPEED_MULTIPLIER;
    if (effect == EffectType::Trap && effectTimer > 0)
        base *= GameConstants::Ball::TRAP_SPEED_MULTIPLIER;
    return base;
}

static QPointF splitDir(QPointF input, qreal lastDx, qreal lastDy)
{
    QPointF dir = input;
    if (std::abs(dir.x()) < 1e-6 && std::abs(dir.y()) < 1e-6) dir = QPointF(lastDx, lastDy);
    if (std::abs(dir.x()) < 1e-6 && std::abs(dir.y()) < 1e-6) dir = QPointF(1, 0);
    qreal len = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
    qreal angle = std::atan2(dir.y(), dir.x())
        + (QRandomGenerator::global()->generateDouble() * 2.0 - 1.0) * GameConstants::Ball::SPLIT_RANDOM_ANGLE;
    return QPointF(std::cos(angle), std::sin(angle));
}

void Ball::move(qreal dx, qreal dy, qreal dt)
{
    if (dx != 0 || dy != 0) {
        qreal len = std::sqrt(dx * dx + dy * dy);
        if (len > 0) { dx /= len; dy /= len; }
        lastDx = dx; lastDy = dy;
    }

    qreal s = speed();
    qreal nx = pos().x() + dx * s * dt;
    qreal ny = pos().y() + dy * s * dt;
    qreal r = radius();
    nx = qBound(r, nx, static_cast<qreal>(GameConstants::World::MAP_WIDTH) - r);
    ny = qBound(r, ny, static_cast<qreal>(GameConstants::World::MAP_HEIGHT) - r);
    setPos(nx, ny);
}

Ball* Ball::split(QPointF direction)
{
    if (radius() < GameConstants::Ball::SPLIT_THRESHOLD) return nullptr;

    qreal newMass = m_mass * GameConstants::Ball::SPLIT_MASS_RETAIN;
    m_mass = newMass;
    qreal r = radius();
    qreal oldR = std::sqrt((m_mass + newMass) / M_PI);
    setMass(m_mass);

    auto* nb = new Ball(newMass, m_color, isPlayer, aiLevel);
    nb->aiId = aiId;
    nb->effect = effect;
    nb->effectTimer = effectTimer;
    nb->growOriginalMass = growOriginalMass;

    QPointF dir = splitDir(direction, lastDx, lastDy);
    qreal jitter = 1.0 + (QRandomGenerator::global()->generateDouble() - 0.5) * 0.4;
    qreal offset = (nb->radius() * 2.0 + 30.0) * jitter;
    nb->setPos(pos() + dir * offset);
    return nb;
}

EjectBall* Ball::eject()
{
    if (radius() < GameConstants::Ball::EJECT_THRESHOLD) return nullptr;

    qreal ejectR = GameConstants::EntityRadius::EJECTBALL;
    m_mass -= M_PI * ejectR * ejectR;
    if (m_mass < M_PI * GameConstants::World::MIN_RADIUS * GameConstants::World::MIN_RADIUS)
        m_mass = M_PI * GameConstants::World::MIN_RADIUS * GameConstants::World::MIN_RADIUS;

    QPointF dir = splitDir(QPointF(lastDx, lastDy), lastDx, lastDy);
    qreal spJitter = 1.0 + (QRandomGenerator::global()->generateDouble() - 0.5) * 0.3;
    return new EjectBall(pos(), m_color, dir.x() * spJitter, dir.y() * spJitter);
}

void Ball::eat(Entity* target)
{
    m_mass += target->mass();
    target->onEaten(this);
}

void Ball::applyEffect(EffectType et)
{
    switch (et) {
    case EffectType::Speed:
        effectTimer = GameConstants::EffectDuration::SPEED;
        break;
    case EffectType::Shield:
        effectTimer = GameConstants::EffectDuration::SHIELD;
        break;
    case EffectType::Grow:
        effectTimer = GameConstants::EffectDuration::GROW;
        growOriginalMass = m_mass;
        m_mass *= GameConstants::Ball::Grow::RADIUS_MULTIPLIER * GameConstants::Ball::Grow::RADIUS_MULTIPLIER;
        break;
    case EffectType::Invisible:
        effectTimer = GameConstants::EffectDuration::INVISIBLE;
        break;
    case EffectType::Magnet:
        effectTimer = GameConstants::EffectDuration::MAGNET;
        break;
    case EffectType::Bomb:
        m_mass *= GameConstants::Ball::Bomb::RADIUS_RATIO * GameConstants::Ball::Bomb::RADIUS_RATIO;
        return;
    case EffectType::Trap:
        effectTimer = GameConstants::EffectDuration::TRAP;
        break;
    case EffectType::Poison:
        effectTimer = GameConstants::EffectDuration::POISON;
        break;
    default: return;
    }
    effect = et;
}

bool Ball::hasShield() const
{
    return effect == EffectType::Shield && effectTimer > 0;
}

void Ball::update(qreal dt)
{
    if (effectTimer > 0) {
        effectTimer -= dt;
        if (effectTimer <= 0) {
            if (effect == EffectType::Grow && growOriginalMass > 0) {
                qreal growFactor = GameConstants::Ball::Grow::RADIUS_MULTIPLIER * GameConstants::Ball::Grow::RADIUS_MULTIPLIER;
                qreal expectedMass = growOriginalMass * growFactor;
                if (m_mass > expectedMass) m_mass /= growFactor;
                else m_mass = growOriginalMass;
                growOriginalMass = 0;
            }
            effect = EffectType::None;
        } else if (effect == EffectType::Poison) {
            qreal r = radius();
            qreal newR = qMax(r - GameConstants::Ball::Poison::RADIUS_PER_SEC * dt, 1.0);
            m_mass = M_PI * newR * newR;
        }
    }
}

void Ball::onEaten(Entity* eater)
{
    Q_UNUSED(eater);
    setAlive(false);
}

void Ball::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option); Q_UNUSED(widget);
    if (!isAlive()) return;
    painter->setRenderHint(QPainter::Antialiasing);

    qreal r = radius();
    QColor drawColor = m_color;

    // Invincible flash
    if (effect == EffectType::Shield && effectTimer > 0) {
        qreal flash = std::abs(std::sin(effectTimer * 10.0));
        drawColor = QColor(
            qMin(255, static_cast<int>(m_color.red() + (255 - m_color.red()) * flash)),
            qMin(255, static_cast<int>(m_color.green() + (255 - m_color.green()) * flash)),
            qMin(255, static_cast<int>(m_color.blue() + (255 - m_color.blue()) * flash)));
        // Shield glow
        qreal glowR = r * 1.3;
        painter->setBrush(QColor(255, 255, 255, 100 + 50 * qSin(effectTimer * 5.0)));
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(QPointF(0, 0), glowR, glowR);
    }

    painter->setBrush(drawColor);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QPointF(0, 0), r, r);

    // Invisible
    if (effect == EffectType::Invisible && effectTimer > 0) {
        painter->setOpacity(0.3);
    }

    // Eye
    painter->setBrush(Qt::white);
    painter->drawEllipse(QPointF(0, 0), r * 0.3, r * 0.3);

    // Player outline + cross
    if (isPlayer) {
        painter->setPen(QPen(QColor(100, 255, 100), 3));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(QPointF(0, 0), r * 1.15, r * 1.15);
        painter->setPen(QPen(Qt::white, 2));
        qreal cs = r * 0.4;
        painter->drawLine(QPointF(-cs, 0), QPointF(cs, 0));
        painter->drawLine(QPointF(0, -cs), QPointF(0, cs));
    } else {
        painter->setPen(QPen(Qt::red, 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(QPointF(0, 0), r * 1.1, r * 1.1);
        if (aiLevel >= 2) {
            qreal ts = r * 0.3;
            QPointF tri[] = { QPointF(0, -ts), QPointF(-ts*0.8, ts*0.5), QPointF(ts*0.8, ts*0.5) };
            painter->setBrush(Qt::red);
            painter->setPen(Qt::NoPen);
            painter->drawPolygon(tri, 3);
        }
    }
}

QRectF Ball::boundingRect() const
{
    qreal r = radius() * 1.5;
    return QRectF(-r, -r, r * 2, r * 2);
}
