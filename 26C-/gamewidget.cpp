#include "gamewidget.h"      // 引入 GameWidget 的头文件
#include <QPainter>          // QPainter 用来绘图
#include <QPen>              // QPen 用来设置线条颜色和粗细
#include <QBrush>            // QBrush 用来设置填充颜色
#include <QPainterPath>      // QPainterPath 用来绘制连续路径
#include <QColor>            // QColor 用来设置颜色
#include <QTimer>
#include <QtMath>
// GameWidget 构造函数
// 当 GameWidget 被创建时，会自动执行这里
GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)       { // 调用 QWidget 的构造函数，因为 GameWidget 继承 QWidget
    setFixedSize(1200, 800);  // 设置游戏窗口大小
    setFocusPolicy(Qt::StrongFocus);
    m_gameMapBackground = QPixmap(":/images/images/game_map.jpg");
    m_winImage = QPixmap(":/images/images/win.png");
    m_loseImage = QPixmap(":/images/images/lose.png");
    initPaths();              // 初始化敌人移动路径
    initSpawnQueue();        // 初始化这一关的出怪顺序
    m_timer = new QTimer(this);  // 创建定时器，this 表示这个定时器属于 GameWidget
    // connect 是 Qt 的信号槽机制
    // 意思是：每当 m_timer 触发 timeout 信号，就调用 updateGame 函数
    connect(m_timer, &QTimer::timeout, this, &GameWidget::updateGame);
    m_timer->start(30);      // 每 30 毫秒触发一次，相当于游戏每 30ms 更新一帧
}


// 析构函数
// 当 GameWidget 被销毁时，会执行这里
GameWidget::~GameWidget()
{
    qDeleteAll(m_enemies);   // 删除 m_enemies 里所有 new 出来的 Enemy 对象
    m_enemies.clear();       // 清空敌人数组
    qDeleteAll(m_towers);      // 删除所有防御塔
    m_towers.clear();
    qDeleteAll(m_bullets);
    m_bullets.clear();
}


// 初始化敌人移动路径
void GameWidget::initPaths()
{
    m_paths.clear();

    // 上方路线
    QVector<QPointF> path1;

    path1 << QPointF(40, 230)
          << QPointF(305, 230)
          << QPointF(305, 90)
          << QPointF(635, 90)
          << QPointF(635, 315)
          << QPointF(1050, 315);

    // 下方路线
    QVector<QPointF> path2;

    path2 << QPointF(40, 570)
          << QPointF(335, 570)
          << QPointF(335, 690)
          << QPointF(760, 690)
          << QPointF(525, 395)
          << QPointF(745, 485)
          << QPointF(1050, 485)
          << QPointF(1050, 315);

    m_paths.append(path1);
    m_paths.append(path2);
}
// 初始化这一关的出怪队列
void GameWidget::initSpawnQueue()
{
    m_waves.clear();          // 清空原来的波次
    m_totalEnemies = 0;       // 总敌人数清零

    // 第 1 波：普通 Bug
    QVector<QString> wave1;
    for (int i = 0; i <8; ++i) {
        wave1.append("bug");
    }

    // 第 2 波：加入快速 DDL
    QVector<QString> wave2;
    for (int i = 0; i < 8; ++i) {
        wave2.append("bug");
    }
    for (int i = 0; i <6; ++i) {
        wave2.append("ddl");
    }

    QVector<QString> wave3;
    for (int i = 0; i < 8; ++i) {
        wave3.append("bug");
    }
    for (int i = 0; i <10; ++i) {
        wave3.append("ddl");
    }

    // 第 4 波：加入厚血怪
    QVector<QString> wave4;
    for (int i = 0; i < 6; ++i) {
        wave4.append("bug");
    }
    for (int i = 0; i <4; ++i) {
        wave4.append("virus");
    }

    // 第 5 波：混合进攻
    QVector<QString> wave5;

    for (int i = 0; i < 10; ++i) {
        wave5.append("ddl");
    }
    for (int i = 0; i <4; ++i) {
        wave5.append("virus");
    }

    // 第 6 波：最终 Boss 波
    QVector<QString> wave6;
    for (int i = 0; i <4; ++i) {
        wave6.append("ddl");
    }
    for (int i = 0; i <3; ++i) {
        wave6.append("virus");
    }
    wave6.append("boss");

    for (int i = 0; i < 2; ++i) {
        wave6.append("virus");
    }
    for (int i = 0; i < 8; ++i) {
        wave6.append("ddl");
    }
    for (int i = 0; i < 8; ++i) {
        wave6.append("ddl");
    }


    m_waves.append(wave1);
    m_waves.append(wave2);
    m_waves.append(wave3);
    m_waves.append(wave4);
    m_waves.append(wave5);
    m_waves.append(wave6);

    for (const QVector<QString>& wave : m_waves) {
        m_totalEnemies += wave.size();
    }

    m_currentWaveIndex = 0;
    m_spawnIndexInWave = 0;
    m_spawnedCount = 0;
    m_wave = 1;
}
// 生成敌人
void GameWidget::spawnEnemy(const QString& type)
{
    // 如果没有路径，就不生成敌人，避免程序崩溃
    if (m_paths.isEmpty()) {
        return;
    }
    int pathIndex = m_nextPathIndex % static_cast<int>(m_paths.size());
    m_nextPathIndex++;
    // 取出本次敌人要走的路径
    QVector<QPointF> selectedPath = m_paths[pathIndex];
    Enemy* enemy = new Enemy(type, selectedPath);

    // 第4波开始，新生成敌人获得强化
    if (m_currentWaveIndex >= 3) {
        enemy->applyLateGameEnhance();
    }

    // Boss光环期间，新生成敌人也获得血量+50%
    if (m_bossAuraActive) {
        enemy->applyBossAura();
    }

    m_enemies.append(enemy);

    // Boss第一次出场时，触发全场光环
    if (type == "boss" && !m_bossAuraTriggered) {
        m_bossAuraTriggered = true;
        m_bossAuraActive = true;

        applyBossAuraToAllEnemies();

        showStatusMessage("警报：Boss出现！所有敌人生命提升50%！", 150);
    }
}

