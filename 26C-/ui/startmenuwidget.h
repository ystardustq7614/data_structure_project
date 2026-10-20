/** @file
 * @brief 菜单/说明页的输入和只读展示；通过信号交给 MainWindow 决定开局或退出。 */
#ifndef STARTMENUWIDGET_H
#define STARTMENUWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QRect>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <array>
class QPainter;

/** 固定尺寸的菜单与说明页面；按钮命中和反馈共用布局，素材缓存随控件生命周期有效。 */
class StartMenuWidget : public QWidget
{
    Q_OBJECT

public:
    /** 在 parent 下创建菜单并加载嵌入素材；缺背景时绘制错误提示，不提供单独加载错误返回。 */
    explicit StartMenuWidget(QWidget *parent = nullptr);

signals:
    /** 开始按钮在同一按钮按下并释放后发出；不在本页面创建或推进战局。 */
    void startGameClicked();
    /** 退出按钮确认点击后发出，由窗口关闭应用；无返回值。 */
    void quitRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QPixmap m_menuBackground;
    QPixmap m_helpBackground;
    // 按 bug/ddl/virus/boss 顺序缓存；变异体属于战斗中的技能结果，不单列基础卡片。
    std::array<QPixmap, 4> m_enemyIcons;

    bool m_showHelp = false;    // false 显示进入界面，true 显示说明界面

    // 坐标对应缩放到固定画布后的背景；QRect 的右/下端点含在命中范围中。
    QRect m_startButtonRect;
    QRect m_helpButtonRect;
    QRect m_exitButtonRect;
    QRect m_backButtonRect;

    bool isOnButton(const QPoint& pos) const;
    enum class Button { None, Start, Help, Exit, Back };
    Button buttonAt(const QPoint& pos) const;
    QRect buttonRect(Button button) const;
    void drawEnemyRoster(QPainter& painter) const;
    void drawButtonFeedback(QPainter& painter, Button button) const;
    void clearInteraction();
    Button m_hoveredButton = Button::None;
    Button m_pressedButton = Button::None;
};

#endif // STARTMENUWIDGET_H
