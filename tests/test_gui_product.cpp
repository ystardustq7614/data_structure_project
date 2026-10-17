#include "ui/gamewidget.h"
#include "ui/mainwindow.h"
#include "ui/startmenuwidget.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QImage>
#include <QKeyEvent>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QResource>
#include <QStackedWidget>
#include <QTimer>

#include <cstdlib>
#include <iostream>

class GameSessionProductTestAccess
{
public:
    static void grantLives(GameSession& session) { session.m_life = 10000; }
    static void startRunning(GameSession& session) { session.m_state = GameSession::State::Running; }
};

class GameWidgetProductTestAccess
{
public:
    static QPointF firstTowerPosition(const GameWidget* game)
    {
        return game->m_session.towerAt(0).position();
    }
    static void grantLives(GameWidget* game)
    { GameSessionProductTestAccess::grantLives(game->m_session); }
    static bool won(const GameWidget* game) { return game->m_session.isWon(); }
    static TowerType selectedType(const GameWidget* game)
    {
        return game->m_ui.selectedTowerType;
    }
    static bool hasSelectedTower(const GameWidget* game)
    {
        return game->m_session.selectedTower() != nullptr;
    }
};

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void waitForEvents(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

void mouseMove(QWidget* widget, const QPoint& pos,
               Qt::MouseButtons buttons = Qt::NoButton)
{
    QMouseEvent event(QEvent::MouseMove, QPointF(pos),
                      QPointF(widget->mapToGlobal(pos)), Qt::NoButton,
                      buttons, Qt::NoModifier);
    QCoreApplication::sendEvent(widget, &event);
}

void mousePress(QWidget* widget, const QPoint& pos)
{
    const QPointF local(pos);
    const QPointF global(widget->mapToGlobal(pos));
    QMouseEvent press(QEvent::MouseButtonPress, local, global, Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(widget, &press);
}

void mouseRelease(QWidget* widget, const QPoint& pos)
{
    const QPointF local(pos);
    const QPointF global(widget->mapToGlobal(pos));
    QMouseEvent release(QEvent::MouseButtonRelease, local, global, Qt::LeftButton,
                        Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(widget, &release);
}

void mouseClick(QWidget* widget, const QPoint& pos)
{
    mousePress(widget, pos);
    mouseRelease(widget, pos);
}

void keyPress(QWidget* widget, int key)
{
    QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
    QCoreApplication::sendEvent(widget, &event);
}

void advanceToResult(GameWidget* game, QTimer* timer)
{
    for (int frame = 0; frame < 12000 && timer->isActive(); ++frame) {
        require(QMetaObject::invokeMethod(game, "updateGame", Qt::DirectConnection),
                "could not invoke product game loop");
    }
    require(!timer->isActive(), "unprotected game did not reach a result");
}

void checkEmbeddedEnemyRendering()
{
    GameRenderer renderer;
    const QVector<QPointF> path{{100, 100}, {1000, 100}};
    for (const auto& entry : {
             qMakePair(QString("bug"), QString(":/images/images/enemy_bug.png")),
             qMakePair(QString("ddl"), QString(":/images/images/enemy.ddl.gif")),
             qMakePair(QString("virus"), QString(":/images/images/enemy_virus.png")),
             qMakePair(QString("boss"), QString(":/images/images/enemy_boss.png"))}) {
        const int size = entry.first == "boss" ? 75 : 65;
        const QPixmap original(entry.second);
        require(!original.isNull(), "enemy image could not be decoded");
        const QPixmap icon = original.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QImage actual(200, 200, QImage::Format_ARGB32_Premultiplied);
        QImage expected = actual;
        actual.fill(Qt::transparent);
        expected.fill(Qt::transparent);
        Enemy enemy(entry.first, path);
        { QPainter p(&actual); renderer.drawEnemy(p, enemy); }
        const int start = 100 - size / 2;
        { QPainter p(&expected); p.drawPixmap(start, start, size, size, icon); }
        require(actual.copy(start, start, size, size) == expected.copy(start, start, size, size),
                "renderer did not preserve actual embedded enemy image");
    }
}

void checkEmbeddedTowerPortraits()
{
    GameRenderer renderer;
    const QPixmap menuBg(":/images/images/menu_bg.jpg");
    require(!menuBg.isNull(), "menu background image could not be decoded for tower reference");

    const QRect srcRects[] = {
        QRect(708, 452, 82, 84), // Normal: 海星
        QRect(809, 451, 95, 87), // Fast: 黄色炮台
        QRect(914, 448, 89, 91)  // Heavy: 红色炮台
    };

    int typeIndex = 0;
    for (TowerType type : {TowerType::Normal, TowerType::Fast, TowerType::Heavy}) {
        Tower tower(QPointF(200, 200), type);
        const int diameter = tower.bodyDiameter();
        const int half = diameter / 2;
        const QRectF body(200 - half, 200 - half, diameter, diameter);

        QImage actual(400, 400, QImage::Format_ARGB32_Premultiplied);
        actual.fill(Qt::transparent);
        {
            QPainter p(&actual);
            renderer.drawTower(p, tower, false);
        }

        // 1. 验证真实角色头像绘制（绝非字母回退塔，且检出映射错误）
        const QPixmap expectedSrc = menuBg.copy(srcRects[typeIndex]);
        QImage expectedPortrait(400, 400, QImage::Format_ARGB32_Premultiplied);
        expectedPortrait.fill(Qt::transparent);
        {
            QPainter p(&expectedPortrait);
            p.setRenderHint(QPainter::SmoothPixmapTransform, true);
            QPainterPath clip;
            clip.addRoundedRect(body, 6, 6);
            p.setClipPath(clip);
            p.drawPixmap(body, expectedSrc, QRectF(expectedSrc.rect()));
        }

        // 选取塔身内部上半部分（避开左下角徽章和外边框）比对像素
        const QRect sampleRegion = body.adjusted(6, 6, -6, -diameter / 2).toRect();
        int solidPixels = 0;
        int matchedPixels = 0;
        for (int y = sampleRegion.top(); y <= sampleRegion.bottom(); ++y) {
            for (int x = sampleRegion.left(); x <= sampleRegion.right(); ++x) {
                const QColor exp = expectedPortrait.pixelColor(x, y);
                if (exp.alpha() > 200) {
                    ++solidPixels;
                    const QColor act = actual.pixelColor(x, y);
                    if (std::abs(act.red() - exp.red()) <= 3 &&
                        std::abs(act.green() - exp.green()) <= 3 &&
                        std::abs(act.blue() - exp.blue()) <= 3) {
                        ++matchedPixels;
                    }
                }
            }
        }
        require(solidPixels > 30, "insufficient solid pixels in tower sample region");
        require(matchedPixels * 100 / solidPixels >= 85,
                "tower portrait does not match expected cropped artwork or is missing fallback");

        // 2. 验证左下角等级徽章（Badge）真实绘制（边框色及米黄底色，回退字母塔此处无徽章且越界处透明）
        // 徽章外缘点：(body.left() - 1, body.bottom() - 7)，在回退圆之外，只有徽章存在时才不透明
        const QPoint badgeOuter(static_cast<int>(body.left() - 1), static_cast<int>(body.bottom() - 7));
        require(actual.pixelColor(badgeOuter).alpha() > 180,
                "tower level badge outer rim is missing (fallback tower detected)");
        const QRectF badgeRect(body.left() - 1, body.bottom() - 15, 16, 16);
        const QPoint badgeCenter = badgeRect.center().toPoint();
        const QColor badgeColor = actual.pixelColor(badgeCenter);
        const bool isBadgeBg = (badgeColor.red() > 230 && badgeColor.green() > 220 && badgeColor.blue() > 190);
        const bool isBadgeText = (badgeColor.red() < 80 && badgeColor.green() < 80 && badgeColor.blue() < 120);
        require(isBadgeBg || isBadgeText, "tower level badge center color does not match expected badge fill");

        // 3. 验证升级后的尺寸变化与徽章位置动态位移
        const int beforeDiameter = tower.bodyDiameter();
        require(tower.upgrade(), "tower upgrade failed in visual test");
        require(tower.bodyDiameter() == beforeDiameter + 2, "upgraded tower diameter did not increase by 2");
        QImage upgradedActual(400, 400, QImage::Format_ARGB32_Premultiplied);
        upgradedActual.fill(Qt::transparent);
        {
            QPainter p(&upgradedActual);
            renderer.drawTower(p, tower, false);
        }
        const int upgradedHalf = tower.bodyDiameter() / 2;
        const QRectF upgradedBody(200 - upgradedHalf, 200 - upgradedHalf, tower.bodyDiameter(), tower.bodyDiameter());
        const QPoint upgradedBadgeOuter(static_cast<int>(upgradedBody.left() - 1), static_cast<int>(upgradedBody.bottom() - 7));
        require(upgradedActual.pixelColor(upgradedBadgeOuter).alpha() > 180,
                "upgraded tower level badge outer rim is missing");

        // 4. 验证建造预览（Build Preview）对所选塔型的原画匹配与 0.72 透明度
        QImage baseActual(1200, 800, QImage::Format_ARGB32_Premultiplied);
        {
            GameSession baseSession;
            GameUiState baseUi;
            baseUi.hasMousePosition = false;
            QPainter p(&baseActual);
            renderer.paint(p, baseSession, baseUi);
        }

        QImage previewActual(1200, 800, QImage::Format_ARGB32_Premultiplied);
        {
            GameSession previewSession;
            GameSessionProductTestAccess::startRunning(previewSession);
            GameUiState previewUi;
            previewUi.hasMousePosition = true;
            previewUi.mousePosition = QPointF(90, 750);
            previewUi.selectedTowerType = type;
            QPainter p(&previewActual);
            renderer.paint(p, previewSession, previewUi);
        }

        const QRectF previewBody(90 - half, 750 - half, diameter, diameter);
        const QRect previewSample = previewBody.adjusted(6, 6, -6, -diameter / 2).toRect();
        int previewSolid = 0;
        int previewMatched = 0;
        for (int y = previewSample.top(); y <= previewSample.bottom(); ++y) {
            for (int x = previewSample.left(); x <= previewSample.right(); ++x) {
                const int expX = x - 90 + 200;
                const int expY = y - 750 + 200;
                const QColor exp = expectedPortrait.pixelColor(expX, expY);
                if (exp.alpha() > 200) {
                    ++previewSolid;
                    const QColor act = previewActual.pixelColor(x, y);
                    const QColor base = baseActual.pixelColor(x, y);
                    const int expR = static_cast<int>(exp.red() * 0.72 + base.red() * 0.28 + 0.5);
                    const int expG = static_cast<int>(exp.green() * 0.72 + base.green() * 0.28 + 0.5);
                    const int expB = static_cast<int>(exp.blue() * 0.72 + base.blue() * 0.28 + 0.5);
                    if (std::abs(act.red() - expR) <= 4 &&
                        std::abs(act.green() - expG) <= 4 &&
                        std::abs(act.blue() - expB) <= 4) {
                        ++previewMatched;
                    }
                }
            }
        }
        require(previewSolid > 30, "insufficient solid pixels in preview sample region");
        require(previewMatched * 100 / previewSolid >= 80,
                "build preview does not match expected tower artwork or 0.72 opacity for selected type");

        ++typeIndex;
    }
}

} // namespace

int main(int argc, char* argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    for (const char* path : {
             ":/images/images/menu_bg.jpg", ":/images/images/help_bg.png",
             ":/images/images/game_map.jpg", ":/images/images/win.png",
             ":/images/images/lose.png", ":/config/attack_waves.json",
             ":/audio/music/menu.mp3",
             ":/audio/music/battle.mp3", ":/audio/music/final_wave.wav",
             ":/images/images/enemy_bug.png", ":/images/images/enemy.ddl.gif",
             ":/images/images/enemy_virus.png", ":/images/images/enemy_boss.png"}) {
        require(QResource(path).isValid(), "embedded product resource is missing");
    }
    require(!QPixmap(":/images/images/menu_bg.jpg").isNull(),
            "menu image could not be decoded");
    require(!QPixmap(":/images/images/game_map.jpg").isNull(),
            "game map image could not be decoded");

    checkEmbeddedEnemyRendering();
    checkEmbeddedTowerPortraits();
    StartMenuWidget independentMenu;
    int quitRequests = 0;
    QObject::connect(&independentMenu, &StartMenuWidget::quitRequested,
                     [&quitRequests] { ++quitRequests; });
    mousePress(&independentMenu, QPoint(369, 557));
    require(quitRequests == 0, "menu quit activated before release");
    mouseRelease(&independentMenu, QPoint(369, 557));
    require(quitRequests == 1, "menu did not emit application quit request");

    MainWindow window;
    window.show();
    waitForEvents(50);

    auto* stack = window.findChild<QStackedWidget*>();
    auto* menu = window.findChild<StartMenuWidget*>();
    require(stack && menu && stack->currentWidget() == menu,
            "initial menu is not active");
    require(window.findChild<GameWidget*>() == nullptr,
            "game was created before the first start click");

    mouseMove(menu, QPoint(369, 386));
    require(menu->cursor().shape() == Qt::PointingHandCursor,
            "start button hover was not recognized");
    mouseMove(menu, QPoint(300, 335));
    require(menu->cursor().shape() == Qt::ArrowCursor,
            "blank area hover was treated as a button");

    const QImage menuImage = menu->grab().toImage();
    const QRect rosterRect(695, 212, 315, 190);
    const QPixmap rawBg(":/images/images/menu_bg.jpg");
    require(menuImage.copy(rosterRect) != rawBg.copy(rosterRect).toImage(),
            "enemy roster was not drawn over menu background");

    // 严密核对菜单四个敌人图标：确保图标已真实渲染，绝非空白卡片或贴错敌人
    const QString enemyQrcs[] = {
        ":/images/images/enemy_bug.png",
        ":/images/images/enemy.ddl.gif",
        ":/images/images/enemy_virus.png",
        ":/images/images/enemy_boss.png"
    };
    for (int i = 0; i < 4; ++i) {
        const QRect card(703 + i * 76, 255, 70, 111);
        const QRect iconArea(card.left() + 4, card.top() + 4, 62, 68);
        const QPixmap rawIcon(enemyQrcs[i]);
        require(!rawIcon.isNull(), "enemy qrc resource could not be loaded for roster verification");
        const QPixmap thumb = rawIcon.scaled(iconArea.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const QImage thumbImg = thumb.toImage();
        const int drawX = iconArea.center().x() - thumb.width() / 2;
        const int drawY = iconArea.center().y() - thumb.height() / 2;

        int solidPixels = 0;
        int matchedPixels = 0;
        for (int y = 0; y < thumbImg.height(); y += 2) {
            for (int x = 0; x < thumbImg.width(); x += 2) {
                const QColor exp = thumbImg.pixelColor(x, y);
                if (exp.alpha() > 220) {
                    ++solidPixels;
                    const QColor act = menuImage.pixelColor(drawX + x, drawY + y);
                    if (std::abs(act.red() - exp.red()) <= 3 &&
                        std::abs(act.green() - exp.green()) <= 3 &&
                        std::abs(act.blue() - exp.blue()) <= 3) {
                        ++matchedPixels;
                    }
                }
            }
        }
        require(solidPixels > 40, "insufficient solid pixels in enemy thumbnail fixture");
        require(matchedPixels * 100 / solidPixels >= 80,
                "menu enemy icon does not match expected artwork or is missing (blank card detected)");
    }
    mouseMove(menu, QPoint(369, 386));
    const QImage startHover = menu->grab().toImage();
    mousePress(menu, QPoint(369, 386));
    require(stack->currentWidget() == menu && window.findChild<GameWidget*>() == nullptr,
            "start activated before button release");
    const QImage startPressed = menu->grab().toImage();
    require(startPressed != startHover, "start press has no visible feedback");
    mouseMove(menu, QPoint(300, 335), Qt::LeftButton);
    mouseRelease(menu, QPoint(300, 335));
    require(stack->currentWidget() == menu && window.findChild<GameWidget*>() == nullptr,
            "dragging out of start still activated it");
    require(menu->grab().toImage() == menuImage,
            "cancelled start press left a visual state behind");

    mouseClick(menu, QPoint(369, 477));
    require(menu->grab().toImage() != menuImage,
            "help click did not change the displayed page");
    mouseMove(menu, QPoint(369, 730));
    const QImage backHover = menu->grab().toImage();
    mousePress(menu, QPoint(369, 730));
    require(menu->grab().toImage() != backHover,
            "help back press has no visible feedback");
    mouseRelease(menu, QPoint(369, 730));
    require(menu->grab().toImage() == menuImage,
            "help back button did not restore menu");
    mouseClick(menu, QPoint(369, 477));
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QCoreApplication::sendEvent(menu, &escape);
    require(menu->grab().toImage() == menuImage,
            "Escape did not return from help to menu");

    mouseClick(menu, QPoint(369, 386));
    auto* game = window.findChild<GameWidget*>();
    require(game && stack->currentWidget() == game,
            "start click did not show the game");
    auto* timer = game->findChild<QTimer*>();
    require(timer && timer->isActive(), "first game timer is not active");

    timer->stop();
    const auto bottom = game->evaluateBuild(QPointF(90, 799));
    require(bottom.valid() && bottom.center == QPointF(90, 750),
            "bottom click did not map to the last visible grid row");
    require(game->evaluateBuild(QPointF(90, 59)).problem
                == GameWidget::BuildProblem::OutsideMap,
            "HUD position was accepted for building");
    require(game->evaluateBuild(QPointF(90, 230)).problem
                == GameWidget::BuildProblem::Road,
            "road position was accepted for building");

    const QRect panelRect(840, 70, 330, 145);
    require(game->evaluateBuild(QPointF(900, 110)).valid(),
            "cell under event panel must be legal when panel is closed");
    const QImage panelClosed = game->grab().toImage();
    mouseClick(game, QPoint(916, 36));
    const QImage panelOpened = game->grab().toImage();
    require(panelOpened.copy(panelRect) != panelClosed.copy(panelRect),
            "event preview panel did not appear in designated area");
    mouseClick(game, QPoint(900, 110));
    require(game->towerCount() == 0 && game->gold() == 150,
            "event panel did not block building on covered legal map cell when gold is abundant");
    mouseClick(game, QPoint(916, 36));
    require(game->grab().toImage().copy(panelRect) == panelClosed.copy(panelRect),
            "event preview panel did not close cleanly");

    const QImage noPreview = game->grab().toImage();
    mouseMove(game, QPoint(90, 799));
    const QImage validPreview = game->grab().toImage();
    require(validPreview != noPreview, "valid build preview was not drawn");
    // 验证建造预览中在网格吸附中心 (90, 750) 绘制了同型塔（普通塔/海星）半透明头像
    const QColor baseMapPixel = noPreview.pixelColor(90, 750);
    const QColor previewPixel = validPreview.pixelColor(90, 750);
    require(previewPixel != baseMapPixel, "build preview center did not render tower portrait");

    // 验证通过按键 1/2/3 切换塔型时，建造预览准确呈现三种不同塔的原画且各不相同（杜绝写死单一塔型预览）
    keyPress(game, Qt::Key_2);
    const QImage fastPreview = game->grab().toImage();
    require(fastPreview != validPreview,
            "fast tower preview was identical to normal tower preview (preview tower type ignored)");
    keyPress(game, Qt::Key_3);
    const QImage heavyPreview = game->grab().toImage();
    require(heavyPreview != fastPreview && heavyPreview != validPreview,
            "heavy tower preview did not differ from fast or normal tower preview");
    keyPress(game, Qt::Key_1);
    const QImage restoredNormalPreview = game->grab().toImage();
    require(restoredNormalPreview == validPreview,
            "restored normal tower preview did not match initial normal preview");
    mouseMove(game, QPoint(90, 230));
    const QImage roadPreview = game->grab().toImage();
    require(roadPreview != noPreview && roadPreview != validPreview,
            "invalid road preview was not drawn");
    mouseMove(game, QPoint(90, 799));
    mouseClick(game, QPoint(90, 799));
    require(game->towerCount() == 1 && game->gold() == 100
                && GameWidgetProductTestAccess::firstTowerPosition(game) == QPointF(90, 750),
            "bottom build did not create one visible tower at the preview center");
    mouseClick(game, QPoint(90, 230));
    require(game->towerCount() == 1 && game->gold() == 100,
            "road rejection changed towers or gold");
    mouseClick(game, QPoint(90, 799));
    require(game->towerCount() == 1 && game->gold() == 100,
            "occupied bottom cell changed towers or gold");
    mouseClick(game, QPoint(150, 750));
    require(game->towerCount() == 2 && game->gold() == 50,
            "adjacent legal grid cell was rejected");
    mouseClick(game, QPoint(210, 750));
    require(game->towerCount() == 3 && game->gold() == 0,
            "third legal tower was rejected");
    require(game->evaluateBuild(QPointF(270, 750)).problem
                == GameWidget::BuildProblem::InsufficientGold,
            "insufficient gold was not reflected in build preview");
    mouseMove(game, QPoint(270, 750));
    const QImage noGoldPreview = game->grab().toImage();
    mouseClick(game, QPoint(270, 750));
    require(game->towerCount() == 3 && game->gold() == 0,
            "insufficient-gold build changed towers or gold");

    const QImage beforePanel2 = game->grab().toImage();
    mouseClick(game, QPoint(916, 36));
    const QImage eventPanel = game->grab().toImage();
    require(eventPanel.copy(panelRect) != beforePanel2.copy(panelRect),
            "event preview panel did not appear in designated area on toggle");
    mouseClick(game, QPoint(900, 110));
    require(game->towerCount() == 3, "event panel click built a tower");
    mouseClick(game, QPoint(916, 36));
    timer->start(30);

    mouseMove(game, QPoint(1016, 36));
    const QImage pauseHover = game->grab().toImage();
    mousePress(game, QPoint(1016, 36));
    require(game->grab().toImage() != pauseHover,
            "pause button press has no visible feedback");
    mouseRelease(game, QPoint(1016, 36));
    require(game->isPaused() && !timer->isActive(),
            "pause did not stop the game timer");
    const qint64 pausedTick = game->gameTick();
    waitForEvents(80);
    require(QMetaObject::invokeMethod(game, "updateGame", Qt::DirectConnection)
                && game->gameTick() == pausedTick,
            "paused game advanced through direct or timer update");
    mouseClick(game, QPoint(330, 750));
    require(game->towerCount() == 3 && game->gold() == 0,
            "paused game accepted a build action");
    mouseClick(game, QPoint(90, 750));
    require(!GameWidgetProductTestAccess::hasSelectedTower(game),
            "paused game selected an existing tower");
    QKeyEvent chooseFast(QEvent::KeyPress, Qt::Key_2, Qt::NoModifier);
    QCoreApplication::sendEvent(game, &chooseFast);
    require(GameWidgetProductTestAccess::selectedType(game) == TowerType::Normal,
            "paused game changed tower type");

    if (argc > 1) {
        const QDir output(argv[1]);
        require(validPreview.save(output.filePath("placement_preview.png")),
                "placement preview screenshot could not be saved");
        require(roadPreview.save(output.filePath("road_preview.png")),
                "road preview screenshot could not be saved");
        require(noGoldPreview.save(output.filePath("no_gold_preview.png")),
                "no-gold preview screenshot could not be saved");
        require(eventPanel.save(output.filePath("event_preview.png")),
                "event preview screenshot could not be saved");
        require(window.grab().save(output.filePath("paused.png")),
                "paused screenshot could not be saved");
        require(menuImage.save(output.filePath("menu.png")),
                "menu screenshot could not be saved");
        require(startPressed.save(output.filePath("menu_pressed.png")),
                "menu pressed screenshot could not be saved");
    }

    QKeyEvent resumeKey(QEvent::KeyPress, Qt::Key_P, Qt::NoModifier);
    QCoreApplication::sendEvent(game, &resumeKey);
    require(!game->isPaused() && timer->isActive(),
            "P key did not resume the paused game");
    QKeyEvent repeatedPause(QEvent::KeyPress, Qt::Key_P, Qt::NoModifier, QString(),
                            true, 1);
    QCoreApplication::sendEvent(game, &repeatedPause);
    require(!game->isPaused(), "auto-repeat P toggled pause");
    mouseClick(game, QPoint(1016, 36));
    require(game->isPaused() && !timer->isActive(),
            "pause button did not pause after keyboard resume");
    mouseClick(game, QPoint(1016, 36));
    require(!game->isPaused() && timer->isActive(),
            "continue button did not resume the game");
    mouseClick(game, QPoint(1016, 36));
    require(game->isPaused(), "pause button did not pause before exit");

    mouseClick(game, QPoint(1116, 36));
    require(stack->currentWidget() == menu && !timer->isActive(),
            "paused exit did not return to menu and stop timer");
    mouseClick(menu, QPoint(369, 386));
    require(stack->currentWidget() == game && timer->isActive()
                && !game->isPaused() && game->towerCount() == 0 && game->gold() == 150,
            "new game did not clear paused state and previous towers");

    advanceToResult(game, timer);
    require(!GameWidgetProductTestAccess::won(game),
            "unprotected game did not reach a loss result");
    mouseMove(game, QPoint(750, 600));
    const QImage lossHover = game->grab().toImage();
    mousePress(game, QPoint(750, 600));
    const QImage lossPressed = game->grab().toImage();
    require(stack->currentWidget() == game && lossPressed != lossHover,
            "loss menu press did not show feedback before release");
    if (argc > 1) {
        require(lossPressed.save(QDir(argv[1]).filePath("loss_pressed.png")),
                "loss pressed screenshot could not be saved");
    }
    mouseRelease(game, QPoint(750, 600));
    require(stack->currentWidget() == menu && !timer->isActive(),
            "result-to-menu did not stop the game timer");
    waitForEvents(80);
    require(!timer->isActive(), "hidden game timer restarted in menu");

    mouseClick(menu, QPoint(369, 386));
    require(stack->currentWidget() == game && timer->isActive(),
            "menu start did not reuse and restart the game");
    require(window.findChildren<GameWidget*>().size() == 1,
            "menu start created a second game instance");

    advanceToResult(game, timer);
    mouseClick(game, QPoint(400, 600));
    require(stack->currentWidget() == game && timer->isActive(),
            "result retry did not reset and restart the game");

    GameWidgetProductTestAccess::grantLives(game);
    advanceToResult(game, timer);
    require(GameWidgetProductTestAccess::won(game),
            "high-life game did not reach a win result");
    mouseMove(game, QPoint(400, 510));
    const QImage winHover = game->grab().toImage();
    mousePress(game, QPoint(400, 510));
    const QImage winPressed = game->grab().toImage();
    require(winPressed != winHover,
            "win retry press has no visible feedback");
    if (argc > 1) {
        require(winPressed.save(QDir(argv[1]).filePath("win_pressed.png")),
                "win pressed screenshot could not be saved");
    }
    mouseRelease(game, QPoint(400, 510));
    require(timer->isActive(), "win retry did not restart game");

    GameWidgetProductTestAccess::grantLives(game);
    advanceToResult(game, timer);
    mouseClick(game, QPoint(750, 510));
    require(stack->currentWidget() == menu && !timer->isActive(),
            "win menu button did not return to menu");

    std::cout << "PASS: Qt6 product buttons, pause, placement, event preview and lifecycle\n";
    return 0;
}
