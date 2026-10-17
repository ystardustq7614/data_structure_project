# 视觉迁移实施独立复核（2026-10-17）

> 本文保留第27轮审查时的事实；补齐后的当前结论见 [第29轮复核](REVIEW_PORT_VISUALS_FOLLOWUP_20261017.md)。下列两项旧结论不能直接当作最新未修复清单。

结论：两项视觉功能已迁入，暂未发现新的产品运行缺陷；独立干净构建、五项CTest及九张远端基线截图均通过。但新增视觉测试仍有可证实的漏检，集成工作区也漏带本地最新协作文档，尚不宜将整项交付标为验收完成。建议Gemini先补以下两项。

## 待修正问题

### P2：新增视觉断言无法检出头像/图标消失

位置：集成工作区 `tests/test_gui_product.cpp:133–148`，以及 `:203–207`；原工作区同名文件内容相同。

塔测试只断言中心像素完全不透明、徽章位置有像素。缺少肖像时，原字母圆塔也满足这两条断言；徽章采样点本身又位于塔身内，不能证明徽章存在。菜单只要求面板区域与原背景不同，空面板也会满足；资源存在性检查不能证明图标已实际绘制。

已在**独立审查副本**中做反向验证：跳过全部三塔头像缓存，同时让菜单四类敌人不绘制图标。重新编译后，实际 `test_gui_product.exe` 仍退出0。保存的截图明确呈现空敌人卡片和字母回退塔。因此这里存在已确认的测试漏检，并非推测。

修正要求：以真实Renderer/菜单和qrc或固定视觉基线核对对应贴图主体，能够检出图标缺失、三塔映射错误；补充迁移计划要求的升级/预览相关验证。文字区域与字体差异分开处理。应让上述反向对照触发失败，不能只增加更多alpha或整图“不相同”断言。

证据：`builds/Codex-Review-Port-Visuals-20261017/negative-controls/results.json`，`missing-visual-artwork-run.log` 和对应截图。

### P2：集成工作区没有带入最新本地文档

位置：`builds/Codex-Port-Visuals-20261017/source/AGENTS.md:12–15`、`项目报告补充_第四五章.md:366/439`、`HANDOFF.md:7`。

集成目录的AGENTS仍将根级GameWidget描述为战斗/波次持有者；报告还描述Tower负责绘制，引用已删除根级路径和 `initSpawnQueue()`。这两份文件与原工作区已更新的core/ui文档不同。集成目录的HANDOFF/CHANGELOG也仍是远端旧状态，称feature分支、3项测试，未记录本次迁移。

如果交付该集成分支，现有本地文档改动会被遗漏，协作者或新克隆得到的职责、定位和验证状态将与实际源码不符。ARCHITECTURE虽然已导入，也尚未说明本次塔头像缓存/共用绘制。

修正要求：将本地最新稳定说明纳入集成工作区，按实际UI入口补充菜单/塔展示说明；整理两边交接历史，准确注明集成分支、未提交状态、验证证据及剩余测试问题。保留远端历史，不以旧文档覆盖本地更新。

## 已确认通过及状态勘误

- 相对迁移前实际本地快照，源码变化恰为4个UI文件和 `tests/test_gui_product.cpp`。核心、GameWidget/MainWindow、配置、资源和共享CMake实现内容保持；未修改entity_rendering源码本身不是问题，既有缺图/射程/升级尺寸回归仍可复用。
- 两项远端视觉功能已在新职责中实现：菜单保留共享按钮反馈和quitRequested；Renderer构造期缓存裁图，已建塔与0.72透明度预览共用私有绘制。核心Tower不含绘制依赖，Painter状态在两个调用入口保存/恢复。
- **原工作区也已写入相同的5份源码增量**，与集成worktree的产品/测试/配置内容一致。第26节“原主工作树保护未动”的自述不准确。本轮没有回退或继续修改这些产品文件。
- 事件面板旧盲区已补上：在同一状态、金币150、关闭面板时合法的位置验证拦截。另一个反向对照取消面板拦截后，GUI测试退出1，明确报 `FAIL: event panel did not block building on covered legal map cell when gold is abundant`。该错误表示刻意引入的穿透确实被测试检出。
- 两个Git工作区仍分别是原分支HEAD 6b22b94、集成分支HEAD a715821。迁移内容尚未提交，远端两个提交已位于集成分支历史中；本轮没有fetch、切分支、提交或推送。

