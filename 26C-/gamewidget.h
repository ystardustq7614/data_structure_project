#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H
#include <QWidget>
#include <QTimer>
#include <QVector>
#include <QPointF>
#include <QPainter>
#include <QPixmap>
#include "enemy.h"
#include <QMouseEvent>
#include "tower.h"
#include <QString>
#include "bullet.h"
#include <QKeyEvent>
class GameWidget:public QWidget
{Q_OBJECT
public:
    explicit GameWidget(QWidget *parent = nullptr);   // 构造函数
    ~GameWidget();                                    // 析构函数
    void restartGame(bool startTimer = true);   // 重新开始游戏
signals:
    void finalWaveStarted();
    void backToMenuRequested();   // 请求返回主菜单
protected:
    void paintEvent(QPaintEvent *event) override;     // 绘图函数
    void mousePressEvent(QMouseEvent *event) override;  // 鼠标点击事件
void keyPressEvent(QKeyEvent *event) override;
private slots:
    void updateGame();                                // 游戏更新函数

private:
    QTimer* m_timer = nullptr;        // 游戏循环定时器

   QVector<QVector<QPointF>> m_paths; // 多条敌人路线
    QVector<Enemy*> m_enemies;        // 敌人数组
   QPixmap m_gameMapBackground;     // 主游戏地图背景图
   bool m_debugDrawPath =false;    // 调试用：是否显示原来的代码路径

    int m_gold = 150;                 // 金币
    int m_life = 10;                  // 生命
    int m_wave = 1;                   // 波次

    int m_spawnCounter = 0;           // 出怪计数器
    int m_spawnedCount = 0;           // 已生成敌人数
    int m_totalEnemies = 20;           // 总敌人数
   QVector<QVector<QString>> m_waves;  // 多波敌人，每一波是一个 QString 队列
   QVector<Bullet*> m_bullets;      // 当前地图上的所有子弹
    bool m_gameFinished = false;     // 游戏是否已经结束
   bool m_gameWon = false;       // true 表示胜利，false 表示失败
   QPixmap m_winImage;           // 胜利界面图片
   QPixmap m_loseImage;          // 失败界面图片
   bool m_bossAuraActive = false;       // Boss是否正在强化全场敌人
   bool m_bossAuraTriggered = false;    // Boss光环是否已经触发过

   QString m_statusMessage;             // 屏幕字幕
   int m_statusMessageCounter = 0;      // 字幕剩余显示帧数
   int m_currentWaveIndex = 0;     // 当前第几波，0 表示第一波
   int m_spawnIndexInWave = 0;     // 当前波已经生成到第几个敌人
   int m_spawnInterval = 14;       // 出怪间隔，数值越小，出怪越快
int m_nextPathIndex = 0;        // 下一个敌人从哪条路出来
   int m_waveWaitCounter = 0;      // 波次间隔计数器
   int m_waveWaitTime =20;        // 每波之间等待 20 帧
    QVector<Tower*> m_towers;        // 当前地图上的所有防御塔
   Tower* m_selectedTower = nullptr;   // 当前选中的防御塔
   TowerType m_selectedTowerType = TowerType::Normal;
private:
    void initPaths();                  // 初始化路径
    void initSpawnQueue();              // 初始化出怪队列
    void spawnEnemy(const QString& type); // 按类型生成敌人
    void drawBackground(QPainter& painter);  // 画背景
    void drawGrid(QPainter& painter);        // 画网格
    void drawPath(QPainter& painter);        // 画道路
    void drawHud(QPainter& painter);         // 画状态栏
    void drawMapMarkers(QPainter& painter);   // 绘制起点和终点标记
    QPointF snapToGrid(const QPointF& pos) const;       // 把点击位置吸附到格子中心
    bool canBuildTowerAt(const QPointF& pos) const;     // 判断这个位置能不能建塔
    void checkGameResult();           // 检查胜利或失败
    void removeBulletsTargeting(Enemy* enemy);  // 删除所有瞄准某个敌人的子弹
    Tower* towerAt(const QPointF& pos) const;   // 查找鼠标点击位置是否有塔
    void clearSelectedTower();                  // 取消选中塔
    void upgradeSelectedTower();                // 升级选中塔
    void sellSelectedTower();                   // 出售选中塔
    int m_burstRemaining = 0;      // 当前小队还剩几个怪没出
    int m_burstGapCounter = 0;     // 小队内部间隔计数器
    int m_burstGap = 9;            // 同一小队内每只怪间隔 9 帧，避免粘一起
    void applyBossAuraToAllEnemies();          // 给当前场上敌人加Boss光环
    void removeBossAuraFromAllEnemies();       // 撤销当前场上敌人的Boss光环
    void showStatusMessage(const QString& text, int frames = 120); // 显示字幕
    void drawStatusMessage(QPainter& painter); // 绘制字幕

    void drawGameResultScreen(QPainter& painter);  // 绘制胜利/失败整屏图片
    QRect resultRetryRect() const;                 // 重新挑战按钮范围
    QRect resultMenuRect() const;                  // 返回主页按钮范围
};

#endif // GAMEWIDGET_H
