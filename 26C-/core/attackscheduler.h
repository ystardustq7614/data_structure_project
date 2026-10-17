#ifndef ATTACKSCHEDULER_H
#define ATTACKSCHEDULER_H

#include "attackplan.h"

#include <optional>
#include <queue>
#include <vector>

// 只负责本局逻辑 tick、稳定最小堆和波次等待；不读取配置文件，也不创建游戏对象。
class AttackScheduler
{
public:
    using Kind = AttackEventKind;

    struct Event {
        qint64 dueTick = 0;
        quint64 sequence = 0;
        Kind kind = Kind::Spawn;
        int waveIndex = 0;
        QString enemyType;
    };

    // 复制已验证计划，清空旧事件与计数，并在 tick 0 安排首波；无文件 I/O。
    void reset(const AttackPlan& plan);
    // 推进一个 30 ms 逻辑 tick。activeEnemies 是移动和清理之前的场上敌人数。
    // 未初始化或达到时间安全边界时返回空事件；后者同时使 isReady() 变为 false。
    QVector<Event> advance(int activeEnemies);
    // 按实际执行顺序返回最多 count 个待执行事件；不消费原堆，复制堆需要 O(n)。
    QVector<Event> preview(int count = 3) const;

    // 以下查询不推进调度；默认构造的对象尚未准备好，计数均为零。
    qint64 currentTick() const { return m_tick; }
    int currentWaveIndex() const { return m_currentWaveIndex; }
    int totalEnemies() const { return m_plan ? m_plan->totalEnemies() : 0; }
    int spawnedCount() const { return m_spawnedCount; }
    bool waitingForClear() const { return m_state == State::WaitingForClear; }
    bool hasPendingGeneration() const { return m_spawnedCount < totalEnemies(); }
    int clearThreshold() const;
    bool isReady() const { return m_ready; }

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
