#include "enemy.h"
#include <QtMath>
#include <Qcolor>

Enemy::Enemy(const QString& type, const QVector<QPointF>& path):m_path(path),m_type(type) {
    if(!m_path.isEmpty()){
        m_pos=m_path.first();
    }
    else{
        m_reachedEnd=true;
    }
    setupByType(type);
}
void Enemy::setupByType(const QString&type)
{
    if(type=="bug"){
        m_hp=40;
        m_maxHp=40;
        m_baseMaxHp = 40;
        m_speed=2.4;
        m_reward=10;
        m_armor = 0;
        m_baseArmor = 0;
        m_iconSize=65;
        // 尝试加载敌人图标
        // 无图标 draw 函数会自动画红色圆形代替
        m_icon = QPixmap(":/images/images/enemy_bug.png")
                     .scaled(m_iconSize,
                             m_iconSize,
                             Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    }
    else if (type == "ddl") {
        m_displayName = "DDL";          // 快速怪
        m_hp = 28;                      // 血量较低
        m_maxHp = 28;
        m_baseMaxHp = 28;
        m_speed = 3.6;                  // 速度快
        m_reward = 15;
        m_iconSize = 65;
        m_armor = 0;
        m_baseArmor = 0;
        m_color = QColor(240, 150, 50); // 橙色

        m_icon = QPixmap(":/images/images/enemy.ddl.gif")
                     .scaled(m_iconSize,
                             m_iconSize,
                             Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    }
    else if (type == "virus") {
        m_armor = 8;
        m_baseArmor = 8;
        m_displayName = "Virus";        // 厚血怪
        m_hp =130;                      // 血量高
        m_maxHp = 130;
        m_baseMaxHp = 130;
        m_speed = 2.0;                  // 速度慢
        m_reward = 25;
        m_iconSize = 65;
        m_color = QColor(130, 80, 200); // 紫色

        m_icon = QPixmap(":/images/images/enemy_virus.png")
                     .scaled(m_iconSize,
                             m_iconSize,
                             Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    }
    else if (type == "boss") {
        m_displayName = "Boss";         // 最终 Boss
        m_hp = 550;                     // 超高血量
        m_maxHp = 550;
        m_baseMaxHp = 550;
        m_speed = 1.35;                  // 慢速推进
        m_reward = 100;
        m_iconSize = 75;
        m_color = QColor(70, 70, 70);   // 深灰色
        m_armor = 18;
        m_baseArmor = 18;
        m_icon = QPixmap(":/images/images/enemy_boss.png")
                     .scaled(m_iconSize,
                             m_iconSize,
                             Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    }
    else {
        // 如果传进来的类型不认识，就默认当作 Bug 怪
        m_displayName = "Bug";
        m_hp = 30;
        m_maxHp = 30;
        m_baseMaxHp = 30;
        m_speed = 2.2;
        m_reward = 10;
        m_iconSize =65;
        m_armor = 0;
        m_baseArmor = 0;
        m_color = QColor(220, 70, 70);
    }
    m_baseSpeed = m_speed;
}
void Enemy::update(){
    if(m_reachedEnd||isDead()){
        return;
    }
    updateSpecialAbility();
    if(m_targetIndex>=m_path.size()){
        m_reachedEnd=true;
        return;
    }
    QPointF target=m_path[m_targetIndex];
    //计算目标点与当前位置偏差
    double dx=target.x()-m_pos.x();
    double dy=target.y()-m_pos.y();
    double distance=qSqrt(dx*dx+dy*dy);
    // 如果距离已经很近，直接把敌人放到目标点上
    if (distance <= m_speed) {
        m_pos = target;        // 到达目标点
        m_targetIndex++;       // 下一个目标点

        // 如果已经没有下一个目标点，说明到达终点
        if (m_targetIndex >= m_path.size()) {
            m_reachedEnd = true;
        }
    }
    // 如果还没到目标点，就继续朝目标点移动
    else {
        // dx / distance 和 dy / distance 表示方向
        // 乘以 m_speed 表示每一帧移动多少距离
        m_pos.setX(m_pos.x() + m_speed * dx / distance);
        m_pos.setY(m_pos.y() + m_speed * dy / distance);
    }
}
void Enemy::draw(QPainter&painter)const{
    int half=m_iconSize/2;
    // 如果图片加载成功，就画图片
    if(!m_icon.isNull())
    {
        painter.drawPixmap(static_cast<int>(m_pos.x() - half),  // 图片左上角 x
            static_cast<int>(m_pos.y() - half),  // 图片左上角 y
            m_iconSize,                          // 图片宽度
            m_iconSize,                          // 图片高度
            m_icon                               // 图片对象
            );}
    // 如果图片没加载成功，就画一个红色圆形代替
    else {
        painter.setPen(Qt::NoPen);               // 不画边框
        painter.setBrush(m_color);             // 根据敌人类型使用不同颜色
        painter.drawEllipse(m_pos, half, half);  // 以 m_pos 为圆心画圆

        painter.setPen(Qt::white);               // 设置文字颜色为白色

        // 在圆形中间写 Bug
        painter.drawText(
            QRectF(m_pos.x() - half,
                   m_pos.y() - half,
                   m_iconSize,
                   m_iconSize),
            Qt::AlignCenter,
            m_displayName
            );
    }
    // 下面开始画敌人头顶血条

    int barWidth = m_iconSize;        // 血条宽度和图标一样
    int barHeight = 5;                // 血条高度
    int barX = static_cast<int>(m_pos.x() - barWidth / 2);  // 血条左上角 x
    int barY = static_cast<int>(m_pos.y() - half - 12);     // 血条左上角 y

    // 画血条背景，红色
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(180, 40, 40));
    painter.drawRect(barX, barY, barWidth, barHeight);

    // 画当前血量，绿色
    painter.setBrush(QColor(60, 200, 80));

    // 根据当前血量计算绿色血条长度
    int currentWidth = barWidth * m_hp / m_maxHp;

    painter.drawRect(barX, barY, currentWidth, barHeight);
          }
    bool Enemy::isDead()const{
    return m_hp<=0;
    }
    bool Enemy::hasReachedEnd()const{
    return m_reachedEnd;
    }
    QPointF Enemy::position()const{
    return m_pos;
    }
void Enemy::takeDamage(int damage)
{
    int realDamage = damage - m_armor;   // 护甲抵消伤害

    if (realDamage < 1) {
        realDamage = 1;                  // 最低造成1点伤害，避免完全打不动
    }

    m_hp -= realDamage;

    if (m_hp < 0) {
        m_hp = 0;
    }
}
int Enemy::reward()const{
    return m_reward;
}
int Enemy::damageToLife() const
{
    if (m_type == "virus") {
        return 2;       // 厚血怪漏掉扣 2 点生命
    }
    else if (m_type == "boss") {
        return 5;       // Boss 漏掉扣 5 点生命
    }
    else if (m_type == "mutant") {
        return 2;       // 变异 Virus 扣 2
    }
    return 1;           // Bug 和 DDL 默认扣 1 点生命
}
void Enemy::updateSpecialAbility()
{
    // DDL 冲刺机制
    if (m_type == "ddl") {
        // 如果正在冲刺，倒计时
        if (m_isDashing) {
            m_dashCounter--;

            if (m_dashCounter <= 0) {
                m_isDashing = false;
                m_speed = m_baseSpeed;      // 冲刺结束，恢复原速度
            }

            return;
        }

        // 已经冲刺过，就不再冲刺
        if (m_hasDashed) {
            return;
        }

        // 走到路径中段以后触发冲刺
        if (m_targetIndex >= m_path.size() / 2) {
            m_hasDashed = true;
            m_isDashing = true;
            m_dashCounter = m_dashDuration;

            m_speed = m_baseSpeed * 1.6;    // 冲刺速度
        }

        return;
    }

    // Virus 变异机制
    if (m_type == "virus") {
        // 每个 Virus 只变异一次
        if (m_hasMutated) {
            return;
        }

        // 走到路线后半段还没死，就触发变异
        if (m_targetIndex >= m_path.size() / 2) {
            mutateVirus();
        }

        return;
    }
}
void Enemy::mutateVirus()
{
    m_hasMutated = true;

    // 类型改成 mutant，后面扣血和显示都可以区分
    m_type = "mutant";
    m_displayName = "Mutant";

    // 变异后基础属性提高
    m_baseMaxHp = 160;
    m_baseArmor = 12;

    // 重新根据强化标志计算最新上限和护甲
    m_maxHp = calculateMaxHp();
    m_armor = calculateArmor();

    // 当前血量增加，但不超过最大血量
    m_hp += 120;
    if (m_hp > m_maxHp) {
        m_hp = m_maxHp;
    }

    // 变异后速度略微降低，但更难打死
    m_speed = 1.2;
    m_baseSpeed = m_speed;

    // 奖励提高
    m_reward = 60;

    // 体型变大
    m_iconSize = 65;

    // 没有图片时显示更深的紫色
    m_color = QColor(80, 40, 150);

    QString iconPath = ":/images/images/enemy_mutant.png";
    m_icon = QPixmap(iconPath)
                 .scaled(m_iconSize,
                         m_iconSize,
                         Qt::KeepAspectRatio,
                         Qt::SmoothTransformation);
}

void Enemy::applyLateGameEnhance()
{
    if (m_lateGameEnhanced) {
        return;
    }

    m_lateGameEnhanced = true;

    // 第4波后强化：血量提升30%，护甲+3
    int oldMaxHp = m_maxHp;
    m_maxHp = calculateMaxHp();
    m_armor = calculateArmor();

    if (oldMaxHp > 0) {
        m_hp = static_cast<int>(m_hp * 1.3);
        if (m_hp > m_maxHp) {
            m_hp = m_maxHp;
        }
    }

    // 颜色变深，没图片时也能看出强化
    m_color = m_color.darker(130);

    // 给强化敌人预留图片路径
    QString iconPath;

    if (m_type == "bug") {
        iconPath = ":/images/images/enemy_bug.png";
    }
    else if (m_type == "ddl") {
        iconPath = ":/images/images/enemy.ddl.gif";
    }
    else if (m_type == "virus") {
        iconPath = ":/images/images/enemy_virus.png";
    }
    else if (m_type == "mutant") {
        iconPath = ":/images/images/enemy_mutant.png";
    }
    else if (m_type == "boss") {
        iconPath = ":/images/images/enemy_boss.png";
    }

    if (!iconPath.isEmpty()) {
        m_icon = QPixmap(iconPath)
        .scaled(m_iconSize,
                m_iconSize,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation);
    }
}

void Enemy::applyBossAura()
{
    if (m_bossAuraApplied) {
        return;
    }

    m_bossAuraApplied = true;

    // Boss光环：血量提升50%
    int oldMaxHp = m_maxHp;
    m_maxHp = calculateMaxHp();

    if (oldMaxHp > 0 && m_hp > 0) {
        m_hp = static_cast<int>((static_cast<long long>(m_hp) * m_maxHp + oldMaxHp / 2) / oldMaxHp);
        if (m_hp > m_maxHp) {
            m_hp = m_maxHp;
        }
    }
}

void Enemy::removeBossAura()
{
    if (!m_bossAuraApplied) {
        return;
    }

    m_bossAuraApplied = false;

    int oldMaxHp = m_maxHp;
    m_maxHp = calculateMaxHp();

    if (isDead() || m_hp <= 0) {
        m_hp = 0;
        return;
    }

    if (oldMaxHp > 0) {
        m_hp = static_cast<int>((static_cast<long long>(m_hp) * m_maxHp + oldMaxHp / 2) / oldMaxHp);
        if (m_hp > m_maxHp) {
            m_hp = m_maxHp;
        }
        if (m_hp <= 0) {
            m_hp = 1;
        }
    }
}

int Enemy::calculateMaxHp() const
{
    int hp = m_baseMaxHp;
    if (m_lateGameEnhanced) {
        hp = static_cast<int>(hp * 1.3);
    }
    if (m_bossAuraApplied) {
        hp = static_cast<int>(hp * 1.5);
    }
    return hp;
}

int Enemy::calculateArmor() const
{
    int arm = m_baseArmor;
    if (m_lateGameEnhanced) {
        arm += 3;
    }
    return arm;
}

bool Enemy::isBoss() const
{
    return m_type == "boss";
}

int Enemy::armor() const
{
    return m_armor;
}
