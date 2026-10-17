#ifndef ENEMYARTWORK_H
#define ENEMYARTWORK_H

#include <QPixmap>
#include <QSize>
#include <QString>

namespace EnemyArtwork {
// 仅在加载时裁去透明留白；菜单和战斗各自缓存按比例适配的贴图。
QPixmap load(const QString& resourcePath, const QSize& bounds);
}

#endif
