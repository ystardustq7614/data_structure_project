#ifndef BULLET_H
#define BULLET_H

#include <QPointF>

class Enemy;

// 维护追踪和伤害；目标是非拥有指针，战局必须在删除敌人之前清理关联子弹。
class Bullet
{
public:
    Bullet(const QPointF& startPos, Enemy* target, int damage);
    void update();
    bool isFinished() const;
    Enemy* target() const;
    QPointF position() const;

private:
    QPointF m_pos;
    Enemy* m_target;
    int m_damage;
    double m_speed = 8.0; // 每个逻辑 tick 移动的像素数。
    bool m_finished = false;
};

#endif // BULLET_H
