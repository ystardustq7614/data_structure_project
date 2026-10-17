# 视觉迁移补齐后独立复核（2026-10-17，第29轮）

> 历史报告（第29轮）：下文保留当时的审查结论及反向对照。第30轮两项修复已于第32轮通过独立复核：交接可读，新增预览断言能检出塔型及透明度回归，不再按下文旧结论继续修补。全文历史证明范围及最新结果见 `REVIEW_PREVIEW_CLOSURE_20261017.md`、`HANDOFF.md` 顶部和第32节。

结论：三种已建塔头像和菜单四类敌人的断言补强有效；稳定文档与报告也已同步，未发现新的产品运行缺陷。仍有两项 P2：交接正文被编码转换损坏，以及预览断言仍无法检出塔型错误。当前不宜标记为全部验收完成。

## 待修正问题

### P2：两边 HANDOFF 正文已损坏，不能作为可读交接

交付位置：`builds/Codex-Port-Visuals-20261017/source/HANDOFF.md:1–16` 及整篇历史正文；原工作区收到的同名文件逐字节相同。

按 UTF-8 严格解码后，首行实际是 `# 鎵ц浜ゆ帴锛氳瑙夎縼绉诲疄鏂姐€佺嫭绔嬪鏍镐笌 P2 闂幆淇`，并非终端显示异常。日期、待办、历史结论与 Markdown 标记同样受损，例如第3行年份成为 `?026`，第7行加粗边界成为 `?*`。全文1268行，含1392个字面问号（此数量并不等于损坏字符数量）。尝试直接逆转首行的 GB18030/UTF-8 转换也不能无损解码；仅切换编辑器显示编码不足以恢复。

影响：下一执行者无法可靠读取最新交接及历史证据；同步两份损坏内容不代表文档验收通过。

修正要求：从正确编码副本或编辑器本地历史恢复交接，保留第28轮新增实施事实及本轮审查结论；明确使用 UTF-8 读写，再同步两边。不要用更旧的交接整体覆盖最近轮次。原始损坏文件已保存在本轮独立快照 `source/HANDOFF.md`，编码证据见 `document-integrity.json`。本轮只给根目录交接前置可读审查摘要，未猜测修复损坏历史。

### P2：新增预览断言仍不能证明预览与当前塔型一致

位置：`tests/test_gui_product.cpp:387–392`；集成目录同名文件相同行号。

新增代码仅要求 `(90,750)` 的预览像素不同于底图，不能证明普通塔原画或0.72透明度正确。三种已建塔的原画匹配不能替代预览调用入口的验证。

已实测：仅在独立审查副本，把 `GameRenderer::drawBuildPreview()` 中的 `ui.selectedTowerType` 改成固定 `TowerType::Fast`，保留实际建塔类型和所有其他路径。重新编译后 GUI 测试仍退出0；保存的 `placement_preview.png` 已与正常版不同。也就是说，选择普通塔却出现速射塔预览的回归仍会漏检。这是测试覆盖缺口，不表示当前交付产品已存在该画错问题。

修正要求：以实际 Renderer/游戏页输出，核对预览区域与所选同型塔的原画及透明度关系，覆盖 Normal/Fast/Heavy；至少让上述“普通塔预览固定为速射塔”反向对照失败。避免继续增加单点“不等于底图”断言。证据见 `negative-controls/wrong-preview-tower-run.log`、对应截图及 `results.json`。

## 上轮问题的闭环情况

| 上轮问题 | 本轮独立核对 |
| --- | --- |
| 塔头像缺失仍通过 | 已修复：单独禁用全部塔头像后退出1，报 `tower portrait does not match expected cropped artwork or is missing fallback`，说明真实原画缺失已被检出 |
| 塔型与原画映射 | 已补强：只把 Fast 映射成 Heavy 原画，退出1，报同一原画不匹配错误 |
| 菜单空卡片仍通过 | 已修复：单独取消四类图标绘制后退出1，报 `menu enemy icon does not match expected artwork or is missing (blank card detected)`，说明空卡片已被检出 |
| 升级验证 | 新增三塔升级后直径增2及徽章外缘检查；正常版本通过，未据此宣称全部等级文字已验收 |
| 集成稳定文档过时 | 交付时五份文档均与根目录逐字节相同；AGENTS/报告已使用 core/ui 职责，ARCHITECTURE已补头像缓存和共享绘制说明。HANDOFF编码损坏另列上述P2 |