// 游戏更新函数
// 这个函数会被 QTimer 每 30ms 调用一次
void GameWidget::updateGame()
{   // 如果游戏已经结束，就不再更新
    if (m_gameFinished) {
        return;
    }
    // 多波出怪逻辑
    if (m_currentWaveIndex < m_waves.size()) {
        QVector<QString>& currentWave = m_waves[m_currentWaveIndex];

        // 当前波还没出完
        if (m_spawnIndexInWave < currentWave.size()) {
            m_spawnCounter++;
            int currentInterval = m_spawnInterval - m_currentWaveIndex * 2;
            if (currentInterval < 8) {
                currentInterval = 8;
            }
            // 如果当前没有正在进行的小队爆发，就按间隔触发一组小队
            if (m_burstRemaining <= 0) {
                if (m_spawnCounter >= currentInterval) {
                    int spawnCountThisTime = 1;

                    if (m_currentWaveIndex == 0) {
                        spawnCountThisTime = 1;   // 第 1 波每组 1 个
                    }
                    else if (m_currentWaveIndex == 1) {
                        spawnCountThisTime = 1;   // 第 2 波每组 1 个
                    }
                    else if (m_currentWaveIndex == 2) {
                        spawnCountThisTime = 2;   // 第 3 波每组 2 个
                    }
                    else if (m_currentWaveIndex == 3) {
                        spawnCountThisTime = 2;   // 第 4 波每组 2 个
                    }
                    else if (m_currentWaveIndex == 4) {
                        spawnCountThisTime = 3;   // 第 5 波每组 3 个
                    }
                    else if (m_currentWaveIndex >= 5) {
                        spawnCountThisTime = 3;   // 第 6 波每组 3 个
                    }

                    m_burstRemaining = spawnCountThisTime;
                    m_burstGapCounter = 0;
                    m_spawnCounter = 0;
                }
            }
            // 如果正在进行小队爆发，就每隔几帧出一个，避免粘在一起
            else {
                m_burstGapCounter++;

                if (m_burstGapCounter >= m_burstGap) {
                    if (m_spawnIndexInWave < currentWave.size()) {
                        QString enemyType = currentWave[m_spawnIndexInWave];

                        spawnEnemy(enemyType);

                        m_spawnIndexInWave++;
                        m_spawnedCount++;
                        m_burstRemaining--;
                    }
                    else {
                        m_burstRemaining = 0;
                    }

                    m_burstGapCounter = 0;
                }

            }}

        // 当前波已经出完，准备进入下一波
        else {
            if (m_enemies.size() <= 6) {
            // 如果不是最后一波，就等待一小段时间后进入下一波
            if (m_currentWaveIndex < m_waves.size() - 1) {
                m_waveWaitCounter++;

                if (m_waveWaitCounter >= m_waveWaitTime) {
                    m_currentWaveIndex++;       // 进入下一波
                    m_wave = m_currentWaveIndex + 1;

                    m_spawnIndexInWave = 0;     // 新一波从第 0 个敌人开始
                    m_spawnCounter = 0;
                    m_waveWaitCounter = 0;
                    if (m_currentWaveIndex == 3) {
                        showStatusMessage("警报：第4波开始，敌人获得护甲强化！", 150);
                    }
                    if (m_currentWaveIndex == 5) {
                        showStatusMessage("最终波开始！Boss 即将出现！", 150);
                        emit finalWaveStarted();
                    }
                }
            }
        }
    }
}

    // 更新敌人移动
    for (Enemy* enemy : m_enemies) {
        enemy->update();
    }

    // 更新所有防御塔攻击，防御塔会生成子弹
    for (Tower* tower : m_towers) {
        tower->updateAttack(m_enemies, m_bullets);
    }
    // 更新所有子弹
    for (Bullet* bullet : m_bullets) {
        bullet->update();
    }

    // 删除已经命中或失效的子弹
    for (int i = m_bullets.size() - 1; i >= 0; --i) {
        if (m_bullets[i]->isFinished()) {
            delete m_bullets[i];
            m_bullets.removeAt(i);
        }
    }

    // 处理到终点或死亡的敌人
    for (int i = m_enemies.size() - 1; i >= 0; --i) {
        Enemy* enemy = m_enemies[i];

        if (enemy->hasReachedEnd()) {
            m_life -= enemy->damageToLife();
            if (enemy->isBoss()) {
                m_bossAuraActive = false;
                removeBossAuraFromAllEnemies();
            }
            removeBulletsTargeting(enemy);  // 删除所有瞄准这个敌人的子弹
            delete enemy;
            m_enemies.removeAt(i);
            continue;
        }

        if (enemy->isDead()) {

            m_gold += enemy->reward();
            if (enemy->isBoss()) {
                m_bossAuraActive = false;
                removeBossAuraFromAllEnemies();
                showStatusMessage("Boss已被击败，强化光环消失！", 120);
            }

            removeBulletsTargeting(enemy);
            delete enemy;
            m_enemies.removeAt(i);
            continue;
        }
    }
    if (m_statusMessageCounter > 0) {
        m_statusMessageCounter--;
    }//让字幕倒计时
    checkGameResult();    // 检查游戏是否胜利或失败
    update();                         // 通知 Qt 重新绘制窗口，会触发 paintEvent
}


