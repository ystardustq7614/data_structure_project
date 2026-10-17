#ifndef ENEMY_H
#define ENEMY_H

#include <QPointF>
#include <QString>
#include <QVector>

// 只维护敌人的移动、生命和强化规则；图片、文字与绘制由展示层负责。
class Enemy
{
public:
    Enemy(const QString& type, const QVector<QPointF>& path);

    void update();
    bool isDead() const;
    bool hasReachedEnd() const;
    QPointF position() const;
    int damageToLife() const;
    void takeDamage(int damage);
    int reward() const;
    void applyLateGameEnhance();
    void applyBossAura();
    void removeBossAura();
    bool isBoss() const;
    int armor() const;
    int hp() const;
    int maxHp() const;

    const QString& type() const { return m_type; }
    bool hasMutated() const { return m_hasMutated; }
    bool lateGameEnhanced() const { return m_lateGameEnhanced; }
    // 变异会覆盖旧的展示状态；记录顺序，供展示层还原原有备用颜色。
    bool mutatedAfterLateEnhancement() const { return m_mutatedAfterLateEnhancement; }

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
