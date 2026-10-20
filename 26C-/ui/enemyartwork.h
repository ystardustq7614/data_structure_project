/** @file
 * @brief 敌人透明贴图的加载适配；菜单与战斗复用裁边/缩放规则，各自持有结果缓存。 */
#ifndef ENEMYARTWORK_H
#define ENEMYARTWORK_H

#include <QPixmap>
#include <QSize>
#include <QString>

namespace EnemyArtwork {
/** 读取 resourcePath，裁透明留白后按原比例适配到正尺寸 bounds 内。
 * 返回 pixmap 不保证填满 bounds；读取失败或全透明时返回空图，由调用者提供备用绘制。
 * 本函数不缓存；应在资源加载阶段调用并保存结果，避免每次 paint 重复扫描像素。
 */
QPixmap load(const QString& resourcePath, const QSize& bounds);
}

#endif
