#include "Ball.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QRandomGenerator>
#include <QtMath>

namespace {
qreal easeOutCubic(qreal t)
{
    return 1 - std::pow(1 - t, 3);
}
}

Ball::Ball(qreal mass, QColor color, bool isPlayer, int aiLevel)
    : Entity(mass, color)
    , isPlayer(isPlayer)
    , aiLevel(aiLevel)
    , aiId(0)
{
}

void Ball::removeEffect(EffectType t)
{
    for (int i = 0; i < m_effects.size(); ++i) {
        if (m_effects[i].type == t) {
            m_effects.removeAt(i);
            return;
        }
    }
}

qreal Ball::speed() const
{
    qreal r = radius();
    qreal safeR = std::max(r, static_cast<qreal>(GameConstants::World::MIN_RADIUS));
    qreal base = GameConstants::Ball::BASE_SPEED * std::sqrt(GameConstants::World::MIN_RADIUS / safeR);
    if (hasEffect(EffectType::Speed))
        base *= GameConstants::Ball::SPEED_MULTIPLIER;
    if (hasEffect(EffectType::Trap))
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
        + (QRandomGenerator::global()->generateDouble() * 2.0 - 1.0) * GameConstants::Ball::SPLIT_DIR_JITTER;
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
    nx = std::clamp(nx, r, static_cast<qreal>(GameConstants::World::MAP_WIDTH) - r);
    ny = std::clamp(ny, r, static_cast<qreal>(GameConstants::World::MAP_HEIGHT) - r);
    if (m_splitAnim.active) {
        QPointF movement(dx * s * dt, dy * s * dt);
        m_splitAnim.startPos += movement;
        m_splitAnim.targetPos += movement;
    } else {
        setPos(nx, ny);
    }
}

Ball* Ball::split(QPointF direction)
{
    if (radius() < GameConstants::Ball::SPLIT_THRESHOLD) return nullptr;

    qreal newMass = m_mass * GameConstants::Ball::SPLIT_MASS_RETAIN;
    setMass(newMass);

    auto* nb = new Ball(newMass, m_color, isPlayer, aiLevel);
    nb->aiId = aiId;
    nb->m_effects = m_effects;

    for (auto& ae : m_effects) {
        if (ae.type == EffectType::Grow && ae.growOriginalMass > 0)
            ae.growOriginalMass *= 0.5;
    }
    for (auto& ae : nb->m_effects) {
        if (ae.type == EffectType::Grow && ae.growOriginalMass > 0)
            ae.growOriginalMass *= 0.5;
    }

    QPointF dir = splitDir(direction, lastDx, lastDy);
    qreal jitter = 1.0 + (QRandomGenerator::global()->generateDouble() - 0.5) * 0.6;
    qreal offset = (nb->radius() * 2.0 + 30.0) * jitter;

    QPointF edgePos = pos() + dir * radius();

    nb->setPos(edgePos);
    nb->m_splitAnim.active = true;
    nb->m_splitAnim.startPos = edgePos;
    nb->m_splitAnim.targetPos = pos() + dir * offset;
    nb->m_splitAnim.progress = 0.0;
    nb->m_splitAnim.duration = GameConstants::Physics::SPLIT_ANIM_DURATION;

    m_mergeTimer = 0;
    nb->m_mergeTimer = 0;

    return nb;
}

void Ball::eat(Entity* target)
{
    m_mass += target->mass();
    setMass(m_mass);
    target->onEaten(this);
}

