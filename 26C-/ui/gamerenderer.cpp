#include "gamerenderer.h"
#include "buttonfeedback.h"
#include "enemyartwork.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QPen>
#include <cmath>

GameRenderer::GameRenderer()
    : m_gameMapBackground(":/images/images/game_map.jpg"),
      m_winImage(":/images/images/screens/win.png"), m_loseImage(":/images/images/screens/lose.png")
{
    // 每种贴图只加载一次；模型构造、变异和强化均不触碰 GUI 资源。
    const QHash<QString, QString> paths{
        {"bug", ":/images/images/enemies/bug.png"},
        {"ddl", ":/images/images/enemies/ddl.png"},
        {"virus", ":/images/images/enemies/virus.png"},
        {"boss", ":/images/images/enemies/boss.png"},
        {"mutant", ":/images/images/enemies/mutant.png"}};
    for (auto it = paths.cbegin(); it != paths.cend(); ++it) {
        const int size = it.key() == "boss" ? 75 : 65;
        m_enemyIcons.insert(it.key(), EnemyArtwork::load(it.value(), QSize(size, size)));
    }

    // 独立塔贴图保留原菜单裁图像素，不再依赖已替换的整张菜单。
    m_towerPortraits = {
        QPixmap(":/images/images/towers/normal.png"),
        QPixmap(":/images/images/towers/fast.png"),
        QPixmap(":/images/images/towers/heavy.png")
    };
}

const QPixmap& GameRenderer::towerPortrait(TowerType type) const
{
    if (type == TowerType::Fast) return m_towerPortraits[1];
    if (type == TowerType::Heavy) return m_towerPortraits[2];
    return m_towerPortraits[0];
}

QString GameRenderer::towerName(TowerType type)
{
    if (type == TowerType::Fast) return "速射塔";
    if (type == TowerType::Heavy) return "重炮塔";
    return "普通塔";
}

void GameRenderer::paint(QPainter& painter, const GameSession& session,
                         const GameUiState& ui) const
{
    drawBackground(painter, session, ui);
    if (ui.debugDrawPath) {
        drawGrid(painter, session, ui);
        drawPath(painter, session, ui);
    }
    drawMapMarkers(painter, session, ui);
    for (int i = 0; i < session.towerCount(); ++i) {
        const Tower& tower = session.towerAt(i);
        drawTower(painter, tower, &tower == session.selectedTower());
    }
    for (int i = 0; i < session.enemyCount(); ++i) drawEnemy(painter, session.enemyAt(i));
    for (int i = 0; i < session.bulletCount(); ++i) drawBullet(painter, session.bulletAt(i));
    if (session.isFinished()) {
        drawGameResultScreen(painter, session, ui);
        return;
    }
    drawBuildPreview(painter, session, ui);
    if (session.isPaused()) drawPauseOverlay(painter, session, ui);
    drawHud(painter, session, ui);
    drawEventPreview(painter, session, ui);
    drawStatusMessage(painter, session, ui);
}

bool GameRenderer::hasTowerAt(const QPointF& pos, const GameSession& session) const
{
    for (int i = 0; i < session.towerCount(); ++i)
        if (session.towerAt(i).containsPoint(pos)) return true;
    return false;
}

bool GameRenderer::blocksMapInput(const QPointF& pos, const GameSession& session,
                                   const GameUiState& ui) const
{
    return !std::isfinite(pos.x()) || !std::isfinite(pos.y())
        || pos.y() < GameMap::HudHeight
        || controlAt(pos.toPoint(), session) != Control::None
        || (ui.previewVisible && previewPanelRect().contains(pos.toPoint()));
}

GameSession::BuildEvaluation GameRenderer::evaluateBuild(
    const QPointF& pos, const GameSession& session, const GameUiState& ui) const
{
    if (blocksMapInput(pos, session, ui)) return {};
    return session.evaluateBuild(pos, ui.selectedTowerType);
}

void GameRenderer::drawBackground(QPainter& painter, const GameSession& session,
                                   const GameUiState& ui) const
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

