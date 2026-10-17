#include "tower.h"
#include "enemy.h"
#include "bullet.h"
#include <QPen>
#include <QFont>
#include <QPainterPath>
#include <QtMath>
#include <array>

namespace {

const QPixmap& towerPortrait(TowerType type)
{
    // 三座塔的原画只存在于菜单背景的「我方阵营」卡片中。
    static const QPixmap menu(":/images/images/menu_bg.jpg");
    static const std::array<QPixmap, 3> portraits = {
        menu.copy(QRect(708, 452, 82, 84)),   // 普通塔：海星
        menu.copy(QRect(809, 451, 95, 87)),   // 速射塔：黄色炮台
        menu.copy(QRect(914, 448, 89, 91))    // 重炮塔：红色炮台
    };
    if (type == TowerType::Fast) return portraits[1];
    if (type == TowerType::Heavy) return portraits[2];
    return portraits[0];
}

} // namespace
// Tower 构造函数
Tower::Tower(const QPointF& pos, TowerType type)
    : m_pos(pos),m_type(type)        // 把传进来的位置保存到 m_pos
{setupByType(type);   // 根据塔类型设置不同属性
}
void Tower::setupByType(TowerType type)
{
    if (type == TowerType::Normal) {
        m_cost = 50;                 // 价格便宜
        m_range = 120;               // 范围中等
        m_damage = 18;               // 伤害中等
        m_attackInterval = 25;       // 攻速中等
        m_size = 36;
        m_bodyColor = QColor(80, 140, 230);
        m_text = "N";                // Normal
    }
    else if (type == TowerType::Fast) {
        m_cost = 80;                 // 稍贵
        m_range = 110;               // 范围略小
        m_damage = 10;               // 单次伤害低
        m_attackInterval = 10;       // 攻击很快，适合打 DDL 快怪
        m_size = 34;
        m_bodyColor = QColor(80, 200, 120);
        m_text = "F";                // Fast
    }
    else if (type == TowerType::Heavy) {
        m_cost = 120;                // 最贵
        m_range = 165;               // 范围大
        m_damage = 45;               // 伤害高，适合打 Virus 和 Boss
        m_attackInterval = 55;       // 攻击慢
        m_size = 44;
        m_bodyColor = QColor(220, 120, 60);
        m_text = "H";                // Heavy
    }
    m_totalCost = m_cost;

}
// 每一帧更新塔的攻击逻辑
void Tower::updateAttack(QVector<Enemy*>& enemies, QVector<Bullet*>& bullets)
{
    m_attackCounter++;  // 攻击计数器每帧 +1

    // 还没到攻击间隔，不攻击
    if (m_attackCounter < m_attackInterval) {
        return;
    }

    // 到了攻击时间，重置计数器
    m_attackCounter = 0;

    // 遍历敌人，找第一个在攻击范围内的敌人
    for (Enemy* enemy : enemies) {
        if (enemy->isDead()) {
            continue;
        }

        if (enemy->hasReachedEnd()) {
            continue;
        }

        if (isEnemyInRange(enemy)) {
            Bullet* bullet = new Bullet(m_pos, enemy, m_damage);
            bullets.append(bullet);
            return;
        }
    }
}



// 判断敌人是否在攻击范围内
bool Tower::isEnemyInRange(Enemy* enemy) const
{
    QPointF enemyPos = enemy->position();

    double dx = enemyPos.x() - m_pos.x();
    double dy = enemyPos.y() - m_pos.y();

    double distance = qSqrt(dx * dx + dy * dy);

    return distance <= m_range;
}

// 绘制防御塔
void Tower::draw(QPainter& painter) const
{
    painter.save();
    const int half = m_size / 2;
    // 如果塔被选中，显示攻击范围
    if (m_selected) {
        painter.setPen(QPen(QColor(255, 200, 60, 150), 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(m_pos, m_range, m_range);
    }
    const QRectF body(m_pos.x() - half, m_pos.y() - half, m_size, m_size);
    const QPixmap& portrait = towerPortrait(m_type);
    if (!portrait.isNull()) {
        painter.save();
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        QPainterPath clip;
        clip.addRoundedRect(body, 6, 6);
        painter.setClipPath(clip);
        painter.drawPixmap(body, portrait, QRectF(portrait.rect()));
        painter.restore();

        painter.setPen(QPen(m_bodyColor, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(body, 6, 6);

        const QRectF badge(body.left() - 1, body.bottom() - 15, 16, 16);
        painter.setPen(QPen(QColor(40, 60, 90), 1));
        painter.setBrush(QColor(255, 248, 225));
        painter.drawEllipse(badge);
        QFont font = painter.font();
        font.setBold(true);
        font.setPixelSize(11);
        painter.setFont(font);
        painter.setPen(QColor(20, 40, 80));
        painter.drawText(badge, Qt::AlignCenter, QString::number(m_level));
    } else {
        // 菜单背景缺失时仍可辨识塔型和等级。
        painter.setPen(QPen(QColor(40, 60, 90), 3));
        painter.setBrush(m_bodyColor);
        painter.drawEllipse(m_pos, half, half);
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(12);
        painter.setFont(font);
        painter.setPen(QColor(20, 40, 80));
        painter.drawText(body, Qt::AlignCenter, m_text + QString::number(m_level));
    }
    painter.restore();
}


// 返回塔的位置
QPointF Tower::position() const
{
    return m_pos;
}


// 返回塔的价格
int Tower::cost() const
{
    return m_cost;
}
// 返回塔类型
TowerType Tower::type() const
{
    return m_type;
}


// 根据类型返回价格
int Tower::costForType(TowerType type)
{
    if (type == TowerType::Normal) {
        return 50;
    }
    else if (type == TowerType::Fast) {
        return 80;
    }
    else if (type == TowerType::Heavy) {
        return 120;
    }

    return 50;
}


// 根据类型返回名字
int Tower::radiusForType(TowerType type)
{
    if (type == TowerType::Fast) return 17;
    if (type == TowerType::Heavy) return 22;
    return 18;
}

QString Tower::nameForType(TowerType type)
{
    if (type == TowerType::Normal) {
        return "普通塔";
    }
    else if (type == TowerType::Fast) {
        return "速射塔";
    }
    else if (type == TowerType::Heavy) {
        return "重炮塔";
    }

    return "普通塔";
}
int Tower::level() const
{
    return m_level;
}

int Tower::damage() const
{
    return m_damage;
}

int Tower::range() const
{
    return m_range;
}

int Tower::upgradeCost() const
{
    return m_cost * m_level;
}

int Tower::sellValue() const
{
    return m_totalCost / 2;
}

bool Tower::canUpgrade() const
{
    return m_level < m_maxLevel;
}

bool Tower::upgrade()
{
    if (!canUpgrade()) {
        return false;
    }

    int cost = upgradeCost();

    m_level++;
    m_totalCost += cost;

    m_damage += 10;
    m_range += 12;

    if (m_attackInterval > 8) {
        m_attackInterval -= 3;
    }

    m_size += 2;

    return true;
}

bool Tower::containsPoint(const QPointF& pos) const
{
    double dx = pos.x() - m_pos.x();
    double dy = pos.y() - m_pos.y();
    double distance = qSqrt(dx * dx + dy * dy);

    return distance <= m_size / 2 + 8;
}

void Tower::setSelected(bool selected)
{
    m_selected = selected;
}

bool Tower::isSelected() const
{
    return m_selected;
}
