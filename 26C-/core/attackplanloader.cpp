/** @file
 * @brief 进攻配置的严格验证实现；候选副本全部通过后才替换，禁止半份节奏生效。 */
#include "attackplanloader.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>

#include <cmath>

namespace {
using namespace AttackPlanLimits;

bool readInteger(const QJsonObject& object, const QString& key, int minimum,
                 int maximum, int* result, QString* error)
{
    const QJsonValue value = object.value(key);
    if (!value.isDouble()) {
        *error = QString("%1 must be an integer").arg(key);
        return false;
    }
    // JSON 数字统一以 double 读取；必须先检查有限整数和范围，才能安全转换为帧数。
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
    // 在 readAll 前限制配置体积，阵容与节奏文件共用同一输入边界。
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

bool loadBuiltin(const QString& path, QVector<AttackWave>* waves,
                 int* totalEnemies, QString* error)
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

    QVector<AttackWave> candidate;
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
        AttackWave wave;
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
                // 索引从 0 开始：第六波第 8 个是唯一 Boss，其后 18 个标记为增援。
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
                wave.units.append({type, shouldReinforce ? AttackEventKind::Reinforcement : AttackEventKind::Spawn});
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
    *waves = candidate;
    *totalEnemies = total;
    return true;
}

bool applyRhythmFile(const QString& path, QVector<AttackWave>* waves, QString* error)
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
    // 每个覆盖字段都写入副本；后续任一波验证失败时，调用方仍拿到完整默认配置。
    QVector<AttackWave> candidate = *waves;
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
        AttackWave& wave = candidate[index];
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
    *waves = candidate;
    return true;
}

} // namespace

AttackPlanLoadResult AttackPlanLoader::load(const QString& builtinPath,
                                           const QString& rhythmPath)
{
    AttackPlanLoadResult result;
    QVector<AttackWave> waves;
    int totalEnemies = 0;
    if (!loadBuiltin(builtinPath, &waves, &totalEnemies, &result.error)) {
        return result;
    }

    // 外部文件缺失是正常情况；无效覆盖保留完整默认值，不部分修改已验证配置。
    if (!rhythmPath.isEmpty() && QFileInfo::exists(rhythmPath)) {
        QString reason;
        if (!applyRhythmFile(rhythmPath, &waves, &reason)) {
            result.warning = QString("进攻节奏配置无效，已使用默认节奏：%1").arg(reason);
        }
    }
    result.plan = AttackPlan(std::move(waves), totalEnemies);
    return result;
}
