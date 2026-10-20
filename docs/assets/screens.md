# 菜单、说明与结算页角色替换

本轮延续已有森林主题，只更新背景图中的敌人角色。四份运行图位于 `26C-/images/screens/`，均为1536×1024的PNG，在原1200×800页面内显示；操作、命中矩形和核心战斗界面保持原有实现。

| 运行图 | 原图（已清理，Git历史可追溯） | 用途 |
| --- | --- | --- |
| `menu.png` | `26C-/images/menu_bg.jpg` | 进入菜单 |
| `help.png` | `26C-/images/help_bg.png` | 游戏说明 |
| `lose.png` | `26C-/images/lose.png` | 失败结算 |
| `win.png` | `26C-/images/win.png` | 胜利结算 |

原场景只有三只敌人示意角色，按原位置替换为用户提供的普通怪、快攻怪和Boss；病毒与变异体继续使用已接入的战斗贴图，菜单仍叠加四类真实敌人卡片。胜负页沿用庆祝/失落情绪，不替换我方三座塔。

四图通过内置 `image_gen.imagegen` 编辑，参考已确认的 `26C-/images/enemies/{bug,ddl,boss}.png`，生成文件原样复制入项目；未使用脚本修改图像像素。[art/screens/prompts.json](../../art/screens/prompts.json) 保存提示词与参考路径，[manifest.json](../../art/screens/manifest.json) 保存原图和运行图的SHA256。生成图是衍生美术，不宣称角色区域之外与原图像素逐字节一致；主要文字和按钮位置需要结合实装截图检查。

四份旧背景已按用户要求清理，可从Git提交 `75914dc8dca1ce22ee3aa2cae1f796c2f130d429` 追溯；清单中的原路径及提示词目标路径是历史输入记录。旧菜单中的三座塔头像已按原坐标无损拆到 `26C-/images/towers/`，详见[独立塔贴图](towers.md)，运行时不再依赖旧菜单。新背景、塔外观与建造预览显示效果保持。
