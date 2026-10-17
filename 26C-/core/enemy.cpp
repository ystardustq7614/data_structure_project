#include "enemy.h"

#include <QtMath>

Enemy::Enemy(const QString& type, const QVector<QPointF>& path)
    : m_path(path), m_type(type)
{
    if (!m_path.isEmpty()) {
        m_pos = m_path.first();
    } else {
        m_reachedEnd = true;
    }
    setupByType(type);
}

void Enemy::setupByType(const QString& type)
{
    if (type == "bug") {
        m_hp = 40;
        m_maxHp = 40;
        m_speed = 2.4;
        m_reward = 10;
        m_armor = 0;
    } else if (type == "ddl") {
        m_hp = 28;
        m_maxHp = 28;
        m_speed = 3.6;
        m_reward = 15;
        m_armor = 0;
    } else if (type == "virus") {
        m_hp = 130;
        m_maxHp = 130;
        m_speed = 2.0;
        m_reward = 25;
        m_armor = 8;
    } else if (type == "boss") {
        m_hp = 550;
        m_maxHp = 550;
        m_speed = 1.35;
        m_reward = 100;
        m_armor = 18;
    } else {
        // 未知类型保留旧版独立的备用数值，不套用正常 Bug 的 40 HP。
        m_hp = 30;
        m_maxHp = 30;
        m_speed = 2.2;
        m_reward = 10;
    }
    m_baseMaxHp = m_maxHp;
    m_baseArmor = m_armor;
    m_baseSpeed = m_speed;
}

void Enemy::update()
{
    if (m_reachedEnd || isDead()) {
        return;
    }
    updateSpecialAbility();
    if (m_targetIndex >= m_path.size()) {
        m_reachedEnd = true;
        return;
    }

    const QPointF target = m_path[m_targetIndex];
    const double dx = target.x() - m_pos.x();
    const double dy = target.y() - m_pos.y();
    const double distance = qSqrt(dx * dx + dy * dy);
    if (distance <= m_speed) {
        m_pos = target;
        ++m_targetIndex;
        if (m_targetIndex >= m_path.size()) {
            m_reachedEnd = true;
        }
    } else {
        // 归一化方向后前进，保持原有每 tick 速度与转角处理顺序。
        m_pos.setX(m_pos.x() + m_speed * dx / distance);
        m_pos.setY(m_pos.y() + m_speed * dy / distance);
    }
}

bool Enemy::isDead() const
{
    return m_hp <= 0;
}

bool Enemy::hasReachedEnd() const
{
    return m_reachedEnd;
}

QPointF Enemy::position() const
{
    return m_pos;
}

void Enemy::takeDamage(int damage)
{
    int realDamage = damage - m_armor;
    if (realDamage < 1) {
        realDamage = 1;
    }
    m_hp -= realDamage;
    if (m_hp < 0) {
        m_hp = 0;
    }
}

int Enemy::reward() const
{
    return m_reward;
}

int Enemy::damageToLife() const
{
    if (m_type == "virus") {
        return 2;
    } else if (m_type == "boss") {
        return 5;
    } else if (m_type == "mutant") {
        return 2;
    }
    return 1;
}

void Enemy::updateSpecialAbility()
{
    if (m_type == "ddl") {
        if (m_isDashing) {
            --m_dashCounter;
            if (m_dashCounter <= 0) {
                m_isDashing = false;
                m_speed = m_baseSpeed;
            }
            return;
        }
        if (m_hasDashed) {
            return;
        }
        if (m_targetIndex >= m_path.size() / 2) {
            m_hasDashed = true;
            m_isDashing = true;
            m_dashCounter = m_dashDuration;
            m_speed = m_baseSpeed * 1.6;
        }
        return;
    }

    if (m_type == "virus") {
        if (m_hasMutated) {
            return;
        }
        if (m_targetIndex >= m_path.size() / 2) {
            mutateVirus();
        }
    }
}

void Enemy::mutateVirus()
{
    m_hasMutated = true;
    m_mutatedAfterLateEnhancement = m_lateGameEnhanced;
    m_type = "mutant";

    // 从基础值重新计算，不能覆盖已经获得的晚期强化和 Boss 光环。
    m_baseMaxHp = 160;
    m_baseArmor = 12;
    m_maxHp = calculateMaxHp();
    m_armor = calculateArmor();
    m_hp += 120;
    if (m_hp > m_maxHp) {
        m_hp = m_maxHp;
    }
    m_speed = 1.2;
    m_baseSpeed = m_speed;
    m_reward = 60;
}

void Enemy::applyLateGameEnhance()
{
    if (m_lateGameEnhanced) {
        return;
    }
    m_lateGameEnhanced = true;
    m_maxHp = calculateMaxHp();
    m_hp = static_cast<int>(m_hp * 1.3);
    if (m_hp > m_maxHp) {
        m_hp = m_maxHp;
    }
    m_armor = calculateArmor();
}

void Enemy::applyBossAura()
{
    if (m_bossAuraApplied) {
        return;
    }
    const int oldMaxHp = m_maxHp;
    m_bossAuraApplied = true;
    m_maxHp = calculateMaxHp();
    // 保留整数四舍五入的比例映射；0 HP 不参与映射，不能复活。
    if (oldMaxHp > 0 && m_hp > 0) {
        m_hp = static_cast<int>((static_cast<long long>(m_hp) * m_maxHp + oldMaxHp / 2) / oldMaxHp);
        if (m_hp > m_maxHp) {
            m_hp = m_maxHp;
        }
    }
}

void Enemy::removeBossAura()
{
    if (!m_bossAuraApplied) {
        return;
    }
    const int oldMaxHp = m_maxHp;
    m_bossAuraApplied = false;
    m_maxHp = calculateMaxHp();
    if (oldMaxHp > 0 && m_hp > 0) {
        m_hp = static_cast<int>((static_cast<long long>(m_hp) * m_maxHp + oldMaxHp / 2) / oldMaxHp);
        if (m_hp < 1) {
            m_hp = 1;
        }
        if (m_hp > m_maxHp) {
            m_hp = m_maxHp;
        }
    }
}

int Enemy::calculateMaxHp() const
{
    int maxHp = m_baseMaxHp;
    if (m_lateGameEnhanced) {
        maxHp = static_cast<int>(maxHp * 1.3);
    }
    if (m_bossAuraApplied) {
        maxHp = static_cast<int>(maxHp * 1.5);
    }
    return maxHp;
}

int Enemy::calculateArmor() const
{
    return m_baseArmor + (m_lateGameEnhanced ? 3 : 0);
}

bool Enemy::isBoss() const
{
    return m_type == "boss";
}

int Enemy::armor() const
{
    return m_armor;
}

int Enemy::hp() const
{
    return m_hp;
}

int Enemy::maxHp() const
{
    return m_maxHp;
}
