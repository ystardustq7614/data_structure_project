#ifndef ATTACKSCHEDULER_H
#define ATTACKSCHEDULER_H

#include <QVector>
#include <QString>
#include <QtGlobal>

#include <queue>
#include <vector>

class AttackScheduler
{
public:
    enum class Kind { Spawn, Reinforcement, WaveStart };

    struct Event {
        qint64 dueTick = 0;
        quint64 sequence = 0;
        Kind kind = Kind::Spawn;
        int waveIndex = 0;
        QString enemyType;
    };

    // An empty path uses applicationDirPath()/attack_rhythm.json.
    bool reset(const QString& rhythmPath = QString(),
               QString* warning = nullptr, QString* error = nullptr,
               const QString& builtinPath = QStringLiteral(":/config/attack_waves.json"));
    QVector<Event> advance(int activeEnemies);
    QVector<Event> preview(int count = 3) const;

    qint64 currentTick() const { return m_tick; }
    int currentWaveIndex() const { return m_currentWaveIndex; }
    int totalEnemies() const { return m_totalEnemies; }
    int spawnedCount() const { return m_spawnedCount; }
    bool waitingForClear() const { return m_state == State::WaitingForClear; }
    bool hasPendingGeneration() const { return m_spawnedCount < m_totalEnemies; }
    int clearThreshold() const;
    bool isReady() const { return m_ready; }

private:
    struct Unit {
        QString type;
        Kind kind = Kind::Spawn;
    };

    struct Wave {
        QVector<Unit> units;
        int spawnIntervalFrames = 14;
        int burstSize = 1;
        int burstGapFrames = 9;
        int openingDelayFrames = 23;
        int openingBurstSize = 1;
        int nextWaveDelayFrames = 20;
        int clearThreshold = 6;
    };

    struct LaterFirst {
        bool operator()(const Event& left, const Event& right) const
        {
            if (left.dueTick != right.dueTick) {
                return left.dueTick > right.dueTick;
            }
            return left.sequence > right.sequence;
        }
    };

    using EventHeap = std::priority_queue<Event, std::vector<Event>, LaterFirst>;
    enum class State { Empty, Spawning, WaitingForClear, WaitingForWaveStart, Finished };

    bool loadBuiltin(const QString& path, QString* error);
    bool applyRhythmFile(const QString& path, QString* error);
    void enqueue(qint64 dueTick, Kind kind, int waveIndex, const QString& type = QString());
    void beginWave(int waveIndex, qint64 startTick);

    QVector<Wave> m_waves;
    EventHeap m_events;
    State m_state = State::Empty;
    qint64 m_tick = 0;
    qint64 m_lastSpawnTick = -1;
    quint64 m_nextSequence = 0;
    int m_currentWaveIndex = 0;
    int m_remainingInWave = 0;
    int m_totalEnemies = 0;
    int m_spawnedCount = 0;
    bool m_ready = false;
};

#endif // ATTACKSCHEDULER_H