void GameRenderer::drawGrid(QPainter& painter, const GameSession& session,
                                   const GameUiState& ui) const
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

void GameRenderer::drawPath(QPainter& painter, const GameSession& session,
                                   const GameUiState& ui) const
{
    for (int pathIndex = 0; pathIndex < session.map().paths().size(); ++pathIndex) {
        const QVector<QPointF>& pathPoints = session.map().paths()[pathIndex];
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
    if (!session.map().paths().isEmpty() && !session.map().paths().first().isEmpty()) {
        QPointF endPoint = session.map().paths().first().last();

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

void GameRenderer::drawHud(QPainter& painter, const GameSession& session,
                                   const GameUiState& ui) const
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
                       .arg(session.gold())
                       .arg(session.life())
                       .arg(session.scheduler().currentWaveIndex() + 1)
                       .arg(session.scheduler().spawnedCount())
                       .arg(session.scheduler().totalEnemies())
                       .arg(towerName(ui.selectedTowerType));

    painter.drawText(QRectF(40, 12, 810, 48),
                     Qt::AlignVCenter | Qt::AlignLeft,
                     info);
    for (Control control : {Control::Preview, Control::Pause, Control::Exit}) {
        const QRect button = controlRect(control, session);
        painter.setPen(QPen(QColor(210, 230, 160, 170), 1));
        painter.setBrush(QColor(27, 52, 31, 225));
        painter.drawRoundedRect(button, 11, 11);
        UiDrawing::drawButtonFeedback(painter, button, ui.hoveredControl == control,
                           ui.pressedControl == control,
                           control == Control::Pause && session.isPaused());
        painter.setPen(Qt::white);
        painter.drawText(button, Qt::AlignCenter,
                         control == Control::Preview ? "预览"
                         : control == Control::Pause
                               ? (session.isPaused() ? "继续" : "暂停")
                               : "退出");
    }
}

void GameRenderer::drawStatusMessage(QPainter& painter, const GameSession& session,
                                   const GameUiState& ui) const
{
    if (ui.statusMessageCounter <= 0) {
        return;
    }

    QRectF box(190, 80, 630, ui.statusMessage.size() > 32 ? 76 : 56);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30, 30, 30, 180));
    painter.drawRoundedRect(box, 12, 12);

    painter.setPen(QColor(255, 230, 120));

    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(ui.statusMessage.size() > 32 ? 12 : 18);
    painter.setFont(font);

    painter.drawText(box.adjusted(12, 4, -12, -4),
                     Qt::AlignCenter | Qt::TextWordWrap, ui.statusMessage);
}

void GameRenderer::drawGameResultScreen(QPainter& painter, const GameSession& session,
                                   const GameUiState& ui) const
{
    QPixmap resultImage = session.isWon() ? m_winImage : m_loseImage;

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
                         session.isWon() ? "游戏胜利！" : "游戏失败！");
    }
    for (Control control : {Control::Retry, Control::Menu}) {
        UiDrawing::drawButtonFeedback(painter, controlRect(control, session),
                           ui.hoveredControl == control,
                           ui.pressedControl == control);
    }
}

void GameRenderer::drawBuildPreview(QPainter& painter, const GameSession& session,
                                   const GameUiState& ui) const
{
    if (!ui.hasMousePosition || session.state() != GameSession::State::Running
        || hasTowerAt(ui.mousePosition, session)) return;
    const GameSession::BuildEvaluation candidate = evaluateBuild(ui.mousePosition, session, ui);
    if (candidate.problem == GameSession::BuildProblem::OutsideMap) return;
    const bool valid = candidate.valid();
    const int radius = Tower::radiusForType(ui.selectedTowerType);
    const int diameter = radius * 2;

    painter.save();
    painter.setOpacity(0.72);
    drawTowerVisual(painter, candidate.center, ui.selectedTowerType, diameter, 1);
    painter.restore();

    painter.save();
    painter.setPen(QPen(valid ? QColor(80, 255, 145, 220)
                              : QColor(255, 95, 90, 230), 3));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(candidate.center, radius, radius);
    painter.restore();
}

