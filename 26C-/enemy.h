#ifndef ENEMY_H
#define ENEMY_H
#include <QPointF>      // QPointF 表示敌人的坐标，比如 x=100.5, y=200.0
#include <QVector>      // QVector 是 Qt 里的数组容器
#include <QPixmap>      // QPixmap 用来保存敌人的图片
#include <QPainter>     // QPainter 用来绘制敌人
#include <QString>      // QString 是 Qt 里的字符串类型
#include <QColor>
class Enemy
{
public:
    // 构造函数
    // type 表示敌人类型，比如 "bug"
    // path 表示敌人要走的路线
    Enemy(const QString&type,const QVector<QPointF>&path);
    void update();
    // 更新敌人状态
    // 目前主要是让敌人沿路径移动
    void draw(QPainter&painter)const;
    // 绘制敌人
    // painter 是从 GameWidget 传进来的画家对象
    bool isDead()const;// 判断敌人是否死亡
    bool hasReachedEnd()const;// 判断敌人是否已经到达终点
    QPointF position()const;// 返回敌人当前坐标
    int damageToLife() const;   // 敌人到达终点时扣多少生命
    void takeDamage(int damage);        // 敌人受到伤害
    int reward()const ;                 // 击败敌人奖励金币
    void applyLateGameEnhance();   // 第4波后敌人强化
    void applyBossAura();          // Boss光环：血量提升50%
    void removeBossAura();         // Boss离场时撤销光环
    bool isBoss() const;           // 判断是否Boss
    int armor() const;             // 返回护甲值
    int hp() const;                // 当前生命
    int maxHp() const;             // 当前生命上限
private:
    QVector<QPointF> m_path;// 敌人移动路径
    QPointF m_pos;// 敌人当前坐标
    int m_targetIndex=1;// 当前正在前往的路径点编号
    int m_hp=30;
    int m_maxHp=30;
    int m_baseMaxHp=30;
    double m_speed=1.5;
    double m_baseSpeed = 1.5;        // 原始速度，用来冲刺结束后恢复

    bool m_hasDashed = false;        // DDL 是否已经冲刺过
    bool m_isDashing = false;        // DDL 是否正在冲刺
    int m_dashCounter = 0;           // 冲刺剩余帧数
    int m_dashDuration = 60;         // 冲刺持续时间，60帧约等于1.8秒
    bool m_hasMutated = false;      // Virus 是否已经变异过
    int m_reward=10;
    bool m_reachedEnd=false;
    QString m_type;
    QPixmap m_icon;
    int m_iconSize=36;
    QString m_displayName;      // 显示名称，例如 Bug、DDL、Virus、Boss
    QColor m_color;             // 没有图片时，用不同颜色区分敌人
    //根据敌人类型设置属性
    void setupByType(const QString& type);
    void updateSpecialAbility();     // 更新敌人的特殊能力，比如 DDL 冲刺
    void mutateVirus();             // Virus 变异成小 Boss
    int calculateMaxHp() const;
    int calculateArmor() const;
    int m_armor = 0;                   // 护甲值，减少受到的伤害
    int m_baseArmor = 0;
    bool m_lateGameEnhanced = false;   // 是否已经获得第4波强化
    bool m_bossAuraApplied = false;    // 是否已经获得Boss光环
};

#endif // ENEMY_H
