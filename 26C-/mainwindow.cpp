#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "startmenuwidget.h"
#include "gamewidget.h"

#include <QStackedWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QSoundEffect>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QMediaPlayer* musicPlayer = new QMediaPlayer(this);
    QAudioOutput* audioOutput = new QAudioOutput(this);

    musicPlayer->setAudioOutput(audioOutput);
    audioOutput->setVolume(0.35);
    QSoundEffect* allAttackEffect = new QSoundEffect(this);
    allAttackEffect->setSource(QUrl("qrc:/audio/music/final_wave.wav"));
    allAttackEffect->setLoopCount(1);
    allAttackEffect->setVolume(0.8);

    auto playLoopMusic = [musicPlayer](const QString& path) {
        musicPlayer->stop();
        musicPlayer->setSource(QUrl(path));
        musicPlayer->setLoops(QMediaPlayer::Infinite);
        musicPlayer->play();
    };

    // 程序启动后，先播放进入界面音乐
    playLoopMusic("qrc:/audio/music/menu.mp3");

    // 隐藏 ui 文件自带的菜单栏和状态栏，避免影响 1200×800 游戏画面
    if (ui->menubar) {
        ui->menubar->hide();
    }

    if (ui->statusbar) {
        ui->statusbar->hide();
    }

    // 用 QStackedWidget 管理两个页面：
    // 第 0 页：开始界面
    // 第 1 页：游戏界面
    QStackedWidget* stackWidget = new QStackedWidget(this);
    stackWidget->setFixedSize(1200, 800);

    StartMenuWidget* menuWidget = new StartMenuWidget(stackWidget);
    stackWidget->addWidget(menuWidget);

    setCentralWidget(stackWidget);

    // 这里先不创建 GameWidget
    // 只有点击“开始游戏”之后才创建，避免游戏在菜单背后偷偷运行
    GameWidget* gameWidget = nullptr;

    connect(menuWidget, &StartMenuWidget::startGameClicked,
            this, [this, stackWidget, menuWidget, gameWidget, playLoopMusic, allAttackEffect]() mutable {
                if (gameWidget == nullptr) {
                    gameWidget = new GameWidget(stackWidget);
                    stackWidget->addWidget(gameWidget);

                    connect(gameWidget, &GameWidget::backToMenuRequested,
                            this, [stackWidget, menuWidget, gameWidget, playLoopMusic]() {
                                gameWidget->restartGame(false);
                                stackWidget->setCurrentWidget(menuWidget);
                                menuWidget->setFocus();

                                playLoopMusic("qrc:/audio/music/menu.mp3");
                            });

                    connect(gameWidget, &GameWidget::finalWaveStarted,
                            this, [playLoopMusic, allAttackEffect]() {
                                playLoopMusic("qrc:/audio/music/battle.mp3");

                                allAttackEffect->stop();
                                allAttackEffect->play();
                            });
                } else {
                    gameWidget->restartGame(true);
                }

                stackWidget->setCurrentWidget(gameWidget);
                gameWidget->setFocus();

                playLoopMusic("qrc:/audio/music/battle.mp3");
            });

    setWindowTitle("顶流争夺保卫战");
    setFixedSize(1200, 800);
}

MainWindow::~MainWindow()
{
    delete ui;
}
