/** @file
 * @brief 已验证的进攻计划及共享限制；运行事件、文件读取和实体生成分别由调度器、加载器、战局负责。 */
#ifndef ATTACKPLAN_H
#define ATTACKPLAN_H

#include <QString>
#include <QVector>
#include <QtGlobal>

#include <utility>

// 数量固定为当前六波阵容；文件大小单位为字节，Frames 上限单位为 30 ms 逻辑 tick。
// 加载器与调度器共用这些边界，放宽配置范围时必须同时复核时间溢出保护。
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

/** 配置与运行态共享的事件种类；增援也生成敌人，WaveStart 只通知切波。 */
enum class AttackEventKind { Spawn, Reinforcement, WaveStart };

/** 一个待生成敌人的配置值；type 是内置类型键，WaveStart 不得写入阵容。 */
struct AttackUnit {
    QString type;
    AttackEventKind kind = AttackEventKind::Spawn;
};

/** 一波配置的数据载体；只有进入 AttackPlan 后才保证通过加载器校验。
 * Frames 字段单位为 30 ms 逻辑 tick；units 顺序决定同波出怪顺序。
 * clearThreshold 是清理前的场上敌人数门槛，burstSize 等数量字段不表示时间。
 */
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

/** 已验证的整局进攻计划；只有加载器能构造，调用者可复制或整份替换。
 * 不保存运行 tick 或事件堆，复制后可供不同战局分别调度。
 */
class AttackPlan
{
public:
    /** 复制配置值，不共享可变运行状态；赋值后应重新获取此前借用的 waves() 引用。 */
    AttackPlan(const AttackPlan&) = default;
    AttackPlan& operator=(const AttackPlan&) = default;

    /** 返回已验证的六波配置；借用引用在本计划被替换或销毁前有效，无失败结果。 */
    const QVector<AttackWave>& waves() const { return m_waves; }
    /** 返回整局计划生成数，含增援；当前有效配置固定为 90，不表示已生成数量。 */
    int totalEnemies() const { return m_totalEnemies; }

private:
    friend class AttackPlanLoader;
    AttackPlan(QVector<AttackWave> waves, int totalEnemies)
        : m_waves(std::move(waves)), m_totalEnemies(totalEnemies) {}

    QVector<AttackWave> m_waves;
    int m_totalEnemies;
};

#endif // ATTACKPLAN_H
