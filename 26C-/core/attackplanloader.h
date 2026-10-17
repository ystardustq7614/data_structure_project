#ifndef ATTACKPLANLOADER_H
#define ATTACKPLANLOADER_H

#include "attackplan.h"

#include <optional>

// 内置配置失败时没有 plan 并返回 error；外部覆盖无效时返回默认 plan 和 warning。
struct AttackPlanLoadResult {
    std::optional<AttackPlan> plan;
    QString warning;
    QString error;
};

// 负责进攻配置的文件读取、校验和整份回退，不持有或推进游戏运行状态。
class AttackPlanLoader
{
public:
    // 两个路径由调用方明确指定。缺失或空外部路径静默使用默认值；
    // 无效覆盖不部分生效，内置配置错误则返回无计划，调用方必须阻止开局。
    static AttackPlanLoadResult load(const QString& builtinPath,
                                     const QString& rhythmPath);
};

#endif // ATTACKPLANLOADER_H
