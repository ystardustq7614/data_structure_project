#ifndef STARTMENUWIDGET_H
#define STARTMENUWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QRect>
#include <QMouseEvent>
#include <QKeyEvent>

class StartMenuWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StartMenuWidget(QWidget *parent = nullptr);

signals:
    void startGameClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QPixmap m_menuBackground;   // 进入界面图片
    QPixmap m_helpBackground;   // 游戏说明图片

    bool m_showHelp = false;    // false 显示进入界面，true 显示说明界面

    QRect m_startButtonRect;    // 开始游戏点击范围
    QRect m_helpButtonRect;     // 游戏说明点击范围
    QRect m_exitButtonRect;     // 退出游戏点击范围
    QRect m_backButtonRect;     // 说明界面返回按钮范围

    bool isOnButton(const QPoint& pos) const;
};

#endif // STARTMENUWIDGET_H