## 独立验证

工作目录：`D:/VS Project/data_structrue_project`。实际版本：Qt 6.10.3、MinGW g++ 13.1.0、CMake 3.30.5、Ninja 1.12.1。

先保存集成源码/文档快照与哈希，再于 `builds/Codex-Review-Port-Visuals-20261017/` 新目录验证，没有复用Gemini已有二进制证明通过：

| 检查 | 本轮实际结果 |
| --- | --- |
| 产品新目录配置/编译 | 退出0，21/21，生成final.exe |
| 测试新目录配置/编译 | 退出0，43/43，五个真实目标 |
| CTest | 5/5，3.68秒 |
| GUI另行生成截图 | 退出0，9张；与a715821的同环境参考逐张像素一致 |
| 菜单几何脚本 | 退出0，15点通过 |
| Core导入表 | 三个核心测试均只有Qt6Core及系统/编译器库，无QtGui/Widgets/Multimedia |
| 离屏产品启动 | 存活3秒，由仅持有该进程的验收脚本终止；不证明正常退出 |
| 视觉缺失反向对照 | 故意关闭塔头像和菜单图标后GUI仍通过，确认测试漏检 |
| 面板穿透反向对照 | 故意关闭拦截后GUI失败，确认旧盲区已得到有效补强 |
| 源码/文档空白检查 | 原工作树排除builds后退出0，集成已跟踪diff退出0；另查5份源码与新报告，无尾空白 |

反向对照仅修改审查目录内的源码副本；执行结束后源文件逐字节恢复，并重新构建基线GUI目标。哈希检查确认集成工作区71份文件、原工作区60份产品/测试/配置文件均未被本轮改动；审查源码副本也与捕获时相同。

原工作树全范围 `git diff --check` 仍退出2，来自旧构建目录AutogenInfo.json的MOC_SKIP/UIC_SKIP两处既有尾空白，未清理该生成文件；不能称全工作树检查通过。实际输出与各范围退出码保存于diff-check-results.json和对应日志。

关键复查命令（脚本中保存完整CMake参数、匹配PATH及可写TEMP/TMP）：

```powershell
& './builds/Codex-Review-Port-Visuals-20261017/verify_build.ps1' -Kind product
& './builds/Codex-Review-Port-Visuals-20261017/verify_build.ps1' -Kind tests
& 'C:/Users/ZiggyS/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 -B builds/Codex-Review-Port-Visuals-20261017/source/tests/test_menu_geometry.py
& './builds/Codex-Review-Port-Visuals-20261017/smoke_product.ps1'
& 'C:/Users/ZiggyS/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 -B builds/Codex-Review-Port-Visuals-20261017/check_negative_controls.py
& 'C:/Users/ZiggyS/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 -B builds/Codex-Review-Port-Visuals-20261017/finish_review.py
```

材料还包含 `migration-delta.patch`（相对本地迁移前）、`integration-vs-a715821.patch`（相对远端，含新增/删除文件）、构建/CTest日志、core-imports.log、smoke-result.json、comparison.json、screenshot-comparison.json和protection-results.json。

离屏动态文字仍有方框缺字；九组图只覆盖既有常规场景，不能证明三种塔全部等级的真实桌面显示、音频听感或完整玩法通过。Vulkan头文件提示及媒体日志没有阻断本次构建/测试，不作为额外图形后端或听感验收证据。

本轮原工作区仅维护本报告及HANDOFF/CHANGELOG，其他写入均为独立审查材料。下一执行者为Gemini，处理两项P2并回传实际diff/验证结果，再由Codex复核；无需为这两项重写核心或再次迁移产品功能。
