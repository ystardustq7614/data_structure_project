/** @file
 * @brief 进攻 JSON 的文件适配与验证入口；路径由调用方提供，不参与战局推进。 */
#ifndef ATTACKPLANLOADER_H
#define ATTACKPLANLOADER_H

#include "attackplan.h"

#include <optional>

/** 区分致命的内置配置错误与可回退的外部错误；warning 不代表 plan 不可用。 */
struct AttackPlanLoadResult {
    std::optional<AttackPlan> plan;
    QString warning;
    QString error;
};

/** 无运行状态的加载入口；每次调用独立读取文件，不缓存上一次新局的结果。 */
class AttackPlanLoader
{
public:
    /** 读取 builtinPath 阵容并尝试应用 rhythmPath 中的节奏覆盖。
     * 参数可使用 Qt 资源路径或文件路径；空/不存在的外部路径静默使用内置节奏。
     * 返回的 plan 有值即可开局；内置读取或校验失败时无 plan 并返回 error。
     * 外部读取或校验失败返回默认 plan 和 warning，任何已读字段都不部分生效。
     * 本方法执行文件 I/O；调用者应在新局边界调用，而非每个模拟 tick 调用。
     */
    static AttackPlanLoadResult load(const QString& builtinPath,
                                     const QString& rhythmPath);
};

#endif // ATTACKPLANLOADER_H
