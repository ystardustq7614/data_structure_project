#ifndef GAMESESSION_H
#define GAMESESSION_H

#include "attackplan.h"
#include "attackscheduler.h"
#include "bullet.h"
#include "enemy.h"
#include "gamemap.h"
#include "tower.h"

#include <QString>
#include <QVector>

// 一局游戏的唯一规则与实体持有者。调用者决定何时推进，不访问界面或文件。
class GameSession
{
public:
    enum class State { Stopped, Running, Paused, Won, Lost };
    enum class BuildProblem { None, OutsideMap, Road, Occupied, InsufficientGold };
    struct BuildEvaluation {
        QPointF center;
        BuildProblem problem = BuildProblem::OutsideMap;
        bool valid() const { return problem == BuildProblem::None; }
    };
    struct Notice {
        QString text;
        int frames = 120;
    };
    struct ActionResult {
        QString message;
        int frames = 60;
        bool success = false;
    };
    struct FrameResult {
        bool advanced = false;
        bool finalWaveStarted = false;
        QVector<Notice> notices;
        bool stopped = false;
    };

    GameSession() = default;
    ~GameSession();
    GameSession(const GameSession&) = delete;
    GameSession& operator=(const GameSession&) = delete;

    // 清空上一局后应用有效计划；running=false 重置但保持停止，供返回菜单使用。
    void reset(const AttackPlan& plan, bool running = true);
    void clear(); // 子弹先于敌人销毁，恢复初始金币/生命并进入 Stopped。
    bool setPaused(bool paused); // 只切换 Running/Paused，返回是否真正切换。
    // 仅 Running 推进一步；按旧阶段顺序模拟，通知顺序保留但不操作界面。
    FrameResult step();

    // 原始坐标先校验再吸附；仅判定规则，控件遮挡由界面在调用前拦截。
    BuildEvaluation evaluateBuild(const QPointF& position, TowerType type) const;
    // 交易失败不改变金币和对象；成功创建的对象由本战局独占。
    ActionResult build(const QPointF& position, TowerType type);
    ActionResult upgradeSelectedTower();
    ActionResult sellSelectedTower();
    bool selectTowerAt(const QPointF& position);
    void clearSelectedTower();

    State state() const { return m_state; }
    bool isFinished() const { return m_state == State::Won || m_state == State::Lost; }
    bool isWon() const { return m_state == State::Won; }
    bool isPaused() const { return m_state == State::Paused; }
    int gold() const { return m_gold; }
    int life() const { return m_life; }
    qint64 gameTick() const { return m_scheduler.currentTick(); }
    const GameMap& map() const { return m_map; }
    const AttackScheduler& scheduler() const { return m_scheduler; }
    const Tower* selectedTower() const { return m_selectedTower; }
    int enemyCount() const { return m_enemies.size(); }
    int towerCount() const { return m_towers.size(); }
    int bulletCount() const { return m_bullets.size(); }
    // 索引必须在数量范围内；借用引用最多保留到下一次推进、交易或重置之前。
    const Enemy& enemyAt(int index) const { return *m_enemies.at(index); }
    const Tower& towerAt(int index) const { return *m_towers.at(index); }
    const Bullet& bulletAt(int index) const { return *m_bullets.at(index); }

private:
    friend class GameSessionProductTestAccess;
    void spawnEnemy(const QString& type, int waveIndex, QVector<Notice>& notices);
    void removeBulletsTargeting(const Enemy* enemy);
    void applyBossAuraToAllEnemies();
    void removeBossAuraFromAllEnemies();
    void checkResult();

    GameMap m_map;
    AttackScheduler m_scheduler;
    State m_state = State::Stopped;
    int m_gold = 150;
    int m_life = 10;
    int m_nextPathIndex = 0;
    bool m_bossAuraActive = false;
    bool m_bossAuraTriggered = false;
    // 容器中的裸指针均由本对象独占；删除敌人前先删除引用它的子弹。
    QVector<Enemy*> m_enemies;
    QVector<Tower*> m_towers;
    QVector<Bullet*> m_bullets;
    Tower* m_selectedTower = nullptr;
};

#endif // GAMESESSION_H
