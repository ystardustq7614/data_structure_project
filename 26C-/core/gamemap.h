/** @file
 * @brief 固定地图的坐标、路径和放置几何；渲染与交易共用这些规则，避免判定漂移。 */
#ifndef GAMEMAP_H
#define GAMEMAP_H

#include <QPointF>
#include <QVector>

/** 与背景对应的逻辑像素地图；不持有实体，不随窗口缩放修改路径或网格。 */
class GameMap
{
public:
    // 画布、HUD 和网格均使用游戏逻辑像素；顶部 HUD 区域不能建造。
    static constexpr int Width = 1200;
    static constexpr int Height = 800;
    static constexpr int HudHeight = 60;
    static constexpr int GridSize = 60;

    /** 创建两条固定有序路径；路径坐标与内置地图素材共同构成当前关卡。 */
    GameMap();
    /** @name 只读几何查询
     * 返回逻辑像素尺寸及路径；paths() 借用引用随地图对象有效，无失败结果。
     * @{ */
    static constexpr int width() { return Width; }
    static constexpr int height() { return Height; }
    static constexpr int hudHeight() { return HudHeight; }
    static constexpr int gridSize() { return GridSize; }
    const QVector<QVector<QPointF>>& paths() const { return m_paths; }
    /** @} */
    /** 将 position 限制到可用行列后返回网格中心；非有限坐标返回原点。
     * 此方法只做吸附，不判定原位置是否合法；交易应先拒绝画布外与 HUD 输入。
     */
    QPointF snapToGrid(const QPointF& position) const;
    /** 判断半径为非负逻辑像素的圆是否完整落在 HUD 下方；非法输入返回 false。 */
    bool circleInsideMap(const QPointF& center, int radius) const;
    /** 对有限 center 检查距任一路径线段是否小于 45 像素；恰为 45 不属于道路禁建区。 */
    bool isNearRoad(const QPointF& center) const;

private:
    QVector<QVector<QPointF>> m_paths;
};

#endif // GAMEMAP_H
