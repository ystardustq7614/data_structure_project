#include "gamewidget.h"      // 引入 GameWidget 的头文件
#include <QPainter>          // QPainter 用来绘图
#include <QPen>              // QPen 用来设置线条颜色和粗细
#include <QBrush>            // QBrush 用来设置填充颜色
#include <QPainterPath>      // QPainterPath 用来绘制连续路径
#include <QColor>            // QColor 用来设置颜色
#include <QTimer>
#include <QtMath>
#include <cmath>
// GameWidget 构造函数
// 当 GameWidget 被创建时，会自动执行这里
GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)       { // 调用 QWidget 的构造函数，因为 GameWidget 继承 QWidget
    setFixedSize(1200, 800);  // 设置游戏窗口大小
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    m_gameMapBackground = QPixmap(":/images/images/game_map.jpg");
    m_winImage = QPixmap(":/images/images/win.png");
    m_loseImage = QPixmap(":/images/images/lose.png");
    initPaths();              // 初始化敌人移动路径
    m_state = m_scheduler.reset(QString(), &m_configurationWarning,
                                &m_configurationError)
                  ? GameState::Running : GameState::Stopped;
    m_timer = new QTimer(this);  // 创建定时器，this 表示这个定时器属于 GameWidget
    // connect 是 Qt 的信号槽机制
    // 意思是：每当 m_timer 触发 timeout 信号，就调用 updateGame 函数
    connect(m_timer, &QTimer::timeout, this, &GameWidget::updateGame);
    if (m_state == GameState::Running) {
        m_timer->start(30);      // 每 30 毫秒触发一次
    }
    if (!m_configurationWarning.isEmpty()) {
        showStatusMessage(m_configurationWarning, 180);
    }
}