void Ball::applyEffect(EffectType et, const QList<Ball*>& allBalls)
{
    if (hasShield() && isDebuff(et)) return;

    qreal timer = 0;
    switch (et) {
    case EffectType::Speed:     timer = GameConstants::EffectDuration::SPEED; break;
    case EffectType::Shield:    timer = GameConstants::EffectDuration::SHIELD; break;
    case EffectType::Grow:      timer = GameConstants::EffectDuration::GROW; break;
    case EffectType::Invisible: timer = GameConstants::EffectDuration::INVISIBLE; break;
    case EffectType::Magnet:    timer = GameConstants::EffectDuration::MAGNET; break;
    case EffectType::Bomb:      break;
    case EffectType::Trap:      timer = GameConstants::EffectDuration::TRAP; break;
    case EffectType::Poison:    timer = GameConstants::EffectDuration::POISON; break;
    case EffectType::None: return;
    }

    if (et == EffectType::Bomb) {
        m_mass *= GameConstants::Ball::Bomb::RADIUS_RATIO * GameConstants::Ball::Bomb::RADIUS_RATIO;
        qreal minMass = M_PI * GameConstants::World::MIN_RADIUS * GameConstants::World::MIN_RADIUS;
        if (m_mass < minMass) m_mass = minMass;
        return;
    }

    ActiveEffect ae;
    ae.type = et;
    ae.timer = timer;
    ae.growOriginalMass = (et == EffectType::Grow) ? m_mass : 0;

    if (et == EffectType::Grow) {
        m_mass *= GameConstants::Ball::Grow::RADIUS_MULTIPLIER * GameConstants::Ball::Grow::RADIUS_MULTIPLIER;
    }

    m_effects.append(ae);

    for (Ball* other : allBalls) {
        if (other == this || !other->isAlive()) continue;
        bool sameGroup = (aiId > 0 && other->aiId == aiId) || (isPlayer && other->isPlayer);
        if (!sameGroup) continue;

        ActiveEffect oae;
        oae.type = et;
        oae.timer = timer;
        oae.growOriginalMass = (et == EffectType::Grow) ? other->m_mass : 0;

        if (et == EffectType::Grow) {
            other->m_mass *= GameConstants::Ball::Grow::RADIUS_MULTIPLIER * GameConstants::Ball::Grow::RADIUS_MULTIPLIER;
        }
        other->m_effects.append(oae);
    }
}

void Ball::addInitialEffect(EffectType t, qreal timer)
{
    ActiveEffect ae;
    ae.type = t;
    ae.timer = timer;
    if (t == EffectType::Shield) {
        m_effects.append(ae);
    }
}

bool Ball::hasShield() const
{
    return hasEffect(EffectType::Shield);
}

bool Ball::hasEffect(EffectType t) const
{
    for (const auto& ae : m_effects) {
        if (ae.type == t) return true;
    }
    return false;
}

void Ball::update(qreal dt)
{
    m_mergeTimer += dt;

    if (m_splitAnim.active) {
        m_splitAnim.progress += dt / m_splitAnim.duration;
        if (m_splitAnim.progress >= 1.0) {
            m_splitAnim.active = false;
            setPos(m_splitAnim.targetPos);
        } else {
            qreal t = easeOutCubic(m_splitAnim.progress);
            setPos(m_splitAnim.startPos + (m_splitAnim.targetPos - m_splitAnim.startPos) * t);
        }
    }

    for (int i = m_effects.size() - 1; i >= 0; --i) {
        ActiveEffect& ae = m_effects[i];
        ae.timer -= dt;
        if (ae.timer <= 0) {
            if (ae.type == EffectType::Grow && ae.growOriginalMass > 0) {
                qreal growFactor = GameConstants::Ball::Grow::RADIUS_MULTIPLIER * GameConstants::Ball::Grow::RADIUS_MULTIPLIER;
                qreal expectedMass = ae.growOriginalMass * growFactor;
                if (m_mass > expectedMass) m_mass /= growFactor;
                else m_mass = ae.growOriginalMass;
            }
            m_effects.removeAt(i);
        } else if (ae.type == EffectType::Poison) {
            qreal r = radius();
            qreal newR = std::max(r - GameConstants::Ball::Poison::RADIUS_PER_SEC * dt, 1.0);
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

    if (m_splitAnim.active && m_splitAnim.progress < 0.3) {
        qreal scale = 0.5 + 0.5 * (m_splitAnim.progress / 0.3);
        painter->scale(scale, scale);
        qreal r = radius();
        painter->translate(-r * (1.0 - scale), -r * (1.0 - scale));
    }

    qreal r = radius();
    QColor drawColor = m_color;

    if (hasEffect(EffectType::Shield)) {
        qreal shieldTimer = 0;
        for (const auto& ae : m_effects) {
            if (ae.type == EffectType::Shield) { shieldTimer = ae.timer; break; }
        }
        qreal flash = std::abs(std::sin(shieldTimer * 10.0));
        drawColor = QColor(
            std::min(255, static_cast<int>(m_color.red() + (255 - m_color.red()) * flash)),
            std::min(255, static_cast<int>(m_color.green() + (255 - m_color.green()) * flash)),
            std::min(255, static_cast<int>(m_color.blue() + (255 - m_color.blue()) * flash)));
        qreal glowR = r * 1.3;
        painter->setBrush(QColor(255, 255, 255, 100 + 50 * qSin(shieldTimer * 5.0)));
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(QPointF(0, 0), glowR, glowR);
    }

    painter->setBrush(drawColor);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QPointF(0, 0), r, r);

    if (hasEffect(EffectType::Invisible)) {
        painter->setOpacity(0.3);
    }

    painter->setBrush(Qt::white);
    painter->drawEllipse(QPointF(0, 0), r * 0.3, r * 0.3);

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
