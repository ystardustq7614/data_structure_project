#include "bullet.h"
#include "enemy.h"      // 子弹需要读取敌人位置，也需要对敌人造成伤害

#include <QtMath>
#include <QColor>


// Bullet 构造函数
Bullet::Bullet(const QPointF& startPos, Enemy* target, int damage)
    : m_pos(startPos),       // 子弹从防御塔位置出发
    m_target(target),      // 保存目标敌人
    m_damage(damage)       // 保存伤害
{
}


// 更新子弹位置
void Bullet::update()
{
    // 如果子弹已经结束，就不再更新
    if (m_finished) {
        return;
    }

    // 如果目标不存在，子弹结束
    if (m_target == nullptr) {
        m_finished = true;
        return;
    }

    // 如果目标已经死亡或者到达终点，子弹结束
    if (m_target->isDead() || m_target->hasReachedEnd()) {
        m_finished = true;
        return;
    }

    // 获取目标敌人位置
    QPointF targetPos = m_target->position();

    // 计算子弹到目标的方向
    double dx = targetPos.x() - m_pos.x();
    double dy = targetPos.y() - m_pos.y();

    // 计算子弹到目标的距离
    double distance = qSqrt(dx * dx + dy * dy);

    // 如果距离很近，认为命中
    if (distance <= m_speed) {
        m_target->takeDamage(m_damage);  // 对敌人造成伤害
        m_finished = true;               // 子弹命中后消失
        return;
    }

    // 没命中时，继续朝目标方向移动
    m_pos.setX(m_pos.x() + m_speed * dx / distance);
    m_pos.setY(m_pos.y() + m_speed * dy / distance);
}


// 绘制子弹
void Bullet::draw(QPainter& painter) const
{
    if (m_finished) {
        return;
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 220, 80));  // 黄色子弹
    painter.drawEllipse(m_pos, m_size, m_size);
}

// 判断子弹是否结束
bool Bullet::isFinished() const
{
    return m_finished;
}

// 返回子弹目标
Enemy* Bullet::target() const
{
    return m_target;
}