// 析构函数
// 当 GameWidget 被销毁时，会执行这里
GameWidget::~GameWidget()
{
    qDeleteAll(m_bullets);
    m_bullets.clear();
    qDeleteAll(m_enemies);   // 删除 m_enemies 里所有 new 出来的 Enemy 对象
    m_enemies.clear();       // 清空敌人数组
    qDeleteAll(m_towers);      // 删除所有防御塔
    m_towers.clear();
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
// 生成敌人
void GameWidget::spawnEnemy(const QString& type, int waveIndex)
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
    if (waveIndex >= 3) {
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
    if (m_state != GameState::Running) {
        return;
    }
    // 所有出怪、增援与切波事件由稳定最小堆按逻辑 tick 执行。
    // 传入的是敌人移动和清理之前的数量，保持旧版清场门槛的观察相位。
    const QVector<AttackScheduler::Event> events = m_scheduler.advance(m_enemies.size());
    if (!m_scheduler.isReady()) {
        m_state = GameState::Stopped;
        m_timer->stop();
        showStatusMessage("进攻计时超出可用范围，请退出并重新开始。", 600);
        return;
    }
    for (const AttackScheduler::Event& event : events) {
        if (event.kind == AttackScheduler::Kind::WaveStart) {
            if (event.waveIndex == 3) {
                showStatusMessage("警报：第4波开始，敌人获得护甲强化！", 150);
            }
            if (event.waveIndex == 5) {
                showStatusMessage("最终波开始！Boss 即将出现！", 150);
                emit finalWaveStarted();
            }
        } else {
            spawnEnemy(event.enemyType, event.waveIndex);
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

            removeBulletsTargeting(enemy);  // 删除所有瞄准这个敌人的子弹
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

    if (m_gameFinished) {
        drawGameResultScreen(painter);
        return;
    }

    drawBuildPreview(painter);
    if (m_state == GameState::Paused) {
        drawPauseOverlay(painter);
    }
    // 控件位于暂停遮罩之上，暂停时仍可继续或退出。
    drawHud(painter);
    drawEventPreview(painter);
    drawStatusMessage(painter);

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

    QString info = QString("金币：%1   生命：%2   波次：%3/6   敌人：%4/%5   当前塔：%6   1/2/3选塔 U升级 X出售")
                       .arg(m_gold)
                       .arg(m_life)
                       .arg(m_scheduler.currentWaveIndex() + 1)
                       .arg(m_scheduler.spawnedCount())
                       .arg(m_scheduler.totalEnemies())
                       .arg(Tower::nameForType(m_selectedTowerType));

    painter.drawText(QRectF(40, 12, 810, 48),
                     Qt::AlignVCenter | Qt::AlignLeft,
                     info);
    for (Control control : {Control::Preview, Control::Pause, Control::Exit}) {
        const QRect button = controlRect(control);
        painter.setPen(QPen(QColor(210, 230, 160, 170), 1));
        painter.setBrush(QColor(27, 52, 31, 225));
        painter.drawRoundedRect(button, 11, 11);
        drawButtonFeedback(painter, button, m_hoveredControl == control,
                           m_pressedControl == control,
                           control == Control::Pause && m_state == GameState::Paused);
        painter.setPen(Qt::white);
        painter.drawText(button, Qt::AlignCenter,
                         control == Control::Preview ? "预览"
                         : control == Control::Pause
                               ? (m_state == GameState::Paused ? "继续" : "暂停")
                               : "退出");
    }
}
// 把鼠标点击位置吸附到网格中心
QPointF GameWidget::snapToGrid(const QPointF& pos) const
{
    int gridSize = 60;     // 网格大小

    // 算出当前点击位置属于第几个格子
    int gridX = qBound(0, static_cast<int>(pos.x()) / gridSize,
                       width() / gridSize - 1);
    int gridY = qBound(1, static_cast<int>(pos.y()) / gridSize,
                       height() / gridSize - 1);

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


// 预览与实际建造共用这条判定路径。
GameWidget::BuildEvaluation GameWidget::evaluateBuild(const QPointF& rawPosition) const
{
    BuildEvaluation result;
    if (m_state != GameState::Running || !std::isfinite(rawPosition.x())
        || !std::isfinite(rawPosition.y()) || rawPosition.x() < 0
        || rawPosition.x() >= width() || rawPosition.y() < 60
        || rawPosition.y() >= height()
        || controlAt(rawPosition.toPoint()) != Control::None
        || (m_previewVisible && previewPanelRect().contains(rawPosition.toPoint()))) {
        return result;
    }
    result.center = snapToGrid(rawPosition);
    const int radius = Tower::radiusForType(m_selectedTowerType);
    if (result.center.x() - radius < 0 || result.center.x() + radius >= width()
        || result.center.y() - radius < 60
        || result.center.y() + radius >= height()) {
        return result;
    }
    for (const QVector<QPointF>& path : m_paths) {
        for (int i = 0; i < path.size() - 1; ++i) {
            if (distanceToSegment(result.center, path[i], path[i + 1]) < 45) {
                result.problem = BuildProblem::Road;
                return result;
            }
        }
    }
    for (Tower* tower : m_towers) {
        double dx = result.center.x() - tower->position().x();
        double dy = result.center.y() - tower->position().y();
        double distance = qSqrt(dx * dx + dy * dy);
        if (distance < 42) {
            result.problem = BuildProblem::Occupied;
            return result;
        }
    }
    result.problem = m_gold < Tower::costForType(m_selectedTowerType)
                         ? BuildProblem::InsufficientGold : BuildProblem::None;
    return result;
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
    m_mousePosition = clickPos;
    m_hasMousePosition = true;
    m_hoveredControl = controlAt(clickPos.toPoint());
    if (m_hoveredControl != Control::None) {
        m_pressedControl = m_hoveredControl;
        update();
        return;
    }
    if (m_state != GameState::Running || clickPos.y() < 60
        || (m_previewVisible && previewPanelRect().contains(clickPos.toPoint()))) {
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
    const BuildEvaluation build = evaluateBuild(clickPos);
    if (build.problem == BuildProblem::InsufficientGold) {
        showStatusMessage("金币不足，无法建造防御塔！", 60);
        return;
    }
    if (!build.valid()) {
        showStatusMessage("这里不能建造防御塔！", 60);
        return;
    }
    Tower* tower = new Tower(build.center, m_selectedTowerType);
    m_towers.append(tower);
    m_gold -= Tower::costForType(m_selectedTowerType);
    update();
}

void GameWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    const Control released = controlAt(event->position().toPoint());
    const Control pressed = m_pressedControl;
    m_pressedControl = Control::None;
    m_hoveredControl = released;
    update();
    if (released != pressed || released == Control::None) return;

    switch (released) {
    case Control::Preview:
        if (m_state != GameState::Finished) m_previewVisible = !m_previewVisible;
        update();
        return;
    case Control::Pause:
        setPaused(m_state == GameState::Running);
        return;
    case Control::Exit:
    case Control::Menu:
        emit backToMenuRequested();
        return;
    case Control::Retry:
        restartGame();
        return;
    case Control::None:
        return;
    }
}

void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_mousePosition = event->position();
    m_hasMousePosition = true;
    m_hoveredControl = controlAt(m_mousePosition.toPoint());
    if (m_pressedControl != Control::None && m_pressedControl != m_hoveredControl) {
        m_pressedControl = Control::None;
    }
    setCursor(m_hoveredControl == Control::None ? Qt::ArrowCursor : Qt::PointingHandCursor);
    update();
}

void GameWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    resetInteraction();
    update();
}

void GameWidget::focusOutEvent(QFocusEvent *event)
{
    m_pressedControl = Control::None;
    m_hoveredControl = Control::None;
    m_hasMousePosition = false;
    update();
    QWidget::focusOutEvent(event);
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
        m_state = GameState::Finished;

        if (m_timer) {
            m_timer->stop();
        }

        update();
        return;
    }

    // 胜利条件：所有敌人生成完，并且地图上没有敌人
    if (!m_scheduler.hasPendingGeneration()
        && m_scheduler.spawnedCount() == m_scheduler.totalEnemies()
        && m_enemies.isEmpty()) {
        m_gameFinished = true;
        m_gameWon = true;
        m_state = GameState::Finished;

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
    if ((event->key() == Qt::Key_P || event->key() == Qt::Key_Escape)
        && !event->isAutoRepeat() && m_state != GameState::Finished) {
        setPaused(m_state == GameState::Running);
        return;
    }
    if (m_state != GameState::Running) {
        return;
    }
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
        if (!enemy->isBoss() && !enemy->isDead() && !enemy->hasReachedEnd()) {
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

    QRectF box(190, 80, 630, m_statusMessage.size() > 32 ? 76 : 56);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30, 30, 30, 180));
    painter.drawRoundedRect(box, 12, 12);

    painter.setPen(QColor(255, 230, 120));

    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(m_statusMessage.size() > 32 ? 12 : 18);
    painter.setFont(font);

    painter.drawText(box.adjusted(12, 4, -12, -4),
                     Qt::AlignCenter | Qt::TextWordWrap, m_statusMessage);
}
void GameWidget::drawGameResultScreen(QPainter& painter)
{
    QPixmap resultImage = m_gameWon ? m_winImage : m_loseImage;

    if (!resultImage.isNull()) {
        // 直接把整张胜利/失败图铺满 1200×800
        painter.drawPixmap(rect(), resultImage);
    } else {
        painter.fillRect(rect(), QColor(40, 80, 45));
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(36);
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter,
                         m_gameWon ? "游戏胜利！" : "游戏失败！");
    }
    for (Control control : {Control::Retry, Control::Menu}) {
        drawButtonFeedback(painter, controlRect(control),
                           m_hoveredControl == control,
                           m_pressedControl == control);
    }
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
    const bool wasPaused = m_state == GameState::Paused;
    m_state = GameState::Stopped;

    qDeleteAll(m_bullets);
    m_bullets.clear();
    qDeleteAll(m_enemies);
    m_enemies.clear();

    qDeleteAll(m_towers);
    m_towers.clear();


    m_selectedTower = nullptr;

    m_gold = 150;
    m_life = 10;

    m_gameFinished = false;
    m_gameWon = false;

    m_bossAuraActive = false;
    m_bossAuraTriggered = false;

    m_statusMessage.clear();
    m_statusMessageCounter = 0;

    m_nextPathIndex = 0;
    m_selectedTowerType = TowerType::Normal;
    resetInteraction();
    m_previewVisible = false;

    initPaths();
    const bool configured = m_scheduler.reset(QString(), &m_configurationWarning,
                                               &m_configurationError);
    if (wasPaused) emit pauseChanged(false);
    if (!configured) {
        showStatusMessage("进攻配置错误：" + m_configurationError, 600);
    } else if (startTimer && !m_configurationWarning.isEmpty()) {
        showStatusMessage(m_configurationWarning, 180);
    }

    if (startTimer && configured && m_timer) {
        m_state = GameState::Running;
        m_timer->start(30);
    }

    setFocus();
    update();
}