## 独立验证和边界

工作目录：`D:/VS Project/data_structrue_project`。实际工具：Qt 6.10.3、MinGW g++ 13.1.0、CMake 3.30.5、Ninja 1.12.1、Python 3.12.14。工具版本与 Git HEAD 输出保存在 `environment.log`。

以实际交付生成独立快照和全新测试构建目录 `builds/Codex-Review-Port-Visuals-Followup-20261017/`。相对第27轮审查快照，产品/资源/配置没有变化，只有 `tests/test_gui_product.cpp` 发生源码变化；两工作区产品/测试/配置内容一致。因此本轮重新构建完整测试入口，没有重复构建未变化的产品 final 目标。第27轮的产品构建记录仍是历史证据。

| 检查 | 本轮实际结果 |
| --- | --- |
| 测试全新配置/编译 | 退出0，43/43，链接真实产品实现和qrc |
| CTest | 5/5，3.42秒 |
| GUI正常版另行截图 | 退出0，9张；与远端a715821的同环境参考逐张RGB像素一致 |
| 菜单源码几何检查 | 退出0，15点 |
| 反向对照 | 塔头像缺失、Fast错用Heavy原画、菜单图标缺失均退出1；预览固定Fast退出0，确认上述漏检 |
| 源码/文档差异空白检查 | 根目录排除builds及集成已跟踪diff均退出0；改动GUI测试与四份可读稳定文档无尾空白 |
| 全根目录差异空白检查 | 退出2，仍为旧构建AutogenInfo.json中MOC_SKIP/UIC_SKIP两处既有尾空白，未清理 |

反向对照仅修改审查副本，结束后逐字节恢复产品源码并重建、运行基线GUI，退出0。哈希确认交付71份文件、根目录60份产品/测试/配置均未改变，恢复后的审查快照也完全一致。错误预览与正常版截图的差异边界为 `(71,731)–(109,769)`（右下为开区间），对应预览塔身。详细结果见 `protection-results.json`、`screenshot-comparison.json` 与 `review-summary.json`。

全快照文本扫描另发现178行尾空白：历史 `项目注释要求.md` 175处、HANDOFF历史3处，未批量格式化；这不影响上述针对本轮改动的检查。明细见 `snapshot-whitespace.json`。

完整执行入口（脚本保存匹配PATH、TEMP/TMP、Qt插件路径和CMake全部参数）：

```powershell
& 'C:/Users/ZiggyS/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 -B builds/Codex-Review-Port-Visuals-Followup-20261017/prepare_review.py
& './builds/Codex-Review-Port-Visuals-Followup-20261017/verify_build.ps1' -Kind tests
& 'C:/Users/ZiggyS/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 -B builds/Codex-Review-Port-Visuals-Followup-20261017/source/tests/test_menu_geometry.py
& 'C:/Users/ZiggyS/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 -B builds/Codex-Review-Port-Visuals-Followup-20261017/check_negative_controls.py
& 'C:/Users/ZiggyS/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 -B builds/Codex-Review-Port-Visuals-Followup-20261017/finish_review.py
```

prepare_review.py 只用于首次捕获，不覆盖已存在的审查快照。构建/CTest日志、正常与反向截图、`followup-source.patch` 和编码原文保存在同目录。真实桌面字体、全部塔型等级画面及音频听感本轮未人工验收。

本轮根目录文档改动为本报告、旧报告历史标注、HANDOFF可读前置摘要及CHANGELOG；没有修改两边交付的产品/测试/配置，没有修改集成worktree文档，没有提交、推送或切换分支。下一执行者：Gemini，修复交接编码与预览断言，并同步两边；Codex随后针对这两项复核，无需重做视觉迁移。
