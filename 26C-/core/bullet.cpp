/** @file
 * @brief 按目标当前位置追踪并结算一次伤害；对象删除与目标引用安全由战局统一控制。 */
#include "bullet.h"
#include "enemy.h"

#include <QtMath>

Bullet::Bullet(const QPointF& startPos, Enemy* target, int damage)
    : m_pos(startPos), m_target(target), m_damage(damage)
{
}

void Bullet::update()
{
    if (m_finished) {
        return;
    }
    if (m_target == nullptr) {
        m_finished = true;
        return;
    }
    // 这项检查不能识别悬空指针；拥有者仍须先删关联子弹，再销毁目标敌人。
    if (m_target->isDead() || m_target->hasReachedEnd()) {
        m_finished = true;
        return;
    }

    const QPointF targetPos = m_target->position();
    const double dx = targetPos.x() - m_pos.x();
    const double dy = targetPos.y() - m_pos.y();
    const double distance = qSqrt(dx * dx + dy * dy);
    // 抵达条件也覆盖零距离，先结算再返回，避免后续归一化方向时除以零。
    if (distance <= m_speed) {
        m_target->takeDamage(m_damage);
        m_finished = true;
        return;
    }
    m_pos.setX(m_pos.x() + m_speed * dx / distance);
    m_pos.setY(m_pos.y() + m_speed * dy / distance);
}

bool Bullet::isFinished() const
{
    return m_finished;
}

Enemy* Bullet::target() const
{
    return m_target;
}

QPointF Bullet::position() const
{
    return m_pos;
}
