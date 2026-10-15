#include "attackscheduler.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>

#include <cmath>
#include <limits>

namespace {

constexpr int kWaveCount = 6;
constexpr int kEnemyCount = 90;
constexpr int kMaximumFileBytes = 65536;
constexpr int kMaximumFrameValue = 10000;
constexpr qint64 kMaximumScheduleSpan = qint64(kMaximumFrameValue)
                                         * kEnemyCount * 11;
const int kWaveEnemyCounts[kWaveCount] = {8, 14, 18, 10, 14, 26};

bool readInteger(const QJsonObject& object, const QString& key, int minimum,
                 int maximum, int* result, QString* error)
{
    const QJsonValue value = object.value(key);
    if (!value.isDouble()) {
        *error = QString("%1 must be an integer").arg(key);
        return false;
    }
    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number
        || number < minimum || number > maximum) {
        *error = QString("%1 must be an integer in [%2, %3]")
                     .arg(key).arg(minimum).arg(maximum);
        return false;
    }
    *result = static_cast<int>(number);
    return true;
}

bool readJsonFile(const QString& path, QJsonObject* root, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QString("cannot open %1: %2").arg(path, file.errorString());
        return false;
    }
    if (file.size() > kMaximumFileBytes) {
        *error = QString("%1 exceeds %2 bytes").arg(path).arg(kMaximumFileBytes);
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        *error = QString("invalid JSON in %1: %2").arg(path, parseError.errorString());
        return false;
    }
    if (!document.isObject()) {
        *error = QString("JSON root in %1 must be an object").arg(path);
        return false;
    }
    *root = document.object();
    return true;
}

bool hasOnlyKeys(const QJsonObject& object, const QSet<QString>& keys,
                 QString* error)
{
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!keys.contains(it.key())) {
            *error = QString("unknown key: %1").arg(it.key());
            return false;
        }
    }
    return true;
}

} // namespace

