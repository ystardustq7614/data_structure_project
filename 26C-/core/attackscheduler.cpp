// 稳定最小堆只保存本局事件，已验证计划通过值复制与其他局隔离。
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