QRect GameWidget::controlRect(Control control) const
{
    switch (control) {
    case Control::Preview: return QRect(870, 20, 92, 32);
    case Control::Pause: return QRect(970, 20, 92, 32);
    case Control::Exit: return QRect(1070, 20, 92, 32);
    case Control::Retry: return resultRetryRect();
    case Control::Menu: return resultMenuRect();
    case Control::None: return QRect();
    }
    return QRect();
}

QRect GameWidget::previewPanelRect() const
{
    return QRect(840, 70, 330, 145);
}

GameWidget::Control GameWidget::controlAt(const QPoint& pos) const
{
    if (m_gameFinished) {
        if (resultRetryRect().contains(pos)) return Control::Retry;
        if (resultMenuRect().contains(pos)) return Control::Menu;
        return Control::None;
    }
    if (m_state == GameState::Stopped) {
        return controlRect(Control::Exit).contains(pos) ? Control::Exit : Control::None;
    }
    for (Control control : {Control::Preview, Control::Pause, Control::Exit}) {
        if (controlRect(control).contains(pos)) return control;
    }
    return Control::None;
}

void GameWidget::setPaused(bool paused)
{
    if (paused && m_state == GameState::Running) {
        m_state = GameState::Paused;
        if (m_timer) m_timer->stop();
        emit pauseChanged(true);
    } else if (!paused && m_state == GameState::Paused) {
        m_state = GameState::Running;
        if (m_timer) m_timer->start(30);
        emit pauseChanged(false);
    }
    update();
}

