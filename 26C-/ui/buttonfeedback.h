#ifndef BUTTONFEEDBACK_H
#define BUTTONFEEDBACK_H

#include <QRect>
class QPainter;

namespace UiDrawing {
// 共享视觉反馈，输入状态仍由各页面维护；菜单和战斗沿用各自的圆角。
void drawButtonFeedback(QPainter& painter, const QRect& rect, bool hovered,
                        bool pressed, bool selected = false, int radius = 13);
}

#endif
