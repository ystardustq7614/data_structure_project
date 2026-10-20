/** @file
 * @brief 菜单和游戏共用的按钮反馈绘制；仅消费输入状态，不执行命中判定或业务动作。 */
#ifndef BUTTONFEEDBACK_H
#define BUTTONFEEDBACK_H

#include <QRect>
class QPainter;

namespace UiDrawing {
/** 在活动 painter 的逻辑 rect 内画反馈；hovered/pressed/selected 由页面维护。
 * radius 为逻辑像素圆角，菜单与战斗可分别指定；仅在 rect 内缩的范围绘制，不改变命中布局。
 * 全部未激活时不绘制；激活时会改变 painter 的画笔/画刷，由调用者管理状态，无失败结果。
 */
void drawButtonFeedback(QPainter& painter, const QRect& rect, bool hovered,
                        bool pressed, bool selected = false, int radius = 13);
}

#endif
