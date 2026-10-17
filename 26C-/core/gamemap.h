#ifndef GAMEMAP_H
#define GAMEMAP_H

#include <QPointF>
#include <QVector>

// 固定地图及几何规则；不持有实体，也不依赖窗口、资源或输入事件。
class GameMap
{
public:
    static constexpr int Width = 1200;
    static constexpr int Height = 800;
    static constexpr int HudHeight = 60;
    static constexpr int GridSize = 60;

    GameMap();
    static constexpr int width() { return Width; }
    static constexpr int height() { return Height; }
    static constexpr int hudHeight() { return HudHeight; }
    static constexpr int gridSize() { return GridSize; }
    const QVector<QVector<QPointF>>& paths() const { return m_paths; }
    QPointF snapToGrid(const QPointF& position) const;
    bool circleInsideMap(const QPointF& center, int radius) const;
    bool isNearRoad(const QPointF& center) const;

private:
    QVector<QVector<QPointF>> m_paths;
};

#endif // GAMEMAP_H
