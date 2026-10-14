#ifndef TOWER_H
#define TOWER_H
#include <QPointF>
#include <QPainter>
#include <QColor>
#include <QVector>
#include <QString>
class Bullet;
class Enemy;
enum class TowerType
{
    Normal,    // 普通塔
    Fast,      // 速射塔
    Heavy      // 重炮塔
};
class Tower
{
public:
    explicit Tower(const QPointF&pos, TowerType type);//pos 表示塔建造的位置
    void draw(QPainter&painter)const;//绘制防御塔
    // 获取塔的位置
    QPointF position() const;

    // 获取塔的建造价格
    int cost() const;
   void updateAttack(QVector<Enemy*>& enemies, QVector<Bullet*>& bullets);
    // 返回塔的类型
    TowerType type() const;
   int level() const;                  // 返回等级
   int damage() const;                 // 返回伤害
   int range() const;                  // 返回范围

   int upgradeCost() const;            // 升级费用
   int sellValue() const;              // 出售返还金币

   bool canUpgrade() const;            // 是否还能升级
   bool upgrade();                     // 执行升级

   bool containsPoint(const QPointF& pos) const;  // 判断鼠标是否点中塔

   void setSelected(bool selected);    // 设置是否选中
   bool isSelected() const;            // 是否被选中
    // 根据塔类型返回价格
    static int costForType(TowerType type);

    // 根据塔类型返回名称
    static QString nameForType(TowerType type);

private:
    QPointF m_pos;          // 塔的位置
TowerType m_type;           // 塔的类型
    int m_cost = 50;        // 建造费用
    int m_range = 120;      // 攻击范围，后面攻击敌人会用
    int m_damage =18;      // 伤害，后面攻击敌人会用

    int m_size = 36;        // 塔显示大小
    int m_level = 1;          // 塔等级
    int m_maxLevel = 3;       // 最大等级
    int m_totalCost = 50;     // 总花费，用来计算出售返还
    bool m_selected = false;  // 是否被选中
    int m_attackInterval = 25;  // 25帧攻击一次
    int m_attackCounter = 0;
    QColor m_bodyColor;         // 塔的颜色
    QString m_text;             // 塔中间显示的字母
private:
    // 根据塔类型设置属性
    void setupByType(TowerType type);

    // 判断敌人是否在攻击范围内
    bool isEnemyInRange(Enemy* enemy) const;
};

#endif // TOWER_H