// 绘图函数
// 每次窗口需要重画时，Qt 会自动调用这个函数
void GameWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);                   // 暂时不用 event，防止编译器警告

    QPainter painter(this);            // 创建画家对象，用来在当前窗口上绘图

    // 开启抗锯齿，让圆形和线条更平滑
    painter.setRenderHint(QPainter::Antialiasing, true);

    drawBackground(painter);           // 画地图图片背景

    if (m_debugDrawPath) {
        drawGrid(painter);
        drawPath(painter);
    }
    drawMapMarkers(painter);

    // 先画塔
    for (Tower* tower : m_towers) {
        tower->draw(painter);
    }

    // 再画敌人
    for (Enemy* enemy : m_enemies) {
        enemy->draw(painter);
    }

    // 再画子弹，让子弹显示在敌人和道路上方
    for (Bullet* bullet : m_bullets) {
        bullet->draw(painter);
    }

    // 状态栏最后画，保证在最上层
    drawHud(painter);
    drawStatusMessage(painter);
    if (m_gameFinished) {
        drawGameResultScreen(painter);
    }

}


void GameWidget::drawBackground(QPainter& painter)
{
    if (!m_gameMapBackground.isNull()) {
        painter.drawPixmap(rect(), m_gameMapBackground);
    }
    else {
        painter.fillRect(rect(), QColor(70, 110, 70));

        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPointSize(24);
        font.setBold(true);
        painter.setFont(font);

        painter.drawText(rect(),
                         Qt::AlignCenter,
                         "游戏地图图片加载失败\n请检查 images/game_map.jpg");
    }
}


