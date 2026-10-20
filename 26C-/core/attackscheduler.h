/** @file
 * @brief 纯运行态进攻调度；接收已验证计划并输出事件，实体生成由 GameSession 执行。 */
#ifndef ATTACKSCHEDULER_H
#define ATTACKSCHEDULER_H

#include "attackplan.h"

#include <optional>
#include <queue>
#include <vector>

/** 持有一局独立的计划副本、逻辑时钟和稳定最小堆；默认构造后须先 reset 才可推进。 */
class AttackScheduler
{
public:
    using Kind = AttackEventKind;

    /** 按 dueTick、sequence 升序交付；waveIndex 从 0 开始，WaveStart 的 enemyType 为空。 */
    struct Event {
        qint64 dueTick = 0;
        quint64 sequence = 0;
        Kind kind = Kind::Spawn;
        int waveIndex = 0;
        QString enemyType;
    };

    /** 复制有效 plan 并清空旧事件/计数；在 tick 0 安排首波，不立即生成敌人或执行 I/O。 */
    void reset(const AttackPlan& plan);
    /** 推进一个 30 ms 逻辑 tick，按实际执行顺序返回本 tick 到期事件。
     * activeEnemies 必须为移动和清理前的非负场上敌人数，决定清场等待的起点。
     * 未初始化时返回空；时间越过安全边界时返回空并令 isReady() 为 false。
     * 空结果也可表示正常等待，调用方须结合 isReady() 区分停止与无到期事件。
     */
    QVector<Event> advance(int activeEnemies);
    /** 返回最多 count 个已入堆事件；count<=0 返回空，不消费事件或推进时钟。
     * 复制堆需要 O(n) 空间；尚未满足清场条件的下一波不会出现在结果中。
     */
    QVector<Event> preview(int count = 3) const;

    /** @name 无副作用查询
     * 时间单位为逻辑 tick，波次索引从 0 开始；生成数含增援，不表示存活数。
     * 默认构造时尚未就绪且计数为零；hasPendingGeneration 仅比较计划量与已生成量。
     * clearThreshold 在无计划时为 0，其余查询直接返回当前值，无失败结果。
     * @{ */
    qint64 currentTick() const { return m_tick; }
    int currentWaveIndex() const { return m_currentWaveIndex; }
    int totalEnemies() const { return m_plan ? m_plan->totalEnemies() : 0; }
    int spawnedCount() const { return m_spawnedCount; }
    bool waitingForClear() const { return m_state == State::WaitingForClear; }
    bool hasPendingGeneration() const { return m_spawnedCount < totalEnemies(); }
    int clearThreshold() const;
    bool isReady() const { return m_ready; }
    /** @} */

private:
    struct LaterFirst {
        bool operator()(const Event& left, const Event& right) const
        {
            if (left.dueTick != right.dueTick) {
                return left.dueTick > right.dueTick;
            }
            // 同 tick 按插入序号执行，保证生成、增援和切波的顺序稳定。
            return left.sequence > right.sequence;
        }
    };

    using EventHeap = std::priority_queue<Event, std::vector<Event>, LaterFirst>;
    enum class State { Empty, Spawning, WaitingForClear, WaitingForWaveStart, Finished };

    void enqueue(qint64 dueTick, Kind kind, int waveIndex, const QString& type = QString());
    void beginWave(int waveIndex, qint64 startTick);

    std::optional<AttackPlan> m_plan;
    EventHeap m_events;
    State m_state = State::Empty;
    qint64 m_tick = 0;
    qint64 m_lastSpawnTick = -1;
    quint64 m_nextSequence = 0;
    int m_currentWaveIndex = 0;
    int m_remainingInWave = 0;
    int m_spawnedCount = 0;
    bool m_ready = false;
};

#endif // ATTACKSCHEDULER_H
