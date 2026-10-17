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

// 应用页面与音频协调。游戏页懒创建并复用，不介入实体或数值规则。
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
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