// 画地图网格
void GameWidget::drawGrid(QPainter& painter)
{
    // 设置网格线颜色和粗细
    painter.setPen(QPen(QColor(210, 220, 210), 1));

    int gridSize = 60;                 // 每个格子 60 像素

    // 画竖线
    for (int x = 0; x <= width(); x += gridSize) {
        painter.drawLine(x, 60, x, height());
    }

    // 画横线
    // 从 y = 60 开始，是因为顶部 60 像素留给状态栏
    for (int y = 60; y <= height(); y += gridSize) {
        painter.drawLine(0, y, width(), y);
    }
}


// 画敌人移动道路
void GameWidget::drawPath(QPainter& painter)
{
    for (int pathIndex = 0; pathIndex < m_paths.size(); ++pathIndex) {
        const QVector<QPointF>& pathPoints = m_paths[pathIndex];
    // 如果路径点少于 2 个，就不能形成道路
    if (pathPoints.size() < 2) {
        return;
    }

    QPainterPath path;                 // 创建路径对象

    path.moveTo(pathPoints[0]);            // 路径从第一个点开始

    // 依次连接后面的路径点
    for (int i = 1; i < pathPoints.size(); ++i) {
        path.lineTo(pathPoints[i]);
    }
    painter.setBrush(Qt::NoBrush);
    // 先画道路外边框，颜色深一点，线条粗一点
    painter.setPen(QPen(QColor(130, 105, 70),
                        42,
                        Qt::SolidLine,
                        Qt::RoundCap,
                        Qt::RoundJoin));
    painter.drawPath(path);
    painter.setBrush(Qt::NoBrush);
    // 再画道路主体，颜色浅一点，线条稍微细一点
    painter.setPen(QPen(QColor(210, 180, 120),
                        34,
                        Qt::SolidLine,
                        Qt::RoundCap,
                        Qt::RoundJoin));
    painter.drawPath(path);

    // 画起点 S
    painter.setPen(Qt::NoPen);                 // 不画边框
    painter.setBrush(QColor(80, 170, 90));     // 绿色填充
    painter.drawEllipse(pathPoints.first(), 18, 18);

    painter.setPen(Qt::white);                 // 白色文字
    painter.drawText(QRectF(pathPoints.first().x() - 20,
                            pathPoints.first().y() - 10,
                            40,
                            20),
                     Qt::AlignCenter,
                     "S");

    // 画终点 E
    painter.setPen(Qt::NoPen);

    // 显示 S1 / S2，表示不同起点
    QString startText = QString("S%1").arg(pathIndex + 1);
    painter.setPen(Qt::white);
    painter.drawText(QRectF(pathPoints.last().x() - 20,
                            pathPoints.last().y() - 10,
                            40,
                            20),
                     Qt::AlignCenter,
                     startText);
}
    // 所有路径共用终点
    if (!m_paths.isEmpty() && !m_paths.first().isEmpty()) {
        QPointF endPoint = m_paths.first().last();

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(200, 80, 80));
        painter.drawEllipse(endPoint, 18, 18);

        painter.setPen(Qt::white);
        painter.drawText(QRectF(endPoint.x() - 20,
                                endPoint.y() - 10,
                                40,
                                20),
                         Qt::AlignCenter,
                         "E");
    }
}

