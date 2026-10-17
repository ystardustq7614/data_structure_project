#ifndef ATTACKPLAN_H
#define ATTACKPLAN_H

#include <QString>
#include <QVector>
#include <QtGlobal>

#include <utility>

// 配置与运行态共用的边界；校验范围和调度溢出保护必须同步使用这些值。
namespace AttackPlanLimits {
inline constexpr int kWaveCount = 6;
inline constexpr int kEnemyCount = 90;
inline constexpr int kMaximumFileBytes = 65536;
inline constexpr int kMaximumFrameValue = 10000;
// 覆盖单波全部小队及切波等待的保守跨度，沿用既有调度的溢出边界。
inline constexpr qint64 kMaximumScheduleSpan = qint64(kMaximumFrameValue)
                                              * kEnemyCount * 11;
inline constexpr int kWaveEnemyCounts[kWaveCount] = {8, 14, 18, 10, 14, 26};
}

// 同一事件类型同时供配置加载器和调度器使用，避免两者互相包含。
enum class AttackEventKind { Spawn, Reinforcement, WaveStart };

// 配置中的一个生成单位；WaveStart 仅由运行时调度器生成。
struct AttackUnit {
    QString type;
    AttackEventKind kind = AttackEventKind::Spawn;
};

// 一波的只读配置数据。Frames 字段单位为 30 ms 逻辑 tick，数量字段不表示时间。
struct AttackWave {
    QVector<AttackUnit> units;
    int spawnIntervalFrames = 14;
    int burstSize = 1;
    int burstGapFrames = 9;
    int openingDelayFrames = 23;
    int openingBurstSize = 1;
    int nextWaveDelayFrames = 20;
    int clearThreshold = 6;
};

class AttackPlanLoader;

// 已验证的整局进攻计划。只有加载器能构造；外部只能复制、替换整份值或只读查询。
// 不包含事件堆、运行 tick、文件路径或 GUI 对象，可供多局独立使用。
class AttackPlan
{
public:
    AttackPlan(const AttackPlan&) = default;
    AttackPlan& operator=(const AttackPlan&) = default;

    // 返回已验证的六波配置；引用在本值对象被替换或销毁前有效。
    const QVector<AttackWave>& waves() const { return m_waves; }
    // 返回计划的生成总数；当前规则要求六波共 90 个敌人。
    int totalEnemies() const { return m_totalEnemies; }

private:
    friend class AttackPlanLoader;
    AttackPlan(QVector<AttackWave> waves, int totalEnemies)
        : m_waves(std::move(waves)), m_totalEnemies(totalEnemies) {}

    QVector<AttackWave> m_waves;
    int m_totalEnemies;
};

#endif // ATTACKPLAN_H
