#include "enemyartwork.h"

#include <QImage>
#include <algorithm>

QPixmap EnemyArtwork::load(const QString& resourcePath, const QSize& bounds)
{
    QImage image(resourcePath);
    if (image.isNull()) return {};

    if (image.hasAlphaChannel()) {
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
        image = image.copy(QRect(QPoint(left, top), QPoint(right, bottom)));
    }

    return QPixmap::fromImage(image).scaled(bounds, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
}
