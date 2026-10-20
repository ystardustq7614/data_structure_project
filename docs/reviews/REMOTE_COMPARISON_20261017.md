# 本地重构与协作者更新对比（2026-10-17）

结论：两边改动方向互补。建议保留当前 `core/ui` 分层，把协作者新增的菜单敌人展示、塔头像和建造预览迁入相应 UI 模块。原工作树保持不变，本轮没有合并、切分支、提交或推送。

## 版本与范围

| 版本 | 实际状态 |
| --- | --- |
| 共同基线 / 当前HEAD | `6b22b94f5879c22daf006a606a62ee3c33f3fe20` |
| 本地版本 | `codex/review-ui-pause-placement-events` 的实际工作树快照，包含尚未提交、尚未跟踪的重构源码，不能只用HEAD代表 |
| 协作者版本 | `origin/feature-ui-pause-placement-events`，`a715821ce5274bc50a7ae6e6f63be5ded935cbbc` |
| main | 本地及抓取后的 `origin/main` 仍为 `e6a5350`，没有这两项新提交 |

已实际执行 `git -c credential.interactive=false -c credential.guiPrompt=false fetch --no-tags origin`，成功获取并确认远端引用。`git rev-list --left-right --count HEAD...origin/feature-ui-pause-placement-events` 为 `0 2`，只计已提交历史，不包括本地工作树的大量重构。

远端新增两项提交，作者均为EntropyXi，时间为Asia/Shanghai：

| 提交 | 时间 | 改动 |
| --- | --- | --- |
| `748ea9d` | 2026-10-17 10:39:43 | `fix: show actual enemy sprites on start menu`：菜单原背景中的三幅示意怪物改为覆盖面板，显示实际bug/ddl/virus/boss图片 |
| `a715821` | 2026-10-17 10:54:29 | `feat: show tower portraits in game and placement preview`：从现有菜单背景裁取三种塔头像，已建塔和半透明建造预览使用同型形象 |

相对共同基线共7个文件、161行新增、29行删除：4个产品文件、1个GUI测试、2份交接文档。没有更改图片/音频资源、qrc、CMake、敌人规则、调度器或塔平衡参数；变更的塔代码限于缓存和绘制。

## 按当前结构对照

| 内容 | 本地当前实现 | 远端新增行为 | 合入本地时的落点 |
| --- | --- | --- | --- |
| 项目结构 | `GameSession`权威战局；QtCore核心与UI绘制分开；五项CTest | 仍在旧根级类文件中，三项CTest | 保留本地结构，不以旧文件覆盖新模块 |
| 菜单敌人 | 原菜单背景；战斗Renderer已使用实际敌人贴图 | 在背景敌方区域覆盖纸张面板，显示四类真实敌人及Virus说明 | `26C-/ui/startmenuwidget.h/.cpp` |
| 已建塔 | `GameRenderer::drawTower()` 绘制N/F/H字母圆塔 | `Tower::draw()` 使用菜单裁图、类型色边框、左下等级徽章，缺图保留字母回退 | 图片缓存和绘制均迁入 `26C-/ui/gamerenderer.h/.cpp`，核心 `Tower` 继续只提供规则和只读属性 |
| 建造预览 | Renderer绘制绿色/红色半透明圆 | 远端GameWidget临时构造Tower并调用其draw，再叠合法性轮廓 | `GameRenderer::drawBuildPreview()` 复用UI塔绘制，保留现有共享放置判定 |
| 敌人资源测试 | 本地测试已逐类解码实际图片，并验证Renderer绘制内容 | 增补四个 `QResource::isValid()` 检查 | 可并入 `tests/test_gui_product.cpp`；它只补存在性，不能证明菜单面板已正确绘出 |
| 文档 | 含本地拉取复核、结构重构、并行准备的历史 | 远端第21、22节分别描述菜单与塔形象 | 整理两项提交摘要并保留本地历史；节号与另一台机器的工具链/验收结果需分别记录 |

关键当前入口：`ui/startmenuwidget.cpp:16/34`、`ui/gamerenderer.cpp:9/303/539`、`core/tower.cpp:6/28`、`tests/test_gui_product.cpp:107/140`，路径均相对 `26C-/`，测试文件相对工作区根目录。远端裁图来自现有 `menu_bg.jpg`，未增加独立塔图片。

## 冲突与三方预演

已保存共同基线、远端完整源码、本地实际源码快照，并用 `git merge-file --diff3 -p` 生成**未应用**的文本预览：

| 映射后的文件 | 文本预演结果 | 解释 |
| --- | --- | --- |
| `26C-/ui/startmenuwidget.cpp` | 0处冲突 | 菜单功能补丁与本地共享按钮反馈/退出信号迁移可在文本层合并 |
| `26C-/ui/startmenuwidget.h` | 0处冲突 | 新增图标缓存和绘制声明可迁移 |
| `tests/test_gui_product.cpp` | 0处冲突 | 资源存在性检查与本地重构后的测试可合并 |
| `HANDOFF.md` | 5处冲突 | 顶部状态、分工和不同任务历史不能直接覆盖 |
| `CHANGELOG.md` | 1处冲突 | 两边都在顶部新增记录，应保留各自实际变更与证据 |

