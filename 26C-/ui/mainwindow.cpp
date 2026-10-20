/** @file
 * @brief 页面/音频信号接线；复用游戏页时显式重置和停表，不依赖 QWidget 的可见性。 */
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gamewidget.h"
#include "startmenuwidget.h"
#include <QApplication>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QSoundEffect>
#include <QStackedWidget>
#include <QUrl>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initializeAudio();
    playLoopMusic("qrc:/audio/music/menu.mp3");
    if (ui->menubar) ui->menubar->hide();
    if (ui->statusbar) ui->statusbar->hide();
    m_stack = new QStackedWidget(this);
    m_stack->setFixedSize(GameMap::Width, GameMap::Height);
    m_menu = new StartMenuWidget(m_stack);
    m_stack->addWidget(m_menu);
    setCentralWidget(m_stack);
    connect(m_menu, &StartMenuWidget::startGameClicked, this, &MainWindow::startGame);
    // 菜单退出终止应用，局内退出则只回菜单，两种意图必须接到不同入口。
    connect(m_menu, &StartMenuWidget::quitRequested, qApp, &QApplication::quit);
    setWindowTitle("顶流争夺保卫战");
    setFixedSize(GameMap::Width, GameMap::Height);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::initializeAudio()
{
    // BGM 使用可暂停/循环的 QMediaPlayer，最终波短音效使用独立 QSoundEffect 一次播放。
    m_musicPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_musicPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.35);
    m_finalWaveEffect = new QSoundEffect(this);
    m_finalWaveEffect->setSource(QUrl("qrc:/audio/music/final_wave.wav"));
    m_finalWaveEffect->setLoopCount(1);
    m_finalWaveEffect->setVolume(0.8);
}

void MainWindow::playLoopMusic(const QString& path)
{
    m_musicPlayer->stop();
    m_musicPlayer->setSource(QUrl(path));
    m_musicPlayer->setLoops(QMediaPlayer::Infinite);
    m_musicPlayer->play();
}

void MainWindow::ensureGameWidget()
{
    if (m_game) return;
    m_game = new GameWidget(m_stack);
    m_stack->addWidget(m_game);
    connect(m_game, &GameWidget::backToMenuRequested, this, &MainWindow::showMenu);
    connect(m_game, &GameWidget::finalWaveStarted, this, &MainWindow::handleFinalWave);
    connect(m_game, &GameWidget::pauseChanged, this, &MainWindow::handlePauseChanged);
}

void MainWindow::startGame()
{
    // 构造游戏页已开启首局；复用实例必须重新读配置并清除上一局的塔、暂停与结算状态。
    if (m_game) m_game->restartGame(true);
    else ensureGameWidget();
    m_stack->setCurrentWidget(m_game);
    m_game->setFocus();
    playLoopMusic("qrc:/audio/music/battle.mp3");
}

void MainWindow::showMenu()
{
    // 隐藏 QWidget 不会停止其 QTimer；必须先重置为 Stopped，再切换页面。
    m_game->restartGame(false);
    m_stack->setCurrentWidget(m_menu);
    m_menu->setFocus();
    playLoopMusic("qrc:/audio/music/menu.mp3");
}

void MainWindow::handleFinalWave()
{
    // 最终波信号重新开始战斗 BGM，并播放一次独立 WAV；不是切换到 final_wave.mp3。
    playLoopMusic("qrc:/audio/music/battle.mp3");
    m_finalWaveEffect->stop();
    m_finalWaveEffect->play();
}

void MainWindow::handlePauseChanged(bool paused)
{
    if (paused) m_musicPlayer->pause();
    else m_musicPlayer->play();
}
