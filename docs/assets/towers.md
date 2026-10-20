# 独立塔贴图

`26C-/images/towers/{normal,fast,heavy}.png` 保存原有三座塔在运行时使用的头像，分别为海星、黄色炮台和红色炮台。

它们由Qt按原Renderer的三个矩形从旧菜单裁出并无损保存为PNG，没有重新生成角色。解码后的像素与原裁图完全一致，塔与建造预览继续共用这些资源。

| 类型 | 原菜单裁图 `(x,y,w,h)` | 运行图 |
| --- | --- | --- |
| Normal | `(708,452,82,84)` | `26C-/images/towers/normal.png` |
| Fast | `(809,451,95,87)` | `26C-/images/towers/fast.png` |
| Heavy | `(914,448,89,91)` | `26C-/images/towers/heavy.png` |

原菜单 `26C-/images/menu_bg.jpg` 已随废弃素材清理移除；可从Git提交 `75914dc8dca1ce22ee3aa2cae1f796c2f130d429` 追溯。原图SHA256、裁图矩形和PNG哈希记录在 [art/towers/manifest.json](../../art/towers/manifest.json)。