// 画顶部状态栏
void GameWidget::drawHud(QPainter& painter)
{
    // 半透明圆角背景，不再整条盖住地图
    QRectF hudRect(20, 12, width() - 40, 48);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(35, 65, 38, 190));
    painter.drawRoundedRect(hudRect, 18, 18);

    // 细边框
    painter.setPen(QPen(QColor(210, 230, 160, 180), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(hudRect, 18, 18);

    // 文字样式
    QFont font = painter.font();
    font.setPointSize(12);
    font.setBold(true);
    painter.setFont(font);

    painter.setPen(QColor(255, 250, 220));

    QString info = QString("金币：%1    生命：%2    波次：%3/%4    敌人：%5/%6    当前塔：%7    操作：1普通塔  2速射塔  3重炮塔  U升级  X出售")
                       .arg(m_gold)
                       .arg(m_life)
                       .arg(m_wave)
                       .arg(m_waves.size())
                       .arg(m_spawnedCount)
                       .arg(m_totalEnemies)
                       .arg(Tower::nameForType(m_selectedTowerType));

    painter.drawText(QRectF(40, 12, width() - 80, 48),
                     Qt::AlignVCenter | Qt::AlignLeft,
                     info);
}
// 把鼠标点击位置吸附到网格中心
QPointF GameWidget::snapToGrid(const QPointF& pos) const
{
    int gridSize = 60;     // 网格大小

    // 算出当前点击位置属于第几个格子
    int gridX = static_cast<int>(pos.x()) / gridSize;
    int gridY = static_cast<int>(pos.y()) / gridSize;

    // 返回这个格子的中心点
    return QPointF(gridX * gridSize + gridSize / 2,
                   gridY * gridSize + gridSize / 2);
}


// 计算点 p 到线段 ab 的距离
// 这个函数用来判断塔是不是太靠近道路
static double distanceToSegment(const QPointF& p,
                                const QPointF& a,
                                const QPointF& b)
{
    double vx = b.x() - a.x();      // 线段 ab 的 x 方向
    double vy = b.y() - a.y();      // 线段 ab 的 y 方向

    double wx = p.x() - a.x();      // 点 p 到点 a 的 x 方向
    double wy = p.y() - a.y();      // 点 p 到点 a 的 y 方向

    double len2 = vx * vx + vy * vy;    // 线段长度的平方

    if (len2 == 0) {
        // 如果 a 和 b 是同一个点，就直接计算 p 到 a 的距离
        double dx = p.x() - a.x();
        double dy = p.y() - a.y();
        return qSqrt(dx * dx + dy * dy);
    }

    // t 表示 p 投影在线段 ab 上的位置比例
    double t = (wx * vx + wy * vy) / len2;

    // 限制 t 在 0 到 1 之间，保证投影点在线段上
    if (t < 0) {
        t = 0;
    }
    else if (t > 1) {
        t = 1;
    }

    // 计算投影点坐标
    QPointF projection(a.x() + t * vx,
                       a.y() + t * vy);

    // 返回 p 到投影点的距离
    double dx = p.x() - projection.x();
    double dy = p.y() - projection.y();

    return qSqrt(dx * dx + dy * dy);
}


// 判断某个位置能不能建塔
bool GameWidget::canBuildTowerAt(const QPointF& pos) const
{
    // 不能建在顶部状态栏
    if (pos.y() < 80) {
        return false;
    }

    // 不能建在道路附近
    // 道路主体宽度大约 34，外边框 42
    // 这里用 45 做安全距离
    // 不能建在任何一条道路附近
        for (const QVector<QPointF>& path : m_paths) {
        for (int i = 0; i < path.size() - 1; ++i) {
            double d = distanceToSegment(pos, path[i], path[i + 1]);

            if (d < 45) {
                return false;
            }
        }
    }

    // 不能和已有防御塔太近
    for (Tower* tower : m_towers) {
        double dx = pos.x() - tower->position().x();
        double dy = pos.y() - tower->position().y();
        double distance = qSqrt(dx * dx + dy * dy);

        if (distance < 42) {
            return false;
        }
    }

    return true;
}


// 鼠标点击事件
// 玩家点击地图时，会自动调用这个函数
void GameWidget::mousePressEvent(QMouseEvent *event)
{
    setFocus();

    if (event->button() != Qt::LeftButton) {
        return;
    }

    QPointF clickPos = event->position();

    // 游戏结束后，只处理结果界面的两个按钮
    if (m_gameFinished) {
        QPoint clickPoint = clickPos.toPoint();

        if (resultRetryRect().contains(clickPoint)) {
            restartGame();
            return;
        }

        if (resultMenuRect().contains(clickPoint)) {
            emit backToMenuRequested();
            return;
        }

        return;
    }
    // 如果点击的是已有防御塔，就选中它，不建新塔
    Tower* clickedTower = towerAt(clickPos);

    if (clickedTower != nullptr) {
        clearSelectedTower();

        m_selectedTower = clickedTower;
        m_selectedTower->setSelected(true);

        update();
        return;
    }

    // 如果点的不是塔，就取消当前选中
    clearSelectedTower();
    // 把点击位置吸附到网格中心
    QPointF buildPos = snapToGrid(clickPos);

    int towerCost = Tower::costForType(m_selectedTowerType);

    // 金币不足，不能建塔
    if (m_gold < towerCost) {
        showStatusMessage("金币不足，无法建造防御塔！", 60);
        return;
    }

    // 位置不合法，不能建塔
    if (!canBuildTowerAt(buildPos)) {
        showStatusMessage("这里不能建造防御塔！", 60);
        return;
    }

    // 创建防御塔
    Tower* tower = new Tower(buildPos, m_selectedTowerType);

    // 加入防御塔数组
    m_towers.append(tower);

    // 扣金币
    m_gold -= towerCost;

    // 刷新画面
    update();
}
void GameWidget::checkGameResult()
{
    if (m_gameFinished) {
        return;
    }

    // 失败条件：生命值小于等于 0
    if (m_life <= 0) {
        m_gameFinished = true;
        m_gameWon = false;

        if (m_timer) {
            m_timer->stop();
        }

        update();
        return;
    }

    // 胜利条件：所有敌人生成完，并且地图上没有敌人
    if (m_spawnedCount >= m_totalEnemies && m_enemies.isEmpty()) {
        m_gameFinished = true;
        m_gameWon = true;

        if (m_timer) {
            m_timer->stop();
        }

        update();
        return;
    }
}
void GameWidget::removeBulletsTargeting(Enemy* enemy)
{
    for (int i = m_bullets.size() - 1; i >= 0; --i) {
        if (m_bullets[i]->target() == enemy) {
            delete m_bullets[i];
            m_bullets.removeAt(i);
        }
    }
}
void GameWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_1) {
        m_selectedTowerType = TowerType::Normal;
        update();
    }
    else if (event->key() == Qt::Key_2) {
        m_selectedTowerType = TowerType::Fast;
        update();
    }
    else if (event->key() == Qt::Key_3) {
        m_selectedTowerType = TowerType::Heavy;
        update();
    }
    else if (event->key() == Qt::Key_U) {
        upgradeSelectedTower();
    }
    else if (event->key() == Qt::Key_X || event->key() == Qt::Key_Delete) {
        sellSelectedTower();
    }
}
Tower* GameWidget::towerAt(const QPointF& pos) const
{
    for (Tower* tower : m_towers) {
        if (tower->containsPoint(pos)) {
            return tower;
        }
    }

    return nullptr;
}

