/** @file
 * @brief 页面交互与字幕的短期状态；可整体重置，不复制生命、金币或实体选择等战局数据。 */
#ifndef GAMEUISTATE_H
#define GAMEUISTATE_H

#include "core/tower.h"
#include <QPointF>
#include <QString>

/** 共用的按钮标识；输入与 Renderer 必须使用相同标识和命中布局。 */
enum class GameControl { None, Preview, Pause, Exit, Retry, Menu };

/** GameWidget 独占的可变交互数据；Renderer 只读访问，字幕按实际推进 tick 计时。 */
struct GameUiState
{
    GameControl hoveredControl = GameControl::None;
    GameControl pressedControl = GameControl::None;
    QPointF mousePosition;
    bool hasMousePosition = false;
    bool previewVisible = false;
    bool debugDrawPath = false;
    TowerType selectedTowerType = TowerType::Normal;
    QString statusMessage;
    int statusMessageCounter = 0;

    /** 用 text 替换当前字幕，并将计数设为 frames；frames<=0 时不显示，不自行请求重绘。 */
    void showStatus(const QString& text, int frames)
    {
        statusMessage = text;
        statusMessageCounter = frames;
    }
    /** 仅在实际推进的一帧末调用；新提示当帧减一，暂停不计时，不清除文本本身。 */
    void advanceSubtitle()
    {
        if (statusMessageCounter > 0) --statusMessageCounter;
    }
    /** 清悬停、按压和鼠标有效性；保留塔型、预览开关及字幕，不改变战局暂停状态。 */
    void resetInteraction()
    {
        hoveredControl = GameControl::None;
        pressedControl = GameControl::None;
        hasMousePosition = false;
    }
};

#endif