远端修改的旧 `26C-/gamewidget.cpp`、`tower.cpp`、`startmenuwidget.cpp/.h` 四个路径，已在本地删除并迁入新模块。因此原补丁不能按旧路径直接应用；当前工作树尚未提交，直接pull/覆盖也不适合作为比较方式。塔绘制还跨越职责拆分，未对 `core/tower.cpp` 做机械文本合并。

文本预演没有修改源文件，也没有验证一个合并候选版本能编译。两边独立构建通过仍不代表合并后通过。

## 本机验证与可复查材料

工具链：Qt 6.10.3 / MinGW g++ 13.1.0 / CMake 3.30.5 / Ninja 1.12.1。使用当前工作区可写临时目录和匹配Qt运行库路径；两边分别在新目录构建各自真实测试入口。

| 本轮实际验证 | 本地重构快照 | 远端a715821快照 |
| --- | --- | --- |
| 新目录配置与编译 | 退出0，43/43构建步骤 | 退出0，25/25构建步骤 |
| CTest | 5/5通过，3.60秒 | 3/3通过，2.52秒 |
| 真实GUI测试另行输出截图 | 退出0，9张PNG | 退出0，9张PNG |

九组截图中，`loss_pressed.png`、`win_pressed.png` 像素及SHA256完全一致；其余七组差异符合两项视觉修改：菜单差异范围为 `[694,211,1011,403)`，其余为塔/建造预览区域。已查看两边菜单、暂停中的已建普通塔及合法建造预览截图，确认实际敌人贴图和塔肖像已经在远端生效。像素差异范围不能证明全部交互或所有塔型均已人工验收。

当前offscreen环境中，两边运行时绘制文字仍存在方框缺字；远端新增菜单标题/名称也受影响。旧菜单背景里的文字为图片内容，显示正常不代表动态字体可用。这延续既有离屏字体限制，本轮没有定位或修复真实桌面字体问题。

可打开`builds/Codex-Compare-20261017-a715821/visual-comparison.html`切换九个场景。左侧为本地快照，右侧为远端快照，均不是合并候选的效果。

本轮验证只面向两份快照的真实核心/UI测试与离屏截图，不代表真实桌面字体、鼠标手感、音频听感或完整玩法验收。本地事件面板测试的既有断言盲区仍保留，参见HANDOFF第23节。

原工作树保护核对通过：除本轮允许更新的HANDOFF/CHANGELOG外，快照覆盖的78份原文件SHA256未变；本地80份快照均未改，索引没有暂存内容。全工作树差异检查退出2，检出旧构建目录AutogenInfo.json两处尾空白，未清理既有生成文件；排除builds后的源码/文档检查退出0，新增6份报告/脚本/查看器文本无尾空白，不能将其写作全工作树检查通过。实际结果保存在 `diff-check-results.json` 及两份日志。

材料目录：`builds/Codex-Compare-20261017-a715821/`。

- `base/`：共同基线的七份变更文件；`local/`：本地80份实际源码/资源/文档快照；`remote/`：远端完整产品/测试与相关文档。
- `remote.patch`：远端相对基线的完整补丁；`local-tracked.patch`：本地已跟踪变化，未跟踪新源码另外保存在 `local/`。
- `comparison.json`、`merge-preview/`：映射和文本冲突预演；带冲突标记的文档只供审阅，不能直接作为产品文件。
- `tests-local/`、`tests-remote/`：各自配置、编译、CTest和GUI截图日志；`screenshots-local/`、`screenshots-remote/`：两边实际GUI截图。
- `screenshot-comparison.json`、`visual-comparison.html`：逐场景像素/SHA256对比和可切换的双栏查看器；原始PNG未被修改。
- `local-hashes-before.json`：比较前快照SHA256，用于确认除本轮交接/报告外的源码等文件未改。
- `verify_workspace_unchanged.py`、`workspace-protection.json`：原工作树78文件和本地80文件快照的保护核对。

复查命令（工作目录为工作区根目录；脚本位于上述隔离目录）：

```powershell
& 'C:/Users/ZiggyS/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -B builds/Codex-Compare-20261017-a715821/compare_versions.py
& './builds/Codex-Compare-20261017-a715821/verify_snapshot.ps1' -Variant local
& './builds/Codex-Compare-20261017-a715821/verify_snapshot.ps1' -Variant remote
& 'C:/Users/ZiggyS/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -B builds/Codex-Compare-20261017-a715821/compare_screenshots.py
& 'C:/Users/ZiggyS/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -B builds/Codex-Compare-20261017-a715821/verify_workspace_unchanged.py
git diff --check
git diff --check -- . ':(exclude)builds/**'
```

`compare_versions.py` 对已存在的快照目录会拒绝覆盖；重做快照时应使用新的隔离目录，或只复查已保存的补丁、JSON及日志。脚本固定核对远端tip为a715821，防止后续新提交混入本轮比较。

下一执行者为Codex：本轮交付对比结果；若用户后续安排合并或移植，应保留本地分层，按上述UI落点迁移两项功能，再对合并版本单独验证，不自动扩大本轮范围。
