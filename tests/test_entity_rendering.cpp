#include "ui/gamerenderer.h"
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <cstdlib>
#include <iostream>

namespace {
void require(bool ok, const char* message)
{
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
QImage canvas()
{
    QImage result(500, 500, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);
    return result;
}
QImage renderEnemy(const GameRenderer& renderer, const Enemy& enemy)
{
    auto result = canvas();
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.drawEnemy(painter, enemy);
    return result;
}
void mutate(Enemy& enemy)
{
    enemy.update();
    enemy.update();
    require(enemy.hasMutated(), "fixture did not mutate actual Enemy");
}
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    // 本目标不链接图片 qrc，自然验证产品缺图分支，不给产品增加测试开关。
    GameRenderer renderer;
    const QVector<QPointF> path{{100, 100}, {100, 100}, {100, 100}, {1000, 100}};
    Enemy virus("virus", path);
    auto image = renderEnemy(renderer, virus);
    require(image.pixelColor(120, 120) == QColor(130, 80, 200), "Virus fallback changed");
    Enemy ddl("ddl", path), boss("boss", path), unknown("other", path), bug("bug", path);
    require(renderEnemy(renderer, ddl).pixelColor(120, 120) == QColor(240, 150, 50), "DDL fallback changed");
    require(renderEnemy(renderer, boss).pixelColor(125, 120) == QColor(70, 70, 70), "Boss fallback changed");
    require(renderEnemy(renderer, unknown).pixelColor(120, 120) == QColor(220, 70, 70), "unknown fallback changed");
    require(renderEnemy(renderer, bug).pixelColor(100, 56) == QColor(60, 200, 80), "Bug blood bar missing");

    virus.takeDamage(virus.armor() + virus.maxHp() / 2);
    image = renderEnemy(renderer, virus);
    require(image.pixelColor(82, 56) == QColor(60, 200, 80)
            && image.pixelColor(120, 56) == QColor(180, 40, 40), "half HP bar width changed");
    Enemy lateFirst("virus", path), mutationFirst("virus", path);
    lateFirst.applyLateGameEnhance();
    mutate(lateFirst);
    mutate(mutationFirst);
    mutationFirst.applyLateGameEnhance();
    require(renderEnemy(renderer, lateFirst).pixelColor(120, 120) == QColor(80, 40, 150),
            "mutation must overwrite earlier fallback darkening");
    // QImage 为8位通道，darker() 的 QColor 可含16位精度；比较实际输出色值。
    require(renderEnemy(renderer, mutationFirst).pixelColor(120, 120).rgba()
                == QColor(80, 40, 150).darker(130).rgba(),
            "enhancement after mutation must darken fallback");

    for (TowerType type : {TowerType::Normal, TowerType::Fast, TowerType::Heavy}) {
        Tower tower(QPointF(250, 250), type);
        const int before = tower.bodyDiameter();
        auto normal = canvas();
        { QPainter p(&normal); renderer.drawTower(p, tower, false); }
        require(normal.pixelColor(250 + before / 2 - 3, 250).alpha() > 0, "tower body missing");
        require(tower.upgrade() && tower.bodyDiameter() == before + 2, "tower size upgrade changed");
        require(tower.containsPoint(QPointF(250 + tower.bodyDiameter() / 2 + 8, 250)),
                "upgraded click radius does not match body");
        auto selected = canvas();
        { QPainter p(&selected); renderer.drawTower(p, tower, true); }
        int outerPixels = 0;
        for (int y = 0; y < 500; ++y)
            for (int x = 0; x < 500; ++x)
                if ((x < 200 || x > 300 || y < 200 || y > 300)
                    && selected.pixelColor(x, y).alpha() > 0) ++outerPixels;
        require(outerPixels > 100, "selected tower range was not drawn");
        auto unselected = canvas();
        { QPainter p(&unselected); renderer.drawTower(p, tower, false); }
        require(unselected.pixelColor(250 + tower.range(), 250).alpha() == 0,
                "unselected tower retained its range ring");
    }

    Enemy target("bug", path);
    Bullet bullet(QPointF(200, 200), &target, 18);
    image = canvas();
    { QPainter p(&image); renderer.drawBullet(p, bullet); }
    require(image.pixelColor(200, 200) == QColor(255, 220, 80)
            && image.pixelColor(206, 200).alpha() == 255
            && image.pixelColor(210, 200).alpha() == 0, "bullet radius or color changed");
    target.takeDamage(10000);
    bullet.update();
    image = canvas();
    { QPainter p(&image); renderer.drawBullet(p, bullet); }
    require(image.pixelColor(200, 200).alpha() == 0, "finished bullet was painted");
    std::cout << "PASS: actual fallback entity rendering and geometry\n";
}
