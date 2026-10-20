/** @file
 * @brief 资源加载期的透明边界提取；扫描成本只在加载时承担，绘制使用调用方缓存。 */
#include "enemyartwork.h"

#include <QImage>
#include <algorithm>

QPixmap EnemyArtwork::load(const QString& resourcePath, const QSize& bounds)
{
    QImage image(resourcePath);
    if (image.isNull()) return {};

    if (image.hasAlphaChannel()) {
        // 统一为 ARGB32 后才能按 QRgb 行读取 alpha；非零透明度的抗锯齿边缘也必须保留。
        image = image.convertToFormat(QImage::Format_ARGB32);
        int left = image.width(), top = image.height(), right = -1, bottom = -1;
        for (int y = 0; y < image.height(); ++y) {
            const auto* row = reinterpret_cast<const QRgb*>(image.constScanLine(y));
            for (int x = 0; x < image.width(); ++x) {
                if (qAlpha(row[x]) == 0) continue;
                left = std::min(left, x);
                right = std::max(right, x);
                top = std::min(top, y);
                bottom = std::max(bottom, y);
            }
        }
        if (right < left || bottom < top) return {};
        // 两角式 QRect 包含右/下端点，边界像素不能被当作排他范围裁掉。
        image = image.copy(QRect(QPoint(left, top), QPoint(right, bottom)));
    }

    return QPixmap::fromImage(image).scaled(bounds, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
}
