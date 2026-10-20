# 构建、运行与验证

以下命令均在仓库根目录的 PowerShell 中执行。产品入口为 `26C-/CMakeLists.txt`，目标为 `final`；测试入口为 `tests/CMakeLists.txt`。构建目录分别为 `builds/product/` 和 `builds/tests/`，可从源码重新生成。

## 工具链

需要 CMake 3.16 或更新版本、Ninja、支持 C++17 的编译器，以及 Qt 6 Core、Gui、Widgets、Multimedia。Qt 和编译器必须属于匹配的 kit；Qt 5 不能替代 Qt 6。

2026-10-17 本机验证使用 Qt 6.10.3、MinGW 13.1.0、CMake 3.30.5、Ninja 1.12.1。下列路径是该安装的示例，按自己的位置调整四个变量；这些路径不写入共享 CMake 或 VS Code 配置：

```powershell
$qtKit = 'C:/Qt/6.10.3/mingw_64'
$mingwBin = 'C:/Qt/Tools/mingw1310_64/bin'
$cmakeBin = 'C:/Qt/Tools/CMake_64/bin'
$ninjaBin = 'C:/Qt/Tools/Ninja'
$env:PATH = "$qtKit/bin;$mingwBin;$cmakeBin;$ninjaBin;" + $env:PATH
```

已使用匹配的 Qt 开发终端时，可以沿用其中的 PATH。更换 Qt 或编译器时使用新的构建目录，避免复用另一套 kit 的缓存。

## 编译与启动

```powershell
cmake -S 26C- -B builds/product -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_PREFIX_PATH=$qtKit" "-DCMAKE_CXX_COMPILER=$mingwBin/g++.exe"
cmake --build builds/product --parallel 2
& ./builds/product/final.exe
```

游戏窗口固定为 1200×800。图片、地图和音频通过 `resources.qrc` 嵌入，不需要从工作目录查找这些资源。资源编译单元较大，第一次编译可能耗时较长。

需要在普通资源管理器中双击启动时，可将匹配的运行库复制到本机构建目录：

```powershell
& "$qtKit/bin/windeployqt.exe" --compiler-runtime --no-translations ./builds/product/final.exe
```

这一步只准备本地运行文件，不发布程序。让工具根据程序实际链接的 Qt 库判断运行库类型；程序使用 Debug 编译配置也可能链接 kit 自带的 Release Qt 库，不要据此强制 `--debug`。复制运行库后启动仍失败时，先检查 Qt 与编译器是否匹配。

## 回归测试

```powershell
cmake -S tests -B builds/tests -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_PREFIX_PATH=$qtKit" "-DCMAKE_CXX_COMPILER=$mingwBin/g++.exe"
cmake --build builds/tests --parallel 2
ctest --test-dir builds/tests --output-on-failure
python -X utf8 -B tests/test_menu_geometry.py
```

Python 脚本只使用标准库；C++ 测试不依赖 Python。

| CTest 名称 | 检查内容 |
| --- | --- |
| `attack_scheduler` | 真实配置加载、调度、事件及旧版出怪轨迹 |
| `enemy_product` | 真实敌人的强化、变异、光环、生命映射 |
| `session_product` | 战局生命周期、经济、实体清理和结算 |
| `entity_rendering` | 无图片资源时的备用绘制 |
| `gui_product` | 真实 UI、计时器、输入、贴图、建造预览及结算 |

绘制和 GUI 两组由 CTest 自动设置 `QT_QPA_PLATFORM=offscreen`。需要保存当前界面截图时：

```powershell
$env:QT_QPA_PLATFORM = 'offscreen'
New-Item -ItemType Directory -Path ./builds/screenshots -Force | Out-Null
& ./builds/tests/test_gui_product.exe ./builds/screenshots
Remove-Item Env:QT_QPA_PLATFORM
```

先创建截图目录再传给测试程序。离屏断言和截图不等于真实桌面人工验收；字体、键鼠操作及音频听感仍需桌面检查。每轮实际命令、退出结果和限制记录在[交接文档](../collaboration/HANDOFF.md)。

## 出怪配置与目录维护

构建时会复制 `attack_rhythm.example.json` 到可执行文件目录。需要调整节奏时将其复制为同目录的 `attack_rhythm.json`，新局重新读取；不存在时使用内置配置，非法配置整份回退。字段和限制见[进攻节奏说明](../gameplay/ATTACK_RHYTHM.md)。

VS Code 的 `.vscode/settings.json` 使用 `${workspaceFolder}/26C-` 定位源码，并按 kit 和配置生成自己的 `builds/` 子目录，与上述命令行目录各自独立。`builds/` 仅存放生成物；源码在 `26C-/`，测试在 `tests/`，验证结论写入 `docs/`。不要在构建目录中保存唯一的源码或文档副本。
