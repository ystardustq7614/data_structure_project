/** @file
 * @brief 页面和音频的应用级协调；实体、战斗数值与逐帧时钟留在游戏页及核心层。 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
class QStackedWidget;
class QMediaPlayer;
class QAudioOutput;
class QSoundEffect;
class GameWidget;
class StartMenuWidget;
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/** 以 Qt 父子关系持有菜单、懒创建的游戏页和音频对象；游戏页在多局之间复用。 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    /** 在 parent 下建立菜单和音频；创建后进入菜单，首次开始才创建游戏页面。 */
    explicit MainWindow(QWidget* parent = nullptr);
    /** 释放生成的 Ui 包装；页面/音频对象由 QObject 父子关系统一释放。 */
    ~MainWindow() override;
private:
    void initializeAudio();
    void ensureGameWidget();
    void startGame();
    void showMenu();
    void playLoopMusic(const QString& path);
    void handleFinalWave();
    void handlePauseChanged(bool paused);
    Ui::MainWindow* ui;
    QStackedWidget* m_stack = nullptr;
    StartMenuWidget* m_menu = nullptr;
    GameWidget* m_game = nullptr;
    QMediaPlayer* m_musicPlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    QSoundEffect* m_finalWaveEffect = nullptr;
};
#endif
