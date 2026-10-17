#ifndef TOWER_H
#define TOWER_H

#include <QPointF>
#include <QVector>

class Enemy;

enum class TowerType { Normal, Fast, Heavy };

// 维护塔的属性、升级和攻击冷却，不拥有敌人，也不创建或拥有子弹。
class Tower
{
public:
    explicit Tower(const QPointF& pos, TowerType type);

    QPointF position() const;
    int cost() const;
    // 每 tick 更新冷却并返回首个可攻击目标；返回指针只借用，调用者负责生成子弹。
    Enemy* updateAttack(const QVector<Enemy*>& enemies);
    TowerType type() const;
    int level() const;
    int damage() const;
    int range() const;
    int bodyDiameter() const;
    int upgradeCost() const;
    int sellValue() const;
    bool canUpgrade() const;
    bool upgrade();
    bool containsPoint(const QPointF& pos) const;

    static int costForType(TowerType type);
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
