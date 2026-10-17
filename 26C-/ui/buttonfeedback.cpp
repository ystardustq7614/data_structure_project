#include "buttonfeedback.h"
#include <QPainter>
#include <QPen>

void UiDrawing::drawButtonFeedback(QPainter& painter, const QRect& rect,
                                   bool hovered, bool pressed, bool selected, int radius)
{
    if (!hovered && !pressed && !selected) return;
    painter.setPen(QPen(pressed ? QColor(255, 218, 100) : QColor(245, 225, 155),
                        pressed ? 3 : 2));
    painter.setBrush(pressed ? QColor(0, 0, 0, 78)
                             : selected ? QColor(245, 204, 75, 65)
                                        : QColor(255, 255, 255, 32));
    painter.drawRoundedRect(rect.adjusted(2, 2, -2, -2), radius, radius);
}