bool AttackScheduler::loadBuiltin(const QString& path, QString* error)
{
    QJsonObject root;
    if (!readJsonFile(path, &root, error)) {
        return false;
    }
    if (!hasOnlyKeys(root, {"version", "waves"}, error)) {
        return false;
    }
    int version = 0;
    if (!readInteger(root, "version", 1, 1, &version, error)) {
        return false;
    }
    const QJsonValue wavesValue = root.value("waves");
    if (!wavesValue.isArray() || wavesValue.toArray().size() != kWaveCount) {
        *error = "built-in configuration must contain six waves";
        return false;
    }

    QVector<Wave> candidate;
    candidate.reserve(kWaveCount);
    int total = 0;
    int bossCount = 0;
    const QJsonArray wavesArray = wavesValue.toArray();
    for (int waveIndex = 0; waveIndex < kWaveCount; ++waveIndex) {
        const QJsonValue waveValue = wavesArray.at(waveIndex);
        if (!waveValue.isObject()) {
            *error = QString("wave %1 must be an object").arg(waveIndex + 1);
            return false;
        }
        const QJsonObject object = waveValue.toObject();
        if (!hasOnlyKeys(object,
                         {"spawnIntervalFrames", "burstSize", "burstGapFrames",
                          "openingDelayFrames", "openingBurstSize",
                          "nextWaveDelayFrames", "clearThreshold", "groups"}, error)) {
            return false;
        }
        Wave wave;
        if (!readInteger(object, "spawnIntervalFrames", 1, kMaximumFrameValue,
                         &wave.spawnIntervalFrames, error)
            || !readInteger(object, "burstSize", 1, 10, &wave.burstSize, error)
            || !readInteger(object, "burstGapFrames", 1, kMaximumFrameValue,
                            &wave.burstGapFrames, error)
            || !readInteger(object, "openingDelayFrames", 1, kMaximumFrameValue,
                            &wave.openingDelayFrames, error)
            || !readInteger(object, "openingBurstSize", 1, 10,
                            &wave.openingBurstSize, error)
            || !readInteger(object, "nextWaveDelayFrames", 1, kMaximumFrameValue,
                            &wave.nextWaveDelayFrames, error)
            || !readInteger(object, "clearThreshold", 0, kEnemyCount,
                            &wave.clearThreshold, error)) {
            *error = QString("wave %1: %2").arg(waveIndex + 1).arg(*error);
            return false;
        }
        if (wave.openingBurstSize > wave.burstSize) {
            *error = QString("wave %1 openingBurstSize exceeds burstSize").arg(waveIndex + 1);
            return false;
        }
        const QJsonValue groupsValue = object.value("groups");
        if (!groupsValue.isArray() || groupsValue.toArray().isEmpty()) {
            *error = QString("wave %1 groups must be a nonempty array").arg(waveIndex + 1);
            return false;
        }
        for (const QJsonValue& groupValue : groupsValue.toArray()) {
            if (!groupValue.isObject()) {
                *error = QString("wave %1 contains an invalid group").arg(waveIndex + 1);
                return false;
            }
            const QJsonObject group = groupValue.toObject();
            if (!hasOnlyKeys(group, {"type", "count", "kind"}, error)) {
                return false;
            }
            const QString type = group.value("type").toString();
            if (type != "bug" && type != "ddl" && type != "virus" && type != "boss") {
                *error = QString("wave %1 has unknown enemy type").arg(waveIndex + 1);
                return false;
            }
            if (group.contains("kind") && !group.value("kind").isString()) {
                *error = QString("wave %1 has non-text event kind").arg(waveIndex + 1);
                return false;
            }
            const QString kindText = group.value("kind").toString("spawn");
            if (kindText != "spawn" && kindText != "reinforcement") {
                *error = QString("wave %1 has unknown event kind").arg(waveIndex + 1);
                return false;
            }
            int count = 0;
            if (!readInteger(group, "count", 1, kEnemyCount, &count, error)) {
                return false;
            }
            for (int index = 0; index < count; ++index) {
                const int unitIndex = wave.units.size();
                const bool shouldReinforce = waveIndex == 5 && unitIndex >= 8;
                if ((kindText == "reinforcement") != shouldReinforce) {
                    *error = QString("wave %1 reinforcement segment is misplaced")
                                 .arg(waveIndex + 1);
                    return false;
                }
                if (type == "boss") {
                    ++bossCount;
                    if (waveIndex != 5 || unitIndex != 7) {
                        *error = "boss must be the eighth enemy of wave six";
                        return false;
                    }
                }
                wave.units.append({type, shouldReinforce ? Kind::Reinforcement : Kind::Spawn});
            }
        }
        if (wave.units.size() != kWaveEnemyCounts[waveIndex]) {
            *error = QString("wave %1 has the wrong enemy count").arg(waveIndex + 1);
            return false;
        }
        total += wave.units.size();
        candidate.append(wave);
    }
    if (total != kEnemyCount || bossCount != 1) {
        *error = "built-in enemy total or boss count is invalid";
        return false;
    }
    m_waves = candidate;
    m_totalEnemies = total;
    return true;
}

