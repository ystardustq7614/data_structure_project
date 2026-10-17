#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include "core/gamesession.h"
#include "gamerenderer.h"
#include "gameuistate.h"
#include <QWidget>
class QTimer;

// Qt 适配层：输入转战局命令、驱动唯一计时器并请求绘制，不持有第二份战斗状态。
class GameWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GameWidget(QWidget* parent = nullptr);
    ~GameWidget() override = default;
    // false 用于返回菜单：仍重置本局和配置，但不启动计时器。
    void restartGame(bool startTimer = true);
    using BuildProblem = GameSession::BuildProblem;
    using BuildEvaluation = GameSession::BuildEvaluation;
    BuildEvaluation evaluateBuild(const QPointF& rawPosition) const;
    int gold() const { return m_session.gold(); }
    int towerCount() const { return m_session.towerCount(); }
    bool isPaused() const { return m_session.isPaused(); }
    qint64 gameTick() const { return m_session.gameTick(); }
signals:
    void finalWaveStarted();
    void backToMenuRequested();
    void pauseChanged(bool paused);
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
private slots:
    void updateGame();
private:
    friend class GameWidgetProductTestAccess;
    using Control = GameControl;
    void syncTimer();
    void setPaused(bool paused);
    void applyAction(const GameSession::ActionResult& result);
    QTimer* m_timer = nullptr;
    GameSession m_session;
    GameRenderer m_renderer;
    GameUiState m_ui;
};

#endif
