/** @file
 * @brief 共享的按钮状态外观；只叠加反馈，不重画背景中已包含的按钮主体。 */
#include "buttonfeedback.h"
#include <QPainter>
#include <QPen>

void UiDrawing::drawButtonFeedback(QPainter& painter, const QRect& rect,
                                   bool hovered, bool pressed, bool selected, int radius)
{
    if (!hovered && !pressed && !selected) return;
    // 按压优先于选中/悬停；内缩 2 像素避免描边越过共享命中矩形。
    painter.setPen(QPen(pressed ? QColor(255, 218, 100) : QColor(245, 225, 155),
                        pressed ? 3 : 2));
    painter.setBrush(pressed ? QColor(0, 0, 0, 78)
                             : selected ? QColor(245, 204, 75, 65)
                                        : QColor(255, 255, 255, 32));
    painter.drawRoundedRect(rect.adjusted(2, 2, -2, -2), radius, radius);
}