void GameWidget::clearSelectedTower()
{
    if (m_selectedTower != nullptr) {
        m_selectedTower->setSelected(false);
        m_selectedTower = nullptr;
    }
}

void GameWidget::upgradeSelectedTower()
{
    if (m_selectedTower == nullptr) {
        showStatusMessage("请先点击选择一座防御塔。", 60);
        return;
    }

    if (!m_selectedTower->canUpgrade()) {
        showStatusMessage("这座防御塔已经满级。", 60);
        return;
    }

    int cost = m_selectedTower->upgradeCost();

    if (m_gold < cost) {
        showStatusMessage("金币不足，无法升级。", 60);
        return;
    }

    m_gold -= cost;
    m_selectedTower->upgrade();

    update();
}

void GameWidget::sellSelectedTower()
{
    if (m_selectedTower == nullptr) {
        showStatusMessage("请先点击选择一座防御塔。", 60);
        return;
    }

    m_gold += m_selectedTower->sellValue();

    for (int i = m_towers.size() - 1; i >= 0; --i) {
        if (m_towers[i] == m_selectedTower) {
            delete m_towers[i];
            m_towers.removeAt(i);
            break;
        }
    }

    m_selectedTower = nullptr;

    update();
}
void GameWidget::applyBossAuraToAllEnemies()
{
    for (Enemy* enemy : m_enemies) {
        if (!enemy->isDead() && !enemy->hasReachedEnd()) {
            enemy->applyBossAura();
        }
    }
}

void GameWidget::removeBossAuraFromAllEnemies()
{
    for (Enemy* enemy : m_enemies) {
        if (enemy && !enemy->isDead() && !enemy->hasReachedEnd() && !enemy->isBoss()) {
            enemy->removeBossAura();
        }
    }
}

