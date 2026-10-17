#include "gamemap.h"

#include <QtMath>
#include <cmath>

namespace {

double distanceToSegment(const QPointF& point, const QPointF& start, const QPointF& end)
{
    const double vx = end.x() - start.x();
    const double vy = end.y() - start.y();
    const double wx = point.x() - start.x();
    const double wy = point.y() - start.y();
    const double lengthSquared = vx * vx + vy * vy;
    if (lengthSquared == 0) {
        return qSqrt(wx * wx + wy * wy);
    }
    const double fraction = qBound(0.0, (wx * vx + wy * vy) / lengthSquared, 1.0);
    const double dx = point.x() - (start.x() + fraction * vx);
    const double dy = point.y() - (start.y() + fraction * vy);
    return qSqrt(dx * dx + dy * dy);
}

} // namespace

GameMap::GameMap()
    : m_paths{
          {QPointF(40, 230), QPointF(305, 230), QPointF(305, 90),
           QPointF(635, 90), QPointF(635, 315), QPointF(1050, 315)},
          {QPointF(40, 570), QPointF(335, 570), QPointF(335, 690),
           QPointF(760, 690), QPointF(525, 395), QPointF(745, 485),
           QPointF(1050, 485), QPointF(1050, 315)}}
{
}

QPointF GameMap::snapToGrid(const QPointF& position) const
{
    // 先限制浮点坐标再转换整数，避免无效或极大输入造成转换溢出。
    if (!std::isfinite(position.x()) || !std::isfinite(position.y())) {
        return QPointF();
    }
    const int gridX = static_cast<int>(qBound(0.0, position.x(), double(Width - 1)))
                      / GridSize;
    const int gridY = qBound(1,
                            static_cast<int>(qBound(0.0, position.y(), double(Height - 1)))
                                / GridSize,
                            Height / GridSize - 1);
    return QPointF(gridX * GridSize + GridSize / 2,
                   gridY * GridSize + GridSize / 2);
}

bool GameMap::circleInsideMap(const QPointF& center, int radius) const
{
    return std::isfinite(center.x()) && std::isfinite(center.y()) && radius >= 0
           && center.x() - radius >= 0 && center.x() + radius < Width
           && center.y() - radius >= HudHeight && center.y() + radius < Height;
}

bool GameMap::isNearRoad(const QPointF& center) const
{
    for (const auto& path : m_paths) {
        for (int index = 0; index + 1 < path.size(); ++index) {
            if (distanceToSegment(center, path.at(index), path.at(index + 1)) < 45) {
                return true;
            }
        }
    }
    return false;
}
