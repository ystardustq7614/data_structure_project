#include "../26C-/gamewidget.h"
#include "../26C-/mainwindow.h"
#include "../26C-/startmenuwidget.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QImage>
#include <QKeyEvent>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPixmap>
#include <QResource>
#include <QStackedWidget>
#include <QTimer>

#include <cstdlib>
#include <iostream>

class GameWidgetProductTestAccess
{
public:
    static QPointF firstTowerPosition(const GameWidget* game)
    {
        return game->m_towers.first()->position();
    }
    static void grantLives(GameWidget* game) { game->m_life = 10000; }
    static bool won(const GameWidget* game) { return game->m_gameWon; }
    static TowerType selectedType(const GameWidget* game)
    {
        return game->m_selectedTowerType;
    }
    static bool hasSelectedTower(const GameWidget* game)
    {
        return game->m_selectedTower != nullptr;
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

void advanceToResult(GameWidget* game, QTimer* timer)
{
    for (int frame = 0; frame < 12000 && timer->isActive(); ++frame) {
        require(QMetaObject::invokeMethod(game, "updateGame", Qt::DirectConnection),
                "could not invoke product game loop");
    }
    require(!timer->isActive(), "unprotected game did not reach a result");
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
             ":/images/images/enemy_bug.png", ":/images/images/enemy.ddl.gif",
             ":/images/images/enemy_virus.png", ":/images/images/enemy_boss.png",
             ":/audio/music/menu.mp3",
             ":/audio/music/battle.mp3", ":/audio/music/final_wave.wav"}) {
        require(QResource(path).isValid(), "embedded product resource is missing");
    }
    require(!QPixmap(":/images/images/menu_bg.jpg").isNull(),
            "menu image could not be decoded");
    require(!QPixmap(":/images/images/game_map.jpg").isNull(),
            "game map image could not be decoded");

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
    const QImage noPreview = game->grab().toImage();
    mouseMove(game, QPoint(90, 799));
    const QImage validPreview = game->grab().toImage();
    require(validPreview != noPreview, "valid build preview was not drawn");
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

    mouseClick(game, QPoint(916, 36));
    const QImage eventPanel = game->grab().toImage();
    require(eventPanel != validPreview, "event preview panel did not appear");
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
