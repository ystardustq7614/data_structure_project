/** @file
 * @brief 单局规则边界：统一实体所有权、交易与模拟；调用方负责时钟、输入、文件路径及展示。 */
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

/** 一局游戏的唯一可变规则与实体持有者；不可复制，避免裸指针容器产生重复所有权。 */
class GameSession
{
public:
    /** 只有 Running 接受交易并推进；Won/Lost 须 reset 后才能重新开局。 */
    enum class State { Stopped, Running, Paused, Won, Lost };
    enum class BuildProblem { None, OutsideMap, Road, Occupied, InsufficientGold };
    /** 纯建造候选；center 在原始位置通过校验后才吸附，失败原因按校验先后返回。 */
    struct BuildEvaluation {
        QPointF center;
        BuildProblem problem = BuildProblem::OutsideMap;
        /** 返回候选是否可建；只表示判定时状态，实际交易仍须重新校验。 */
        bool valid() const { return problem == BuildProblem::None; }
    };
    /** 按发生顺序交给界面的提示；frames 使用实际推进的逻辑 tick，不是墙钟时间。 */
    struct Notice {
        QString text;
        int frames = 120;
    };
    /** 交易结果；success 才代表已生效，空 message 也可用于静默拒绝非 Running 操作。 */
    struct ActionResult {
        QString message;
        int frames = 60;
        bool success = false;
    };
    /** 单步输出；advanced 区分实际推进，stopped 表示调度异常停止，通知不直接绘制。 */
    struct FrameResult {
        bool advanced = false;
        bool finalWaveStarted = false;
        QVector<Notice> notices;
        bool stopped = false;
    };

    /** 构造空的 Stopped 战局；无进攻计划，reset 有效计划后才能推进。 */
    GameSession() = default;
    /** 释放全部独占实体；销毁顺序与 clear 相同，不触碰外部界面或音频。 */
    ~GameSession();
    GameSession(const GameSession&) = delete;
    GameSession& operator=(const GameSession&) = delete;

    /** 清空旧实体并复制有效 plan；running=false 重置但保持 Stopped，供返回菜单使用。
     * 恢复初始金币/生命并使全部旧实体借用失效；不负责启动任何计时器。
     */
    void reset(const AttackPlan& plan, bool running = true);
    /** 子弹先于敌人销毁，清计划/选择并恢复初始金币生命；反复调用仍为 Stopped。 */
    void clear();
    /** paused 只允许 Running/Paused 互换；实际切换返回 true，其余状态不变并返回 false。 */
    bool setPaused(bool paused);
    /** 仅 Running 推进一个 tick，返回通知及切波/停止标志；非 Running 返回空结果。
     * 调度、移动、攻击、清理按固定顺序执行，实体借用可在清理时失效。
     * 调度超出安全边界会转入 Stopped 并返回提示；本方法不操作界面或外部 QTimer。
     */
    FrameResult step();

    /** 对原始逻辑坐标 position 和塔型 type 做无副作用判定；先校验原坐标再吸附。
     * 非 Running、非有限/越界位置均返回 OutsideMap；遮挡由界面在调用前拦截。
     * 其余失败按地图边界、道路、占用、金币顺序返回，不保证 center 在所有失败中可用。
     */
    BuildEvaluation evaluateBuild(const QPointF& position, TowerType type) const;
    /** 重新判定 position/type 并建塔扣款；失败通过 ActionResult 返回，不改变金币或实体。 */
    ActionResult build(const QPointF& position, TowerType type);
    /** 校验运行态、选中塔、等级与金币后升级扣款；失败不扣款，非 Running 静默拒绝。 */
    ActionResult upgradeSelectedTower();
    /** 出售选中塔并按累计投入退金币，立即销毁塔并清选择；无选择或非 Running 不生效。 */
    ActionResult sellSelectedTower();
    /** Running 时选中输入顺序中首个命中的塔并返回 true；未命中或停止时保留旧选择并返回 false。 */
    bool selectTowerAt(const QPointF& position);
    /** 仅清除借用选择，不出售塔、不改变金币；可用于鼠标点击空白区域。 */
    void clearSelectedTower();

    /** @name 无副作用状态查询
     * gold/life 为整型余额/剩余生命，漏怪结算后 life 可为负；gameTick 为 30 ms 逻辑 tick。
     * map/scheduler 为借用只读引用；selectedTower 可为空，售塔/reset/clear 后必须重新获取。
     * 数量不提供实体所有权，查询无失败结果，也不推进模拟。
     * @{ */
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
    /** @} */
    /** @name 按当前容器索引借用实体
     * index 必须在对应数量范围内，不提供越界错误结果。
     * 返回引用最多保留到下一次推进、交易、clear/reset 或战局销毁之前。
     * @{ */
    const Enemy& enemyAt(int index) const { return *m_enemies.at(index); }
    const Tower& towerAt(int index) const { return *m_towers.at(index); }
    const Bullet& bulletAt(int index) const { return *m_bullets.at(index); }
    /** @} */

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
