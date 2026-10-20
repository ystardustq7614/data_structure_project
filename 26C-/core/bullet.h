/** @file
 * @brief 追踪子弹的移动与一次性伤害；目标生命周期由持有敌人和子弹的 GameSession 保证。 */
#ifndef BULLET_H
#define BULLET_H

#include <QPointF>

class Enemy;

/** 借用目标的追踪弹；不依赖发射塔，售塔不会撤销已发射伤害。 */
class Bullet
{
public:
    /** 从逻辑坐标 startPos 发射，damage 保存发射时的非负伤害值，不随塔升级改变。
     * target 可为空；非空目标必须在子弹被清理前有效，空目标在下次 update 时结束。
     */
    Bullet(const QPointF& startPos, Enemy* target, int damage);
    /** 推进一个逻辑 tick；追上目标时只伤害一次，空/死亡/到终点目标令子弹结束。
     * 结束后重复调用无副作用；检查目标状态不能识别悬空指针，销毁顺序仍由战局保证。
     */
    void update();
    /** @name 只读查询
     * isFinished 为待清理标志；position 单位为逻辑像素，target 返回可空的借用指针。
     * 查询无副作用，不延长目标寿命；不得用非空指针判断目标仍然存活。
     * @{ */
    bool isFinished() const;
    Enemy* target() const;
    QPointF position() const;
    /** @} */

private:
    QPointF m_pos;
    Enemy* m_target;
    int m_damage;
    double m_speed = 8.0; // 每个逻辑 tick 移动的像素数。
    bool m_finished = false;
};

#endif // BULLET_H
