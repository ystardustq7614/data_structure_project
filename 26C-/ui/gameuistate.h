#ifndef GAMEUISTATE_H
#define GAMEUISTATE_H

#include "core/tower.h"
#include <QPointF>
#include <QString>

enum class GameControl { None, Preview, Pause, Exit, Retry, Menu };

// 仅保存界面交互与字幕。生命、金币、选中实体和运行状态由 GameSession 唯一持有。
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

    void showStatus(const QString& text, int frames)
    {
        statusMessage = text;
        statusMessageCounter = frames;
    }
    // 仅在实际推进的一帧末调用；新提示当帧减一，暂停不计时。
    void advanceSubtitle()
    {
        if (statusMessageCounter > 0) --statusMessageCounter;
    }
    void resetInteraction()
    {
        hoveredControl = GameControl::None;
        pressedControl = GameControl::None;
        hasMousePosition = false;
    }
};

#endif
