#include "tower.h"
#include "enemy.h"

#include <QtMath>

const Tower::BaseStats& Tower::baseStatsForType(TowerType type)
{
    // 构造、价格查询和放置半径共用不可变基础表，避免三处参数漂移。
    static constexpr BaseStats normal{50, 120, 18, 25, 36};
    static constexpr BaseStats fast{80, 110, 10, 10, 34};
    static constexpr BaseStats heavy{120, 165, 45, 55, 44};
    switch (type) {
    case TowerType::Fast: return fast;
    case TowerType::Heavy: return heavy;
    case TowerType::Normal: return normal;
    }
    return normal;
}

Tower::Tower(const QPointF& pos, TowerType type)
    : m_pos(pos), m_type(type), m_baseStats(baseStatsForType(type)),
      m_range(m_baseStats.range), m_damage(m_baseStats.damage),
      m_bodyDiameter(m_baseStats.bodyDiameter), m_totalCost(m_baseStats.cost),
      m_attackInterval(m_baseStats.attackInterval)
{
}

Enemy* Tower::updateAttack(const QVector<Enemy*>& enemies)
{
    ++m_attackCounter;
    if (m_attackCounter < m_attackInterval) {
        return nullptr;
    }
    // 即使本 tick 没有目标也重置，保持原有攻击相位。
    m_attackCounter = 0;
    for (Enemy* enemy : enemies) {
        if (enemy->isDead() || enemy->hasReachedEnd()) {
            continue;
        }
        if (isEnemyInRange(enemy)) {
            return enemy;
        }
    }
    return nullptr;
}

bool Tower::isEnemyInRange(const Enemy* enemy) const
{
    const QPointF enemyPos = enemy->position();
    const double dx = enemyPos.x() - m_pos.x();
    const double dy = enemyPos.y() - m_pos.y();
    return qSqrt(dx * dx + dy * dy) <= m_range;
}

QPointF Tower::position() const
{
    return m_pos;
}

int Tower::cost() const
{
    return m_baseStats.cost;
}

TowerType Tower::type() const
{
    return m_type;
}

int Tower::costForType(TowerType type)
{
    return baseStatsForType(type).cost;
}

int Tower::radiusForType(TowerType type)
{
    return baseStatsForType(type).bodyDiameter / 2;
}

int Tower::level() const
{
    return m_level;
}

int Tower::damage() const
{
    return m_damage;
}

int Tower::range() const
{
    return m_range;
}

int Tower::bodyDiameter() const
{
    return m_bodyDiameter;
}

int Tower::upgradeCost() const
{
    return cost() * m_level;
}

int Tower::sellValue() const
{
    return m_totalCost / 2;
}

bool Tower::canUpgrade() const
{
    return m_level < MaxLevel;
}

bool Tower::upgrade()
{
    if (!canUpgrade()) {
        return false;
    }
    const int nextCost = upgradeCost();
    ++m_level;
    m_totalCost += nextCost;
    m_damage += 10;
    m_range += 12;
    // 保留旧规则：Fast 的攻击间隔为 10 -> 7 -> 7，不强行钳制为 8。
    if (m_attackInterval > 8) {
        m_attackInterval -= 3;
    }
    m_bodyDiameter += 2;
    return true;
}

bool Tower::containsPoint(const QPointF& pos) const
{
    const double dx = pos.x() - m_pos.x();
    const double dy = pos.y() - m_pos.y();
    return qSqrt(dx * dx + dy * dy) <= m_bodyDiameter / 2 + 8;
}
