/** @file
 * @brief 事件到战局命令的适配；输入与绘制共享布局，计时器仅驱动 Running 战局。 */
#include "gamewidget.h"
#include "core/attackplanloader.h"
#include <QCoreApplication>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

GameWidget::GameWidget(QWidget* parent) : QWidget(parent), m_timer(new QTimer(this))
{
    setFixedSize(GameMap::Width, GameMap::Height);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    connect(m_timer, &QTimer::timeout, this, &GameWidget::updateGame);
    restartGame(true);
}

void GameWidget::restartGame(bool startTimer)
{
    // 先停旧时钟再清状态和读取配置，避免重置期间残留 tick 使用上一局的实体或计划。
    m_timer->stop();
    const bool wasPaused = m_session.isPaused();
    m_ui = GameUiState{};
    setCursor(Qt::ArrowCursor);
    // 环境路径只在适配层组装；加载器和战局不会隐式读取可执行文件目录。
    const auto loaded = AttackPlanLoader::load(
        QStringLiteral(":/config/attack_waves.json"),
        QCoreApplication::applicationDirPath() + "/attack_rhythm.json");
    if (loaded.plan) m_session.reset(*loaded.plan, startTimer);
    else m_session.clear();
    if (wasPaused) emit pauseChanged(false);
    if (!loaded.plan) m_ui.showStatus("进攻配置错误：" + loaded.error, 600);
    else if (startTimer && !loaded.warning.isEmpty()) m_ui.showStatus(loaded.warning, 180);
    syncTimer();
    setFocus();
    update();
}

void GameWidget::syncTimer()
{
    if (m_session.state() == GameSession::State::Running) {
        if (!m_timer->isActive()) m_timer->start(30);
    } else m_timer->stop();
}

void GameWidget::updateGame()
{
    const auto frame = m_session.step();
    for (const auto& notice : frame.notices) m_ui.showStatus(notice.text, notice.frames);
    if (frame.finalWaveStarted) emit finalWaveStarted();
    if (frame.advanced) m_ui.advanceSubtitle();
    syncTimer();
    if (frame.advanced || frame.stopped) update();
}

void GameWidget::paintEvent(QPaintEvent*)
{
    // Qt 可在曝光、切页等时机请求重绘，不能把模拟推进放在 paintEvent 内。
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    m_renderer.paint(painter, m_session, m_ui);
}

GameWidget::BuildEvaluation GameWidget::evaluateBuild(const QPointF& pos) const
{
    return m_renderer.evaluateBuild(pos, m_session, m_ui);
}

void GameWidget::applyAction(const GameSession::ActionResult& result)
{
    if (!result.message.isEmpty()) m_ui.showStatus(result.message, result.frames);
    update();
}

void GameWidget::mousePressEvent(QMouseEvent* event)
{
    setFocus();
    if (event->button() != Qt::LeftButton) return;
    const QPointF pos = event->position();
    m_ui.mousePosition = pos;
    m_ui.hasMousePosition = true;
    m_ui.hoveredControl = m_renderer.controlAt(pos.toPoint(), m_session);
    if (m_ui.hoveredControl != Control::None) {
        // 控件点击等待同按钮释放才执行，不能在按下时触发切页或把点击穿透成建造。
        m_ui.pressedControl = m_ui.hoveredControl;
        update();
        return;
    }
    if (m_session.state() != GameSession::State::Running
        || m_renderer.blocksMapInput(pos, m_session, m_ui)) return;
    if (m_session.selectTowerAt(pos)) { update(); return; }
    m_session.clearSelectedTower();
    applyAction(m_session.build(pos, m_ui.selectedTowerType));
}

void GameWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    const Control released = m_renderer.controlAt(event->position().toPoint(), m_session);
    const Control pressed = m_ui.pressedControl;
    m_ui.pressedControl = Control::None;
    m_ui.hoveredControl = released;
    update();
    // 按下后移出、失焦或在另一个按钮松开都应取消，防止拖动触发退出/暂停。
    if (released == Control::None || released != pressed) return;
    switch (released) {
    case Control::Preview:
        if (!m_session.isFinished()) m_ui.previewVisible = !m_ui.previewVisible;
        update();
        return;
    case Control::Pause: setPaused(m_session.state() == GameSession::State::Running); return;
    case Control::Exit:
    case Control::Menu: emit backToMenuRequested(); return;
    case Control::Retry: restartGame(); return;
    case Control::None: return;
    }
}

void GameWidget::mouseMoveEvent(QMouseEvent* event)
{
    m_ui.mousePosition = event->position();
    m_ui.hasMousePosition = true;
    m_ui.hoveredControl = m_renderer.controlAt(m_ui.mousePosition.toPoint(), m_session);
    if (m_ui.pressedControl != Control::None && m_ui.pressedControl != m_ui.hoveredControl)
        m_ui.pressedControl = Control::None;
    setCursor(m_ui.hoveredControl == Control::None ? Qt::ArrowCursor : Qt::PointingHandCursor);
    update();
}

void GameWidget::leaveEvent(QEvent*)
{
    m_ui.resetInteraction();
    setCursor(Qt::ArrowCursor);
    update();
}

void GameWidget::focusOutEvent(QFocusEvent* event)
{
    // 丢失焦点只取消交互，不自动暂停；战局暂停必须经 setPaused 同步时钟与音频。
    m_ui.resetInteraction();
    update();
    QWidget::focusOutEvent(event);
}

void GameWidget::setPaused(bool paused)
{
    if (m_session.setPaused(paused)) {
        syncTimer();
        emit pauseChanged(paused);
    }
    update();
}

void GameWidget::keyPressEvent(QKeyEvent* event)
{
    // 长按产生的重复按键不能反复切换暂停；其他交易热键仍遵循各自原有按键行为。
    if ((event->key() == Qt::Key_P || event->key() == Qt::Key_Escape)
        && !event->isAutoRepeat() && !m_session.isFinished()) {
        setPaused(m_session.state() == GameSession::State::Running);
        return;
    }
    if (m_session.state() != GameSession::State::Running) return;
    switch (event->key()) {
    case Qt::Key_1: m_ui.selectedTowerType = TowerType::Normal; break;
    case Qt::Key_2: m_ui.selectedTowerType = TowerType::Fast; break;
    case Qt::Key_3: m_ui.selectedTowerType = TowerType::Heavy; break;
    case Qt::Key_U: applyAction(m_session.upgradeSelectedTower()); return;
    case Qt::Key_X:
    case Qt::Key_Delete: applyAction(m_session.sellSelectedTower()); return;
    default: return;
    }
    update();
}
