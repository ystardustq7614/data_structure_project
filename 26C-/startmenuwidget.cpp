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
    m_startButtonRect = QRect(218, 345, 303, 83);  // 开始游戏
    m_helpButtonRect  = QRect(239, 446, 261, 63);  // 游戏说明
    m_exitButtonRect  = QRect(239, 526, 261, 63);  // 退出游戏

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
        drawButtonFeedback(painter, Button::Back);
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

    for (Button button : {Button::Start, Button::Help, Button::Exit}) {
        drawButtonFeedback(painter, button);
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

    m_pressedButton = buttonAt(clickPos);
    m_hoveredButton = m_pressedButton;
    update();
}

void StartMenuWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    const Button released = buttonAt(event->position().toPoint());
    const Button pressed = m_pressedButton;
    m_pressedButton = Button::None;
    m_hoveredButton = released;
    update();
    if (pressed == Button::None || pressed != released) return;
    if (released == Button::Start) {
        clearInteraction();
        emit startGameClicked();
    } else if (released == Button::Help) {
        m_showHelp = true;
        clearInteraction();
        update();
    } else if (released == Button::Back) {
        m_showHelp = false;
        clearInteraction();
        update();
    } else if (released == Button::Exit) {
        QApplication::quit();
    }
}

void StartMenuWidget::mouseMoveEvent(QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPoint pos = event->position().toPoint();
#else
    QPoint pos = event->pos();
#endif

    m_hoveredButton = buttonAt(pos);
    if (m_pressedButton != Button::None && m_pressedButton != m_hoveredButton) {
        m_pressedButton = Button::None;
    }
    if (isOnButton(pos)) {
        setCursor(Qt::PointingHandCursor);
    }
    else {
        setCursor(Qt::ArrowCursor);
    }
    update();
}

void StartMenuWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    clearInteraction();
    update();
}

void StartMenuWidget::focusOutEvent(QFocusEvent *event)
{
    clearInteraction();
    update();
    QWidget::focusOutEvent(event);
}

void StartMenuWidget::keyPressEvent(QKeyEvent *event)
{
    // 在说明界面按 Esc 也可以返回主菜单
    if (m_showHelp && event->key() == Qt::Key_Escape) {
        m_showHelp = false;
        clearInteraction();
        update();
        return;
    }

    QWidget::keyPressEvent(event);
}

bool StartMenuWidget::isOnButton(const QPoint& pos) const
{
    return buttonAt(pos) != Button::None;
}

StartMenuWidget::Button StartMenuWidget::buttonAt(const QPoint& pos) const
{
    if (m_showHelp) return m_backButtonRect.contains(pos) ? Button::Back : Button::None;
    if (m_startButtonRect.contains(pos)) return Button::Start;
    if (m_helpButtonRect.contains(pos)) return Button::Help;
    if (m_exitButtonRect.contains(pos)) return Button::Exit;
    return Button::None;
}

QRect StartMenuWidget::buttonRect(Button button) const
{
    switch (button) {
    case Button::Start: return m_startButtonRect;
    case Button::Help: return m_helpButtonRect;
    case Button::Exit: return m_exitButtonRect;
    case Button::Back: return m_backButtonRect;
    case Button::None: return QRect();
    }
    return QRect();
}

void StartMenuWidget::drawButtonFeedback(QPainter& painter, Button button) const
{
    if (button != m_hoveredButton && button != m_pressedButton) return;
    const bool pressed = button == m_pressedButton;
    painter.setPen(QPen(pressed ? QColor(255, 218, 100) : QColor(245, 225, 155),
                        pressed ? 3 : 2));
    painter.setBrush(pressed ? QColor(0, 0, 0, 78)
                             : QColor(255, 255, 255, 32));
    painter.drawRoundedRect(buttonRect(button).adjusted(2, 2, -2, -2), 20, 20);
}

void StartMenuWidget::clearInteraction()
{
    m_hoveredButton = Button::None;
    m_pressedButton = Button::None;
    setCursor(Qt::ArrowCursor);
}
