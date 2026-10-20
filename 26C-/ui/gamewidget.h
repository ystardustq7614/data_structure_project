/** @file
 * @brief Qt 输入和时钟适配；将事件转换为 GameSession 命令，并让 GameRenderer 读取战局。 */
#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include "core/gamesession.h"
#include "gamerenderer.h"
#include "gameuistate.h"
#include <QWidget>
class QTimer;

/** 持有单个战局及其唯一 QTimer 的游戏页面；隐藏页面前须显式停止，隐藏本身不暂停定时器。 */
class GameWidget : public QWidget
{
    Q_OBJECT
public:
    /** 在 parent 下创建固定尺寸页面并立即重置开局；父对象接管 QWidget/QTimer 生命周期。 */
    explicit GameWidget(QWidget* parent = nullptr);
    /** 销毁成员战局并由 Qt 父子关系释放计时器；外部不得保留实体或页面借用。 */
    ~GameWidget() override = default;
    /** 停止旧时钟、清交互并重新读取新局配置；startTimer=false 用于返回菜单。
     * 内置配置失败保持停止并显示错误；外部覆盖无效回退默认计划，开局时显示警告。
     * true 仅在获得有效计划时启动；重置暂停局会发 pauseChanged(false) 同步音频。
     */
    void restartGame(bool startTimer = true);
    using BuildProblem = GameSession::BuildProblem;
    using BuildEvaluation = GameSession::BuildEvaluation;
    /** 对页面逻辑坐标 rawPosition 判定当前选中塔型的建造候选，并包含按钮/面板遮挡。
     * 返回失败原因而不建塔或扣款；OutsideMap 也涵盖当前界面不接受地图输入的情况。
     */
    BuildEvaluation evaluateBuild(const QPointF& rawPosition) const;
    /** @name 战局只读快照
     * 金币、塔数、暂停标志及逻辑 tick 均直接查询唯一 GameSession，不驱动模拟或报告错误。
     * @{ */
    int gold() const { return m_session.gold(); }
    int towerCount() const { return m_session.towerCount(); }
    bool isPaused() const { return m_session.isPaused(); }
    qint64 gameTick() const { return m_session.gameTick(); }
    /** @} */
signals:
    /** 最终波实际开始时发出，供 MainWindow 切换音频；不表示 Boss 已生成。 */
    void finalWaveStarted();
    /** 请求放弃本局回菜单；接收者必须调用 restartGame(false) 并切换页面。 */
    void backToMenuRequested();
    /** paused 为切换后的状态；接收者据此暂停或恢复音乐，不改变战局。 */
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
    // 计时器完全跟随战局状态；包含调度异常、结算与重置入口，避免只停绘制而继续模拟。
    void syncTimer();
    void setPaused(bool paused);
    void applyAction(const GameSession::ActionResult& result);
    QTimer* m_timer = nullptr;
    GameSession m_session;
    GameRenderer m_renderer;
    GameUiState m_ui;
};

#endif