bool AttackScheduler::applyRhythmFile(const QString& path, QString* error)
{
    QJsonObject root;
    if (!readJsonFile(path, &root, error)) {
        return false;
    }
    if (!hasOnlyKeys(root, {"version", "waves"}, error)) {
        return false;
    }
    int version = 0;
    if (!readInteger(root, "version", 1, 1, &version, error)) {
        return false;
    }
    if (!root.value("waves").isArray()
        || root.value("waves").toArray().size() != kWaveCount) {
        *error = "rhythm override must contain six wave objects";
        return false;
    }
    QVector<Wave> candidate = m_waves;
    const QJsonArray array = root.value("waves").toArray();
    for (int index = 0; index < kWaveCount; ++index) {
        if (!array.at(index).isObject()) {
            *error = QString("rhythm wave %1 must be an object").arg(index + 1);
            return false;
        }
        const QJsonObject object = array.at(index).toObject();
        if (!hasOnlyKeys(object,
                         {"spawnIntervalFrames", "burstSize", "burstGapFrames",
                          "openingDelayFrames", "openingBurstSize",
                          "nextWaveDelayFrames", "clearThreshold"}, error)) {
            return false;
        }
        Wave& wave = candidate[index];
        struct Field { const char* key; int* value; int minimum; int maximum; };
        const Field fields[] = {
            {"spawnIntervalFrames", &wave.spawnIntervalFrames, 1, kMaximumFrameValue},
            {"burstSize", &wave.burstSize, 1, 10},
            {"burstGapFrames", &wave.burstGapFrames, 1, kMaximumFrameValue},
            {"openingDelayFrames", &wave.openingDelayFrames, 1, kMaximumFrameValue},
            {"openingBurstSize", &wave.openingBurstSize, 1, 10},
            {"nextWaveDelayFrames", &wave.nextWaveDelayFrames, 1, kMaximumFrameValue},
            {"clearThreshold", &wave.clearThreshold, 0, kEnemyCount}
        };
        for (const Field& field : fields) {
            if (object.contains(QLatin1String(field.key))
                && !readInteger(object, QLatin1String(field.key), field.minimum,
                                field.maximum, field.value, error)) {
                *error = QString("rhythm wave %1: %2").arg(index + 1).arg(*error);
                return false;
            }
        }
        if (wave.openingBurstSize > wave.burstSize) {
            *error = QString("rhythm wave %1 openingBurstSize exceeds burstSize")
                         .arg(index + 1);
            return false;
        }
    }
    m_waves = candidate;
    return true;
}

bool AttackScheduler::reset(const QString& rhythmPath, QString* warning,
                            QString* error, const QString& builtinPath)
{
    if (warning) warning->clear();
    if (error) error->clear();
    m_events = EventHeap();
    m_waves.clear();
    m_state = State::Empty;
    m_tick = 0;
    m_lastSpawnTick = -1;
    m_nextSequence = 0;
    m_currentWaveIndex = 0;
    m_remainingInWave = 0;
    m_totalEnemies = 0;
    m_spawnedCount = 0;
    m_ready = false;

    QString reason;
    if (!loadBuiltin(builtinPath, &reason)) {
        if (error) *error = reason;
        return false;
    }
    const QString path = rhythmPath.isEmpty()
                             ? QCoreApplication::applicationDirPath() + "/attack_rhythm.json"
                             : rhythmPath;
    if (QFileInfo::exists(path) && !applyRhythmFile(path, &reason)) {
        if (warning) *warning = QString("进攻节奏配置无效，已使用默认节奏：%1").arg(reason);
    }
    m_ready = true;
    beginWave(0, 0);
    return true;
}

void AttackScheduler::enqueue(qint64 dueTick, Kind kind, int waveIndex,
                              const QString& type)
{
    m_events.push({dueTick, m_nextSequence++, kind, waveIndex, type});
}

void AttackScheduler::beginWave(int waveIndex, qint64 startTick)
{
    m_currentWaveIndex = waveIndex;
    const Wave& wave = m_waves.at(waveIndex);
    m_remainingInWave = wave.units.size();
    m_state = State::Spawning;

    qint64 firstTick = startTick + wave.openingDelayFrames;
    int firstCount = wave.openingBurstSize;
    int unitIndex = 0;
    while (unitIndex < wave.units.size()) {
        const int count = qMin(firstCount, wave.units.size() - unitIndex);
        for (int i = 0; i < count; ++i) {
            const Unit& unit = wave.units.at(unitIndex++);
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
    if (m_state == State::WaitingForClear && m_lastSpawnTick < m_tick
        && activeEnemies <= m_waves.at(m_currentWaveIndex).clearThreshold) {
        const Wave& wave = m_waves.at(m_currentWaveIndex);
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
                m_state = event.waveIndex == m_waves.size() - 1
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
    return m_waves.isEmpty() ? 0 : m_waves.at(m_currentWaveIndex).clearThreshold;
}