void GameWidget::showStatusMessage(const QString& text, int frames)
{
    m_statusMessage = text;
    m_statusMessageCounter = frames;
    update();
}

void GameWidget::drawStatusMessage(QPainter& painter)
{
    if (m_statusMessageCounter <= 0) {
        return;
    }

    QRectF box(width() / 2 - 330, 80, 660, 56);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30, 30, 30, 180));
    painter.drawRoundedRect(box, 12, 12);

    painter.setPen(QColor(255, 230, 120));

    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(18);
    painter.setFont(font);

    painter.drawText(box, Qt::AlignCenter, m_statusMessage);
}
void GameWidget::drawGameResultScreen(QPainter& painter)
{
    QPixmap resultImage = m_gameWon ? m_winImage : m_loseImage;

    if (!resultImage.isNull()) {
        // 直接把整张胜利/失败图铺满 1200×800
        painter.drawPixmap(rect(), resultImage);
        return;
    }

    // 如果图片加载失败，显示备用文字
    painter.fillRect(rect(), QColor(40, 80, 45));

    painter.setPen(Qt::white);

    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(36);
    painter.setFont(font);

    QString text = m_gameWon ? "游戏胜利！" : "游戏失败！";
    painter.drawText(rect(), Qt::AlignCenter, text);
}

QRect GameWidget::resultRetryRect() const
{
    if (m_gameWon) {
        // 胜利图里的“重新挑战”按钮
        return QRect(280, 465, 300, 95);
    }

    // 失败图里的“重新挑战”按钮
    return QRect(280, 565, 300, 95);
}

QRect GameWidget::resultMenuRect() const
{
    if (m_gameWon) {
        // 胜利图里的“返回主页”按钮
        return QRect(600, 465, 300, 95);
    }

    // 失败图里的“返回主页”按钮
    return QRect(600, 565, 300, 95);
}

void GameWidget::restartGame(bool startTimer)
{
    if (m_timer) {
        m_timer->stop();
    }

    qDeleteAll(m_enemies);
    m_enemies.clear();

    qDeleteAll(m_towers);
    m_towers.clear();

    qDeleteAll(m_bullets);
    m_bullets.clear();

    m_selectedTower = nullptr;

    m_gold = 150;
    m_life = 10;
    m_wave = 1;

    m_spawnCounter = 0;
    m_spawnedCount = 0;
    m_totalEnemies = 0;

    m_gameFinished = false;
    m_gameWon = false;

    m_bossAuraActive = false;
    m_bossAuraTriggered = false;

    m_statusMessage.clear();
    m_statusMessageCounter = 0;

    m_currentWaveIndex = 0;
    m_spawnIndexInWave = 0;
    m_nextPathIndex = 0;

    m_waveWaitCounter = 0;

    m_burstRemaining = 0;
    m_burstGapCounter = 0;

    m_selectedTowerType = TowerType::Normal;

    initPaths();
    initSpawnQueue();

    if (startTimer && m_timer) {
        m_timer->start(30);
    }

    setFocus();
    update();
}
void GameWidget::drawMapMarkers(QPainter& painter)
{
    if (m_paths.isEmpty()) {
        return;
    }

    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(14);
    painter.setFont(font);

    // 画两个起点
    for (int i = 0; i < m_paths.size(); ++i) {
        if (m_paths[i].isEmpty()) {
            continue;
        }

        QPointF startPoint = m_paths[i].first();

        painter.setPen(QPen(QColor(40, 90, 45), 3));
        painter.setBrush(QColor(90, 190, 95, 230));
        painter.drawEllipse(startPoint, 22, 22);

        painter.setPen(Qt::white);
        QString text = QString("S%1").arg(i + 1);

        painter.drawText(QRectF(startPoint.x() - 24,
                                startPoint.y() - 12,
                                48,
                                24),
                         Qt::AlignCenter,
                         text);
    }

    // 画终点
    QPointF endPoint = m_paths.first().last();

    painter.setPen(QPen(QColor(120, 45, 45), 3));
    painter.setBrush(QColor(210, 80, 85, 230));
    painter.drawEllipse(endPoint, 22, 22);

    painter.setPen(Qt::white);
    painter.drawText(QRectF(endPoint.x() - 24,
                            endPoint.y() - 12,
                            48,
                            24),
                     Qt::AlignCenter,
                     "E");
}
