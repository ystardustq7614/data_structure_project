/** @file
 * @brief 敌人的逐 tick 移动、技能、伤害与强化规则；战局负责生成/销毁，展示层负责素材。 */
#ifndef ENEMY_H
#define ENEMY_H

#include <QPointF>
#include <QString>
#include <QVector>

/** 单个敌人的可变规则状态；自行保存路径副本，死亡/到终点后由 GameSession 清理。 */
class Enemy
{
public:
    /** type 使用 bug/ddl/virus/boss 类型键，未知键保留备用数值；path 为有序路径副本。
     * 空路径直接标记到终点；构造不注册到战局，也不施加晚期强化或 Boss 光环。
     */
    Enemy(const QString& type, const QVector<QPointF>& path);

    /** 推进一步技能与移动；死亡或已到终点时无副作用，每 tick 最多到达一个路点。 */
    void update();
    /** @name 只读战斗查询
     * position 单位为逻辑像素；reward 是击杀金币，damageToLife 是漏怪扣血，两者不执行结算。
     * hp 可为 0，maxHp 是含强化的上限；死亡与到终点是独立状态，可同时为 true。
     * 无输入参数或失败结果；这些查询不会改变实体。
     * @{ */
    bool isDead() const;
    bool hasReachedEnd() const;
    QPointF position() const;
    int damageToLife() const;
    /** @} */
    /** 对非负原始 damage 扣除护甲后至少造成 1 点伤害；生命最低为 0，不自行发放奖励。 */
    void takeDamage(int damage);
    /** 返回当前击杀奖励；真正变异后奖励改变，获取数值不会发放金币。 */
    int reward() const;
    /** 幂等施加晚期生命/护甲强化；当前生命按 1.3 倍截断，不重置技能与光环标志。 */
    void applyLateGameEnhance();
    /** 幂等施加光环；按新旧生命上限四舍五入映射当前生命，0 HP 保持死亡。 */
    void applyBossAura();
    /** 幂等撤销光环；重算基础/晚期上限并按比例映射，存活者最低保留 1 HP。 */
    void removeBossAura();
    /** @name 类型与属性快照
     * 返回当前类型、强化标志及实际护甲/生命，无失败结果。
     * type() 为借用引用，随本对象存活；引用内容会在真正变异时变为 mutant。
     * @{ */
    bool isBoss() const;
    int armor() const;
    int hp() const;
    int maxHp() const;

    const QString& type() const { return m_type; }
    bool hasMutated() const { return m_hasMutated; }
    bool lateGameEnhanced() const { return m_lateGameEnhanced; }
    // 变异会覆盖旧的展示状态；记录顺序，供展示层还原原有备用颜色。
    bool mutatedAfterLateEnhancement() const { return m_mutatedAfterLateEnhancement; }
    /** @} */

private:
    void setupByType(const QString& type);
    void updateSpecialAbility();
    void mutateVirus();
    int calculateMaxHp() const;
    int calculateArmor() const;

    QVector<QPointF> m_path;
    QPointF m_pos;
    int m_targetIndex = 1;
    int m_hp = 30;
    int m_maxHp = 30;
    int m_baseMaxHp = 30;
    double m_speed = 1.5;      // 每个逻辑 tick 移动的像素数。
    double m_baseSpeed = 1.5;
    bool m_hasDashed = false;
    bool m_isDashing = false;
    int m_dashCounter = 0;
    int m_dashDuration = 60;   // 保留 60 tick 冲刺时长。
    bool m_hasMutated = false;
    bool m_mutatedAfterLateEnhancement = false;
    int m_reward = 10;
    bool m_reachedEnd = false;
    QString m_type;
    int m_armor = 0;
    int m_baseArmor = 0;
    bool m_lateGameEnhanced = false;
    bool m_bossAuraApplied = false;
};

#endif // ENEMY_H