void GameWidget::resetInteraction()
{
    m_hoveredControl = Control::None;
    m_pressedControl = Control::None;
    m_hasMousePosition = false;
    setCursor(Qt::ArrowCursor);
}

void GameWidget::drawButtonFeedback(QPainter& painter, const QRect& button,
                                    bool hovered, bool pressed, bool selected) const
{
    if (!hovered && !pressed && !selected) return;
    painter.setPen(QPen(pressed ? QColor(255, 218, 100) : QColor(245, 225, 155),
                        pressed ? 3 : 2));
    painter.setBrush(pressed ? QColor(0, 0, 0, 78)
                             : selected ? QColor(245, 204, 75, 65)
                             : QColor(255, 255, 255, 32));
    painter.drawRoundedRect(button.adjusted(2, 2, -2, -2), 13, 13);
}

void GameWidget::drawBuildPreview(QPainter& painter)
{
    if (!m_hasMousePosition || m_state != GameState::Running
        || towerAt(m_mousePosition) != nullptr) return;
    const BuildEvaluation candidate = evaluateBuild(m_mousePosition);
    if (candidate.problem == BuildProblem::OutsideMap) return;
    const bool valid = candidate.valid();
    const int radius = Tower::radiusForType(m_selectedTowerType);
    painter.setPen(QPen(valid ? QColor(80, 255, 145, 220)
                              : QColor(255, 95, 90, 230), 3));
    painter.setBrush(valid ? QColor(60, 225, 120, 100)
                           : QColor(230, 65, 65, 105));
    painter.drawEllipse(candidate.center, radius, radius);
}

void GameWidget::drawPauseOverlay(QPainter& painter)
{
    painter.fillRect(rect(), QColor(0, 0, 0, 125));
    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(38);
    painter.setFont(font);
    painter.setPen(QColor(255, 239, 175));
    painter.drawText(QRect(0, 300, width(), 90), Qt::AlignCenter, "已暂停");
    font.setPointSize(16);
    painter.setFont(font);
    painter.drawText(QRect(0, 390, width(), 60), Qt::AlignCenter,
                     "按 P / Esc 或点击继续；退出将放弃本局");
}

void GameWidget::drawEventPreview(QPainter& painter)
{
    if (!m_previewVisible) return;
    const QRect panel = previewPanelRect();
    painter.setPen(QPen(QColor(230, 215, 150), 2));
    painter.setBrush(QColor(24, 45, 30, 235));
    painter.drawRoundedRect(panel, 12, 12);
    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(12);
    painter.setFont(font);
    painter.setPen(QColor(255, 240, 185));
    painter.drawText(QRect(panel.x() + 14, panel.y() + 10, 300, 25),
                     Qt::AlignLeft | Qt::AlignVCenter, "进攻事件预览");
    font.setBold(false);
    font.setPointSize(11);
    painter.setFont(font);
    const QVector<AttackScheduler::Event> next = m_scheduler.preview(3);
    if (next.isEmpty()) {
        const QString text = m_scheduler.waitingForClear()
                                 ? QString("清场至 %1 名敌人后进入下一波")
                                       .arg(m_scheduler.clearThreshold())
                                 : "当前没有待执行事件";
        painter.drawText(QRect(panel.x() + 14, panel.y() + 48, 300, 25), text);
        return;
    }
    for (int i = 0; i < next.size(); ++i) {
        const auto& event = next.at(i);
        const qint64 remaining = qMax(qint64(0),
                                      event.dueTick - m_scheduler.currentTick());
        const QString enemyName = event.enemyType == "bug" ? "小怪"
                                  : event.enemyType == "ddl" ? "DDL"
                                  : event.enemyType == "virus" ? "变异体"
                                  : event.enemyType == "boss" ? "Boss"
                                  : event.enemyType;
        const QString label = event.kind == AttackScheduler::Kind::WaveStart
                                  ? QString("第%1波开始").arg(event.waveIndex + 1)
                                  : QString("第%1波 %2%3")
                                        .arg(event.waveIndex + 1)
                                        .arg(event.kind == AttackScheduler::Kind::Reinforcement
                                                 ? "增援 " : "出怪 ")
                                        .arg(enemyName);
        painter.drawText(QRect(panel.x() + 14, panel.y() + 43 + i * 29, 300, 25),
                         QString("%1  ·  %2 秒")
                             .arg(label)
                             .arg(QString::number(remaining * 0.03, 'f', 1)));
    }
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
