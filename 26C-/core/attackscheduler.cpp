/** @file
 * @brief 稳定事件堆及清场等待的实现；只输出事件，不读取文件或操作敌人对象。 */
#include "attackscheduler.h"

#include <limits>

using AttackPlanLimits::kMaximumScheduleSpan;

void AttackScheduler::reset(const AttackPlan& plan)
{
    m_events = EventHeap();
    m_plan = plan;
    m_state = State::Empty;
    m_tick = 0;
    m_lastSpawnTick = -1;
    m_nextSequence = 0;
    m_currentWaveIndex = 0;
    m_remainingInWave = 0;
    m_spawnedCount = 0;
    m_ready = true;
    beginWave(0, 0);
}

void AttackScheduler::enqueue(qint64 dueTick, Kind kind, int waveIndex,
                              const QString& type)
{
    m_events.push({dueTick, m_nextSequence++, kind, waveIndex, type});
}

void AttackScheduler::beginWave(int waveIndex, qint64 startTick)
{
    m_currentWaveIndex = waveIndex;
    const AttackWave& wave = m_plan->waves().at(waveIndex);
    m_remainingInWave = wave.units.size();
    m_state = State::Spawning;

    qint64 firstTick = startTick + wave.openingDelayFrames;
    int firstCount = wave.openingBurstSize;
    int unitIndex = 0;
    while (unitIndex < wave.units.size()) {
        const int count = qMin(firstCount, wave.units.size() - unitIndex);
        for (int i = 0; i < count; ++i) {
            const AttackUnit& unit = wave.units.at(unitIndex++);
            enqueue(firstTick + qint64(i) * wave.burstGapFrames,
                    unit.kind, waveIndex, unit.type);
        }
        // 下一组必须晚于本组时隙；+1 保留旧节奏在组间多等待一次检查的规则。
        firstTick += qMax(wave.spawnIntervalFrames,
                          firstCount * wave.burstGapFrames + 1);
        firstCount = wave.burstSize;
    }
}

QVector<AttackScheduler::Event> AttackScheduler::advance(int activeEnemies)
{
    QVector<Event> dispatched;
    if (!m_ready) {
        return dispatched;
    }
    // 在增 tick 和安排整波前预留足够跨度，避免 dueTick 加法溢出后破坏事件顺序。
    if (m_tick > std::numeric_limits<qint64>::max() - kMaximumScheduleSpan) {
        m_ready = false;
        return dispatched;
    }
    ++m_tick;
    // 首次满足清场条件的检查计入等待次数，因此沿用 C+delay-1 的切波时刻。
    if (m_state == State::WaitingForClear && m_lastSpawnTick < m_tick
        && activeEnemies <= m_plan->waves().at(m_currentWaveIndex).clearThreshold) {
        const AttackWave& wave = m_plan->waves().at(m_currentWaveIndex);
        enqueue(m_tick + wave.nextWaveDelayFrames - 1,
                Kind::WaveStart, m_currentWaveIndex + 1);
        m_state = State::WaitingForWaveStart;
    }
    while (!m_events.empty() && m_events.top().dueTick <= m_tick) {
        Event event = m_events.top();
        m_events.pop();
        if (event.kind == Kind::WaveStart) {
            beginWave(event.waveIndex, event.dueTick);
        } else {
            ++m_spawnedCount;
            --m_remainingInWave;
            m_lastSpawnTick = m_tick;
            if (m_remainingInWave == 0) {
                m_state = event.waveIndex == m_plan->waves().size() - 1
                              ? State::Finished : State::WaitingForClear;
            }
        }
        dispatched.append(event);
    }
    return dispatched;
}

QVector<AttackScheduler::Event> AttackScheduler::preview(int count) const
{
    // priority_queue 没有有序遍历接口；在副本上弹出，确保查看预览不消费真实事件。
    QVector<Event> events;
    EventHeap copy = m_events;
    while (!copy.empty() && events.size() < count) {
        events.append(copy.top());
        copy.pop();
    }
    return events;
}

int AttackScheduler::clearThreshold() const
{
    return m_plan ? m_plan->waves().at(m_currentWaveIndex).clearThreshold : 0;
}
