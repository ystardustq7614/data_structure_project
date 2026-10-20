/** @file
 * @brief 固定背景上的菜单交互；按钮坐标以 1200x800 逻辑画布为基准，不使用原图像素坐标。 */
#include "startmenuwidget.h"

#include <QPainter>
#include "buttonfeedback.h"
#include "enemyartwork.h"
#include <QPen>
#include <QFont>
#include <QLinearGradient>
#include <QtGlobal>

StartMenuWidget::StartMenuWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(1200, 800);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    // prefix /images 与文件相对路径 images/... 组合为资源 URL 中的 /images/images/...。
    m_menuBackground = QPixmap(":/images/images/screens/menu.png");
    m_helpBackground = QPixmap(":/images/images/screens/help.png");
    m_enemyIcons = {
        EnemyArtwork::load(":/images/images/enemies/bug.png", QSize(62, 68)),
        EnemyArtwork::load(":/images/images/enemies/ddl.png", QSize(62, 68)),
        EnemyArtwork::load(":/images/images/enemies/virus.png", QSize(62, 68)),
        EnemyArtwork::load(":/images/images/enemies/boss.png", QSize(62, 68))
    };

    // 按背景按钮主体标定，避开草地留白与按钮间隙；悬停、按压和释放共用这些矩形。
    m_startButtonRect = QRect(218, 345, 303, 83);
    m_helpButtonRect  = QRect(239, 446, 261, 63);
    m_exitButtonRect  = QRect(239, 526, 261, 63);

    // 说明页返回按钮位于底部，与主菜单三个按钮使用不同布局。
    m_backButtonRect  = QRect(230, 695, 280, 75);
}

void StartMenuWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

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
                             "游戏说明图片加载失败\n请检查 images/screens/help.png");
        }
        drawButtonFeedback(painter, Button::Back);
        return;
    }

    if (!m_menuBackground.isNull()) {
        painter.drawPixmap(rect(), m_menuBackground);
        drawEnemyRoster(painter);
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
                         "进入界面图片加载失败\n请检查 images/screens/menu.png");
    }

    for (Button button : {Button::Start, Button::Help, Button::Exit}) {
        drawButtonFeedback(painter, button);
    }

}

void StartMenuWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

// 下方保留历史坐标 API 分支；当前只支持 Qt 6，清除遗留分支时可一并移除 Qt 5 写法。
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
    // 只接受同一按钮内按下并释放；切说明页前清交互，避免新页面继承旧按压。
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
        emit quitRequested();
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
    UiDrawing::drawButtonFeedback(painter, buttonRect(button), button == m_hoveredButton,
                                  button == m_pressedButton, false, 20);
}

void StartMenuWidget::clearInteraction()
{
    m_hoveredButton = Button::None;
    m_pressedButton = Button::None;
    setCursor(Qt::ArrowCursor);
}

void StartMenuWidget::drawEnemyRoster(QPainter& painter) const
{
    // 背景角色已烘焙进图片；此面板在纸张范围叠加四类基础敌人的实际资源与名称。
    const QRect panel(695, 212, 315, 190);
    QLinearGradient paper(panel.topLeft(), panel.bottomLeft());
    paper.setColorAt(0, QColor(250, 239, 207));
    paper.setColorAt(1, QColor(226, 203, 151));
    painter.setPen(QPen(QColor(143, 108, 55), 2));
    painter.setBrush(paper);
    painter.drawRoundedRect(panel, 12, 12);

    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(18);
    painter.setFont(font);
    painter.setPen(QColor(65, 74, 39));
    painter.drawText(QRect(704, 217, 297, 31), Qt::AlignCenter, "敌方阵营");

    const char* names[] = {"小怪", "DDL", "Virus", "Boss"};
    for (int i = 0; i < static_cast<int>(m_enemyIcons.size()); ++i) {
        const QRect card(703 + i * 76, 255, 70, 111);
        painter.setPen(QPen(QColor(169, 136, 79), 1));
        painter.setBrush(QColor(255, 249, 230));
        painter.drawRoundedRect(card, 8, 8);

        const QRect iconArea(card.left() + 4, card.top() + 4, 62, 68);
        const QPixmap& icon = m_enemyIcons[i];
        if (!icon.isNull()) {
            painter.drawPixmap(iconArea.x() + (iconArea.width() - icon.width()) / 2,
                               iconArea.y() + (iconArea.height() - icon.height()) / 2, icon);
        }

        font.setPixelSize(14);
        painter.setFont(font);
        painter.setPen(QColor(65, 74, 39));
        painter.drawText(QRect(card.left(), 335, card.width(), 24),
                         Qt::AlignCenter, QString::fromUtf8(names[i]));
    }

    font.setBold(false);
    font.setPixelSize(12);
    painter.setFont(font);
    painter.setPen(QColor(86, 91, 56));
    painter.drawText(QRect(703, 372, 299, 22), Qt::AlignCenter,
                     "Virus 可在战斗中变异");
}
