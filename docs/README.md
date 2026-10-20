# 文档索引

项目介绍和快速运行见根目录 [README](../README.md)。文档按用途分类；目录中的相对链接从本文所在位置解析，正文内的代码路径默认以仓库根目录为基准。

| 分类 | 文档 | 用途 |
| --- | --- | --- |
| `architecture/` 架构 | [项目代码结构](architecture/ARCHITECTURE.md) | 模块职责、依赖、所有权与单帧顺序 |
| `gameplay/` 玩法 | [进攻节奏与局内操作](gameplay/ATTACK_RHYTHM.md) | 操作、外部 JSON 字段、范围与回退规则 |
| `development/` 开发 | [构建与验证](development/BUILDING.md)、[项目注释要求](development/项目注释要求.md) | 工具链、运行库、测试和注释规范 |
| `assets/` 素材 | [敌人](assets/enemies.md)、[页面背景](assets/screens.md)、[防御塔](assets/towers.md) | 类型对应、处理方式与来源记录 |
| `collaboration/` 协作 | [交接状态](collaboration/HANDOFF.md)、[变更记录](collaboration/CHANGELOG.md) | 最新任务、验证证据和实际改动 |
| `plans/` 计划 | [实施计划](plans/IMPLEMENTATION_PLAN.md)、[视觉迁移计划](plans/MIGRATION_PLAN_20261017.md) | 已执行任务的方案与约束 |
| `reviews/` 审查 | [初始问题报告](reviews/ISSUE_VERIFICATION_REPORT.md)、[远端比较](reviews/REMOTE_COMPARISON_20261017.md) | 历史问题与版本差异 |
| `reviews/` 审查 | [首次视觉审查](reviews/REVIEW_PORT_VISUALS_20261017.md)、[后续复核](reviews/REVIEW_PORT_VISUALS_FOLLOWUP_20261017.md)、[修复闭环复核](reviews/REVIEW_PREVIEW_CLOSURE_20261017.md) | 各轮实际结论及验证范围 |
| `reports/` 报告 | [项目报告第四、五章](reports/项目报告补充_第四五章.md) | 总体设计与主要模块说明 |

`AGENTS.md` 保留在[仓库根目录](../AGENTS.md)，供协作工具发现。原图、JSON 清单和提示词继续保存在 `art/`；对应说明统一在 `assets/`。

## 历史资料的使用

计划、审查和交接历史保留当时的事实。已修复的问题、旧源码行号及已清理的 `builds/` 路径不能直接当作当前状态；最新结论以交接顶部和最新轮次为准。原先指向已删除文件的本地链接已改为历史位置文本，未恢复旧素材或构建文件。第 37 轮的本机会话归档位置和检索方法见交接；该归档不属于仓库依赖。
