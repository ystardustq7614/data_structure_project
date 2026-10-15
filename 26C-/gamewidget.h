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
#include "attackscheduler.h"
#include <QKeyEvent>
#include <QFocusEvent>
class GameWidget:public QWidget
{Q_OBJECT
public:
    explicit GameWidget(QWidget *parent = nullptr);   // 构造函数
    ~GameWidget();                                    // 析构函数
    void restartGame(bool startTimer = true);   // 重置并按需启动游戏
    enum class BuildProblem { None, OutsideMap, Road, Occupied, InsufficientGold };
    struct BuildEvaluation {
        QPointF center;
        BuildProblem problem = BuildProblem::OutsideMap;
        bool valid() const { return problem == BuildProblem::None; }
    };
    BuildEvaluation evaluateBuild(const QPointF& rawPosition) const;
    int gold() const { return m_gold; }
    int towerCount() const { return m_towers.size(); }
    bool isPaused() const { return m_state == GameState::Paused; }
    qint64 gameTick() const { return m_scheduler.currentTick(); }
signals:
    void finalWaveStarted();
    void backToMenuRequested();   // 请求返回主菜单
    void pauseChanged(bool paused);
protected:
    void paintEvent(QPaintEvent *event) override;     // 绘图函数
    void mousePressEvent(QMouseEvent *event) override;  // 鼠标点击事件
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
private slots:
    void updateGame();                                // 游戏更新函数

private:
    friend class GameWidgetProductTestAccess;
    QTimer* m_timer = nullptr;        // 游戏循环定时器

   QVector<QVector<QPointF>> m_paths; // 多条敌人路线
    QVector<Enemy*> m_enemies;        // 敌人数组
   QPixmap m_gameMapBackground;     // 主游戏地图背景图
   bool m_debugDrawPath =false;    // 调试用：是否显示原来的代码路径

    int m_gold = 150;                 // 金币
    int m_life = 10;                  // 生命
    enum class GameState { Running, Paused, Finished, Stopped };
    GameState m_state = GameState::Stopped;
    AttackScheduler m_scheduler;
    QString m_configurationError;
    QString m_configurationWarning;
   QVector<Bullet*> m_bullets;      // 当前地图上的所有子弹
    bool m_gameFinished = false;     // 游戏是否已经结束
   bool m_gameWon = false;       // true 表示胜利，false 表示失败
   QPixmap m_winImage;           // 胜利界面图片
   QPixmap m_loseImage;          // 失败界面图片
   bool m_bossAuraActive = false;       // Boss是否正在强化全场敌人
   bool m_bossAuraTriggered = false;    // Boss光环是否已经触发过

   QString m_statusMessage;             // 屏幕字幕
   int m_statusMessageCounter = 0;      // 字幕剩余显示帧数
int m_nextPathIndex = 0;        // 下一个敌人从哪条路出来
    QVector<Tower*> m_towers;        // 当前地图上的所有防御塔
   Tower* m_selectedTower = nullptr;   // 当前选中的防御塔
   TowerType m_selectedTowerType = TowerType::Normal;
private:
    void initPaths();                  // 初始化路径
    void spawnEnemy(const QString& type, int waveIndex); // 按类型生成敌人
    void drawBackground(QPainter& painter);  // 画背景
    void drawGrid(QPainter& painter);        // 画网格
    void drawPath(QPainter& painter);        // 画道路
    void drawHud(QPainter& painter);         // 画状态栏
    void drawMapMarkers(QPainter& painter);   // 绘制起点和终点标记
    QPointF snapToGrid(const QPointF& pos) const;       // 把点击位置吸附到格子中心
    void checkGameResult();           // 检查胜利或失败
    void removeBulletsTargeting(Enemy* enemy);  // 删除所有瞄准某个敌人的子弹
    Tower* towerAt(const QPointF& pos) const;   // 查找鼠标点击位置是否有塔
    void clearSelectedTower();                  // 取消选中塔
    void upgradeSelectedTower();                // 升级选中塔
    void sellSelectedTower();                   // 出售选中塔
    void applyBossAuraToAllEnemies();          // 给当前场上敌人加Boss光环
    void removeBossAuraFromAllEnemies();       // Boss离场后撤销光环
    void showStatusMessage(const QString& text, int frames = 120); // 显示字幕
    void drawStatusMessage(QPainter& painter); // 绘制字幕

    void drawGameResultScreen(QPainter& painter);  // 绘制胜利/失败整屏图片
    QRect resultRetryRect() const;                 // 重新挑战按钮范围
    QRect resultMenuRect() const;                  // 返回主页按钮范围
    enum class Control { None, Preview, Pause, Exit, Retry, Menu };
    Control controlAt(const QPoint& pos) const;
    QRect controlRect(Control control) const;
    QRect previewPanelRect() const;
    void setPaused(bool paused);
    void drawButtonFeedback(QPainter& painter, const QRect& rect,
                            bool hovered, bool pressed, bool selected = false) const;
    void drawBuildPreview(QPainter& painter);
    void drawEventPreview(QPainter& painter);
    void drawPauseOverlay(QPainter& painter);
    void resetInteraction();
    Control m_hoveredControl = Control::None;
    Control m_pressedControl = Control::None;
    QPointF m_mousePosition;
    bool m_hasMousePosition = false;
    bool m_previewVisible = false;
};

#endif // GAMEWIDGET_H