void GameRenderer::drawPauseOverlay(QPainter& painter, const GameSession& session,
                                   const GameUiState& ui) const
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

void GameRenderer::drawEventPreview(QPainter& painter, const GameSession& session,
                                   const GameUiState& ui) const
{
    if (!ui.previewVisible) return;
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
    const QVector<AttackScheduler::Event> next = session.scheduler().preview(3);
    if (next.isEmpty()) {
        const QString text = session.scheduler().waitingForClear()
                                 ? QString("清场至 %1 名敌人后进入下一波")
                                       .arg(session.scheduler().clearThreshold())
                                 : "当前没有待执行事件";
        painter.drawText(QRect(panel.x() + 14, panel.y() + 48, 300, 25), text);
        return;
    }
    for (int i = 0; i < next.size(); ++i) {
        const auto& event = next.at(i);
        const qint64 remaining = qMax(qint64(0),
                                      event.dueTick - session.scheduler().currentTick());
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

void GameRenderer::drawMapMarkers(QPainter& painter, const GameSession& session,
                                   const GameUiState& ui) const
{
    if (session.map().paths().isEmpty()) {
        return;
    }

    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(14);
    painter.setFont(font);

    // 画两个起点
    for (int i = 0; i < session.map().paths().size(); ++i) {
        if (session.map().paths()[i].isEmpty()) {
            continue;
        }

        QPointF startPoint = session.map().paths()[i].first();

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
    QPointF endPoint = session.map().paths().first().last();

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

QRect GameRenderer::resultRetryRect(const GameSession& session) const
{
    if (session.isWon()) {
        // 胜利图里的“重新挑战”按钮
        return QRect(280, 465, 300, 95);
    }

    // 失败图里的“重新挑战”按钮
    return QRect(280, 565, 300, 95);
}

QRect GameRenderer::resultMenuRect(const GameSession& session) const
{
    if (session.isWon()) {
        // 胜利图里的“返回主页”按钮
        return QRect(600, 465, 300, 95);
    }

    // 失败图里的“返回主页”按钮
    return QRect(600, 565, 300, 95);
}

QRect GameRenderer::controlRect(Control control, const GameSession& session) const
{
    switch (control) {
    case Control::Preview: return QRect(870, 20, 92, 32);
    case Control::Pause: return QRect(970, 20, 92, 32);
    case Control::Exit: return QRect(1070, 20, 92, 32);
    case Control::Retry: return resultRetryRect(session);
    case Control::Menu: return resultMenuRect(session);
    case Control::None: return QRect();
    }
    return QRect();
}

QRect GameRenderer::previewPanelRect() const
{
    return QRect(840, 70, 330, 145);
}

GameRenderer::Control GameRenderer::controlAt(const QPoint& pos, const GameSession& session) const
{
    if (session.isFinished()) {
        if (resultRetryRect(session).contains(pos)) return Control::Retry;
        if (resultMenuRect(session).contains(pos)) return Control::Menu;
        return Control::None;
    }
    if (session.state() == GameSession::State::Stopped) {
        return controlRect(Control::Exit, session).contains(pos) ? Control::Exit : Control::None;
    }
    for (Control control : {Control::Preview, Control::Pause, Control::Exit}) {
        if (controlRect(control, session).contains(pos)) return control;
    }
    return Control::None;
}

void GameRenderer::drawEnemy(QPainter& painter, const Enemy& enemy) const
{
    const QString& type = enemy.type();
    const int size = type == "boss" ? 75 : 65;
    const int half = size / 2;
    QColor color;
    QString name;
    QString iconKey;
    if (enemy.hasMutated()) {
        color = QColor(80, 40, 150); name = "Mutant"; iconKey = "mutant";
    } else if (type == "bug") {
        // 保留原 Bug 缺图时的空标签/默认颜色，视觉修正不混入本轮重构。
        iconKey = "bug";
    } else if (type == "ddl") {
        color = QColor(240, 150, 50); name = "DDL"; iconKey = "ddl";
    } else if (type == "virus") {
        color = QColor(130, 80, 200); name = "Virus"; iconKey = "virus";
    } else if (type == "boss") {
        color = QColor(70, 70, 70); name = "Boss"; iconKey = "boss";
    } else {
        color = QColor(220, 70, 70); name = "Bug";
        // 直接构造 mutant 与真正变异不同，但原 late 方法仍会加载其图标。
        if (type == "mutant" && enemy.lateGameEnhanced()) iconKey = "mutant";
    }
    if (enemy.lateGameEnhanced() && !enemy.mutatedAfterLateEnhancement())
        color = color.darker(130);
    const QPixmap icon = m_enemyIcons.value(iconKey);
    const QPointF pos = enemy.position();
    if (!icon.isNull()) {
        // 按原图比例绘制，底边与原有敌人占位对齐，不再把竖图拉伸成正方形。
        painter.drawPixmap(static_cast<int>(pos.x()) - icon.width() / 2,
                           static_cast<int>(pos.y()) + half + 1 - icon.height(), icon);
    } else {
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawEllipse(pos, half, half);
        painter.setPen(Qt::white);
        painter.drawText(QRectF(pos.x() - half, pos.y() - half, size, size), Qt::AlignCenter, name);
    }
    const int barX = static_cast<int>(pos.x() - size / 2);
    const int barY = static_cast<int>(pos.y() - half - 12);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(180, 40, 40));
    painter.drawRect(barX, barY, size, 5);
    painter.setBrush(QColor(60, 200, 80));
    painter.drawRect(barX, barY, size * enemy.hp() / enemy.maxHp(), 5);
}

void GameRenderer::drawTowerVisual(QPainter& painter, const QPointF& pos, TowerType type,
                                   int diameter, int level) const
{
    const int half = diameter / 2;
    QColor bodyColor;
    QString label;
    if (type == TowerType::Normal) { bodyColor = QColor(80, 140, 230); label = "N"; }
    else if (type == TowerType::Fast) { bodyColor = QColor(80, 200, 120); label = "F"; }
    else if (type == TowerType::Heavy) { bodyColor = QColor(220, 120, 60); label = "H"; }

    const QRectF body(pos.x() - half, pos.y() - half, diameter, diameter);
    const QPixmap& portrait = towerPortrait(type);
    if (!portrait.isNull()) {
        painter.save();
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        QPainterPath clip;
        clip.addRoundedRect(body, 6, 6);
        painter.setClipPath(clip);
        painter.drawPixmap(body, portrait, QRectF(portrait.rect()));
        painter.restore();

        painter.setPen(QPen(bodyColor, 2));
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
        painter.drawText(badge, Qt::AlignCenter, QString::number(level));
    } else {
        // 菜单背景缺失时仍可辨识塔型和等级。
        painter.setPen(QPen(QColor(40, 60, 90), 3));
        painter.setBrush(bodyColor);
        painter.drawEllipse(pos, half, half);
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(12);
        painter.setFont(font);
        painter.setPen(QColor(20, 40, 80));
        painter.drawText(body, Qt::AlignCenter, label + QString::number(level));
    }
}

void GameRenderer::drawTower(QPainter& painter, const Tower& tower, bool selected) const
{
    painter.save();
    if (selected) {
        painter.setPen(QPen(QColor(255, 200, 60, 150), 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(tower.position(), tower.range(), tower.range());
    }
    drawTowerVisual(painter, tower.position(), tower.type(), tower.bodyDiameter(), tower.level());
    painter.restore();
}

void GameRenderer::drawBullet(QPainter& painter, const Bullet& bullet) const
{
    if (bullet.isFinished()) return;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 220, 80));
    // 原 m_size 是半径8，与追踪命中距离是独立规则。
    painter.drawEllipse(bullet.position(), 8, 8);
}
