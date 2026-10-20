/** @file
 * @brief 塔的数值、升级与目标选择；GameSession 负责交易扣款及按返回目标创建子弹。 */
#ifndef TOWER_H
#define TOWER_H

#include <QPointF>
#include <QVector>

class Enemy;

/** 三类塔的稳定类型键；素材数组与基础数值表均按这一顺序对应。 */
enum class TowerType { Normal, Fast, Heavy };

/** 单座塔的可变属性与冷却；只借用敌人，不拥有目标或已发射子弹。 */
class Tower
{
public:
    /** 在逻辑坐标 pos 创建一级 type 塔；不校验放置或扣金币，由战局统一处理。 */
    explicit Tower(const QPointF& pos, TowerType type);

    /** 返回固定建造中心，单位为逻辑像素；查询无副作用或失败结果。 */
    QPointF position() const;
    /** 返回一级建造价格，不包含升级支出；出售价值另由累计投入计算。 */
    int cost() const;
    /** 每 tick 推进冷却；到期后按 enemies 输入顺序选首个存活、未到终点且在射程内的目标。
     * enemies 元素必须为仍有效的非空指针；无目标或冷却未到返回 nullptr。
     * 到期即重置冷却，即使没有目标；返回指针只借用，调用者负责创建子弹。
     */
    Enemy* updateAttack(const QVector<Enemy*>& enemies);
    /** @name 属性与交易报价
     * type/level 为当前塔状态；range/bodyDiameter 为逻辑像素，damage 为未扣护甲伤害。
     * upgradeCost 是当前报价，满级时仍可查询但不可据此判断可升级；sellValue 为累计投入的一半，向下取整。
     * 这些查询不扣款、不升级、无失败结果。
     * @{ */
    TowerType type() const;
    int level() const;
    int damage() const;
    int range() const;
    int bodyDiameter() const;
    int upgradeCost() const;
    int sellValue() const;
    bool canUpgrade() const;
    /** @} */
    /** 满级返回 false 且不修改属性；成功累计升级投入并改变属性，但不扣玩家金币。 */
    bool upgrade();
    /** 检查逻辑坐标 pos 是否落入当前塔身半径加 8 像素的点击容差，边界算命中。 */
    bool containsPoint(const QPointF& pos) const;

    /** 返回 type 的一级建造价格；未知枚举回退普通塔数值，不创建实体或扣款。 */
    static int costForType(TowerType type);
    /** 返回 type 的一级塔身半径，不含点击容差；未知枚举回退普通塔数值。 */
    static int radiusForType(TowerType type);

private:
    struct BaseStats {
        int cost;
        int range;
        int damage;
        int attackInterval;  // 单位为逻辑 tick。
        int bodyDiameter;    // 同时用于实体点击范围和展示层尺寸。
    };
    static const BaseStats& baseStatsForType(TowerType type);
    bool isEnemyInRange(const Enemy* enemy) const;

    QPointF m_pos;
    TowerType m_type;
    const BaseStats m_baseStats;
    int m_range;
    int m_damage;
    int m_bodyDiameter;
    int m_level = 1;
    static constexpr int MaxLevel = 3;
    int m_totalCost;
    int m_attackInterval;
    int m_attackCounter = 0;
};

#endif // TOWER_H
