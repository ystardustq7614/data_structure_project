# 敌人素材对应与适配

用户于2026-10-17按下表顺序提供五张JPG。`source/` 保存原文件字节，`manifest.json` 记录原始SHA256；原图不进入Qt运行资源。

| 顺序 | 游戏类型 | 素材 | 运行贴图 |
| --- | --- | --- | --- |
| 1 | 普通怪 `bug` | `source/bug.jpg` | `26C-/images/enemies/bug.png` |
| 2 | 快攻怪 `ddl` | `source/ddl.jpg` | `26C-/images/enemies/ddl.png` |
| 3 | 病毒 `virus` | `source/virus.jpg` | `26C-/images/enemies/virus.png` |
| 4 | 病毒变异形态 `mutant` | `source/mutant.jpg` | `26C-/images/enemies/mutant.png` |
| 5 | Boss `boss` | `source/boss.jpg` | `26C-/images/enemies/boss.png` |

运行PNG是通过内置 `image_gen.imagegen` 背景提取得到的透明衍生图；处理提示词保存于 `background-extraction-prompts.json`，原JPG保持不变。它们是派生素材，不宣称与原图前景像素逐字节相同。病毒、变异体和Boss保留头像/半身构图，不增加行走动画。

展示层 `EnemyArtwork::load()` 在加载时裁去透明留白并等比缩放，结果由各界面缓存；不改变原图或战斗实体。战斗普通怪/快攻怪/病毒/变异体使用65×65包围框，Boss使用75×75包围框，贴图底边对齐原占位底边，生命条位置不变。菜单仍展示四个基础类型，变异体由真实病毒变异时切换。

Qt资源只引用五张新PNG。已按用户要求删除原五张旧敌人素材，Git历史保留其来源；用户本轮提供的五份原始JPG继续保存。未改变敌人速度、血量、护甲、技能、奖励或波次。
