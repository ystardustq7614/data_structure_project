# 顶流争夺保卫战

基于 **C++17 + Qt 6 Widgets/Multimedia** 的桌面塔防游戏。通过建造、升级和出售三种防御塔，抵御六波共 90 个敌人；支持出怪预览、暂停、胜负结算与重新挑战。

当前版本已接入普通怪、快攻怪、病毒、变异体和 Boss 的新角色素材，以及菜单、说明和胜负页的新背景。

## 构建与运行

需要 CMake 3.16 或更新版本、Ninja、Qt 6（Core、Gui、Widgets、Multimedia）及与 Qt kit 匹配的 C++17 编译器。Windows 已验证 Qt 6.10.3 + MinGW 13.1.0。

在已配置好 Qt、编译器、CMake 和 Ninja 的 PowerShell 中，从仓库根目录运行。将第一行改为自己的 Qt kit 路径：

```powershell
$qtKit = 'C:/Qt/6.10.3/mingw_64'
cmake -S 26C- -B builds/product -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_PREFIX_PATH=$qtKit"
cmake --build builds/product --parallel 2
& ./builds/product/final.exe
```

程序依赖 Qt 和编译器运行库；普通终端的环境设置、双击运行所需的本地运行库复制，以及完整验证命令见[构建指南](docs/development/BUILDING.md)。构建生成物统一放在 `builds/`，不纳入交付源码。

## 操作

| 操作 | 方法 |
| --- | --- |
| 选择普通塔 / 速射塔 / 重炮塔 | `1` / `2` / `3` |
| 建造 / 选中已有塔 | 鼠标左键点击地图位置 / 塔 |
| 升级选中的塔 | `U` |
| 出售选中的塔 | `X` 或 `Delete` |
| 暂停 / 继续 | `P`、`Esc` 或右上角按钮 |
| 查看后续出怪 | 右上角“预览” |
| 放弃当前局并返回菜单 | 局内“退出” |

建造预览以绿色表示可建，红色表示道路、已有建筑或金币不足。暂停时不能建造、升级或出售；说明页按 `Esc` 返回菜单。

## 验证

使用同一套 Qt 和编译器环境：

```powershell
cmake -S tests -B builds/tests -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_PREFIX_PATH=$qtKit"
cmake --build builds/tests --parallel 2
ctest --test-dir builds/tests --output-on-failure
python -X utf8 -B tests/test_menu_geometry.py
```

CTest 包含调度器、敌人、战局、备用绘制和真实界面五组回归，界面测试采用 Qt 离屏平台。菜单脚本检查真实源码中的 15 个代表坐标；离屏测试不替代真实桌面字体、键鼠及音频听感的人工验收。最新执行证据见[交接记录](docs/collaboration/HANDOFF.md)。

## 目录与文档

| 目录 | 内容 |
| --- | --- |
| `26C-/core/` | 战局、实体、地图、配置与出怪调度 |
| `26C-/ui/` | 页面、输入、绘制和资源缓存 |
| `26C-/images/`、`26C-/music/` | 通过 Qt 资源系统嵌入的运行素材 |
| `art/` | 用户原始素材、对应清单及衍生图处理记录 |
| `tests/` | 连接真实产品代码的回归测试与几何检查 |
| `docs/` | 分类后的架构、玩法、开发、素材、协作及历史资料 |
| `builds/` | 本机构建、测试日志和截图，已忽略 |

从[文档索引](docs/README.md)继续阅读：[架构](docs/architecture/ARCHITECTURE.md)、[进攻节奏配置](docs/gameplay/ATTACK_RHYTHM.md)、[素材说明](docs/assets/enemies.md)、[变更记录](docs/collaboration/CHANGELOG.md)。协作规则保留在根目录 [AGENTS.md](AGENTS.md)；所有 Git 操作从工作区根仓库执行。
