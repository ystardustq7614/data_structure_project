#include "startmenuwidget.h"

#include <QPainter>
#include <QApplication>
#include <QPen>
#include <QFont>
#include <QtGlobal>

StartMenuWidget::StartMenuWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(1200, 800);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    // 从 resources.qrc 里读取进入界面和说明界面图片
    m_menuBackground = QPixmap(":/images/images/menu_bg.jpg");
    m_helpBackground = QPixmap(":/images/images/help_bg.png");

    // 进入界面的三个点击区域
    // QRect(x, y, width, height)
    m_startButtonRect = QRect(218, 345, 303, 83);   // 开始游戏: X[218, 520], Y[345, 427]
    m_helpButtonRect  = QRect(239, 446, 261, 63);   // 游戏说明: X[239, 499], Y[446, 508]
    m_exitButtonRect  = QRect(239, 526, 261, 63);   // 退出游戏: X[239, 499], Y[526, 588]

    // 游戏说明页左上角返回区域
    m_backButtonRect  = QRect(230, 695, 280, 75);
}

void StartMenuWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 当前显示游戏说明界面
    if (m_showHelp) {
        if (!m_helpBackground.isNull()) {
            painter.drawPixmap(rect(), m_helpBackground);
        }
        else {
            painter.fillRect(rect(), QColor(70, 110, 70));

            painter.setPen(Qt::white);
            QFont font = painter.font();
            font.setPointSize(26);
            font.setBold(true);
            painter.setFont(font);

            painter.drawText(rect(),
                             Qt::AlignCenter,
                             "游戏说明图片加载失败\n请检查 images/help_bg.png");
        }
        return;
    }

    // 当前显示进入界面
    if (!m_menuBackground.isNull()) {
        painter.drawPixmap(rect(), m_menuBackground);
    }
    else {
        painter.fillRect(rect(), QColor(70, 110, 70));

        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPointSize(26);
        font.setBold(true);
        painter.setFont(font);

        painter.drawText(rect(),
                         Qt::AlignCenter,
                         "进入界面图片加载失败\n请检查 images/menu_bg.jpg");
    }

    // 后面如果点击范围不准，可以临时打开这几行红框调试
    /*
    painter.setPen(QPen(Qt::red, 3));
    painter.drawRect(m_startButtonRect);
    painter.drawRect(m_helpButtonRect);
    painter.drawRect(m_exitButtonRect);
    */
}

void StartMenuWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPoint clickPos = event->position().toPoint();
#else
    QPoint clickPos = event->pos();
#endif

    // 如果当前在游戏说明界面，只处理返回按钮
    if (m_showHelp) {
        if (m_backButtonRect.contains(clickPos)) {
            m_showHelp = false;
            update();
        }

        return;
    }

    // 开始游戏
    if (m_startButtonRect.contains(clickPos)) {
        emit startGameClicked();
        return;
    }

    // 游戏说明
    if (m_helpButtonRect.contains(clickPos)) {
        m_showHelp = true;
        update();
        return;
    }

    // 退出游戏
    if (m_exitButtonRect.contains(clickPos)) {
        QApplication::quit();
        return;
    }
}

void StartMenuWidget::mouseMoveEvent(QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPoint pos = event->position().toPoint();
#else
    QPoint pos = event->pos();
#endif

    if (isOnButton(pos)) {
        setCursor(Qt::PointingHandCursor);
    }
    else {
        setCursor(Qt::ArrowCursor);
    }
}

void StartMenuWidget::keyPressEvent(QKeyEvent *event)
{
    // 在说明界面按 Esc 也可以返回主菜单
    if (m_showHelp && event->key() == Qt::Key_Escape) {
        m_showHelp = false;
        update();
        return;
    }

    QWidget::keyPressEvent(event);
}

bool StartMenuWidget::isOnButton(const QPoint& pos) const
{
    if (m_showHelp) {
        return m_backButtonRect.contains(pos);
    }

    return m_startButtonRect.contains(pos)
           || m_helpButtonRect.contains(pos)
           || m_exitButtonRect.contains(pos);
}
