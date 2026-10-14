#ifndef BULLET_H
#define BULLET_H
#include <QPointF>      // 子弹坐标
#include <QPainter>     // 绘制子弹
class Enemy;            // 提前声明 Enemy，避免头文件互相包含
class Bullet
{
public:
    // startPos：子弹起点，也就是防御塔位置
    // target：子弹追踪的敌人
    // damage：命中后造成的伤害
    Bullet(const QPointF& startPos, Enemy* target, int damage);

    // 每一帧更新子弹位置
    void update();

    // 绘制子弹
    void draw(QPainter& painter) const;

    // 子弹是否已经完成任务
    // 比如命中敌人、目标死亡、目标到达终点
    bool isFinished() const;

    // 返回子弹当前目标
    // GameWidget 删除敌人前，需要删除指向这个敌人的子弹
    Enemy* target() const;

private:
    QPointF m_pos;
    Enemy* m_target;

    int m_damage;
    double m_speed = 8.0;

    bool m_finished = false;
    int m_size = 8;
};

#endif // BULLET_H
