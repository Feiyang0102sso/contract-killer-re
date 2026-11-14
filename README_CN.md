# Contract Killer Reverse

[English](README.md) | **中文**

## 概述

本项目在 iOS 版代码的基础上做 PC 端重建。

游戏源码来自对 iOS 1.5.2 版主程序 `BountyHunter` 的 1:1 还原，见 [iOS 原版还原（`ori_IOS` 分支）](https://github.com/Feiyang0102sso/contract-killer-re/tree/ori_IOS)。在 PC 上按 x86 原生编译，原版依赖 iOS 的部分由下面的桥接层替换。

## 桥接层

游戏代码（`CK_source/src/`、`CK_source/packages/`）原样编译，必要的改动用 `#ifdef CK_PC` 包住并注释 `PORT:`。桥接层在 [`CK_source/pc_port/`](CK_source/pc_port/)，负责替换 iOS 平台相关的部分：

| 原版 iOS | PC 版 | 位置 |
| --- | --- | --- |
| 应用生命周期、触摸 | SDL3 窗口和事件；鼠标键盘转换成原版的触摸操作 | `platform_sdl/Applet.cpp`、`Controls.cpp` |
| OpenGL ES 1.1 | 桌面 OpenGL 兼容上下文。游戏画进 1024×768 的帧缓冲，再等比缩放到窗口 | `platform_sdl/GlesDesktop.cpp`、`Graphics.cpp` |
| OpenAL | OpenAL Soft | `platform_sdl/Audio.cpp` |
| 片头视频播放器 | Windows 自带的 Media Foundation 解码，左键点一下跳过 | `platform_sdl/Movie.cpp` |
| 系统字体（动态文字） | Windows 系统字体；原版位图字体不变 | `platform_sdl/SystemFont.cpp` |
| 文件与存档 | Win32 宽字符路径，可以装在中文目录；存档写到 EXE 同目录的 `saves/` | `platform_sdl/Files.cpp` |
| 广告、统计、联网服务、内购 | 空实现，按离线模式运行 | `stubs/` |
| Swerve 3D 引擎 | 两套可切换的后端，见下文 | `swerve_compat/`、`swerve_native/` |

### m3g 引擎

原版的 3D 引擎 Swerve 是没有源码的 ARM 预编译库。游戏只通过函数表调用它：每个对象的第一个字指向一张函数表，游戏按槽位调用。所以只要在同样的槽位放上自己的函数，就能整个换掉引擎。

CK 用的是比 CKZ 更老的一版 Swerve，槽位布局和 CKZ 不同。槽号不靠猜：[`make_swerve_slots.py`](scripts/make_swerve_slots.py) 解读原版引擎核心里每个类的虚表初始化函数，生成“槽号 → 原函数名”的 [`SwerveSlots.h`](CK_source/pc_port/swerve_native/SwerveSlots.h)，原生后端按原函数名填槽。

`BountyHunter.cfg` 的 `[graph] Engine` 决定用哪个后端：

| 值 | 后端 | 说明 |
| --- | --- | --- |
| `m3g`（默认） | 原生 m3gcore | 在 Nokia M3G 1.1 的 C 实现 [m3gcore](CK_source/pc_port/3rd_party/m3gcore/) 上，按 Swerve 的对象接口重写（`swerve_native/`），x86 原生运行，帧率高 |
| `swerve` | 原版 Swerve | Unicorn 在进程内模拟执行原版的 ARMv7 引擎核心 `swerve-arm.bin`（`swerve_compat/`；构建时由 `swerve_core.s` 汇编而成），行为与原版一致，但较慢，主要用于对照 |

两个后端在游戏全部 234 个 M3G 上逐项对拍过（场景数据、动画、拾取、渲染画面），结果和已知差异见 [`tests/README.md`](tests/README.md)。

## Windows x86 构建

### 工具链

| 组件 | 版本 | 获取方式 |
| --- | --- | --- |
| Visual Studio | 2022 或 2026 | 自行安装，勾选 Windows C++ x86 工具和 Windows SDK；提供 MSBuild、头文件、库和 `rc.exe` |
| Python | 3.11 以上 | 自行安装；优先使用仓库或上级目录的 `.venv`，也可以用 MSBuild 属性 `CkPython` 指定 |
| LLVM（`clang-cl`、`lld-link`） | 21.1.8 | 首次构建时由 [`prepare_toolchain.py`](scripts/prepare_toolchain.py) 自动下载、校验，装到 `.cache/deps/`，需要联网 |
| SDL3、OpenAL Soft、Unicorn、m3gcore、GoogleTest | 见 [`3rd_party/README.md`](CK_source/pc_port/3rd_party/README.md) | 已随仓库提供 |

双击根目录的 `build_pc.bat` 即可构建，它会通过 `vswhere` 找到已安装的 Visual Studio。也可以打开 `vs/ck_pc.sln`，选择 `Release | Win32` 构建。

产物是 `bin/Win32/Release/BountyHunter.exe`。构建会自动把 `SDL3.dll`、`OpenAL32.dll`、`BountyHunter.cfg` 放到 EXE 同目录，并由 [`make_swerve_image.py`](scripts/make_swerve_image.py) 从 `swerve_core.s` 汇编出 `swerve-arm.bin`。

`build_pc.bat tests` 构建测试程序 `BountyHunter_tests.exe`，见 [`tests/README.md`](tests/README.md)。

### 游戏素材

素材不随仓库提供，需要从原版 ipa（`Payload/BountyHunter.app/`）里取出，手动复制到 `bin/Win32/Release/assets/`：

| 文件 | 内容 |
| --- | --- |
| `BountyHunter.big` | 游戏数据包 |
| `xga.big` | 1024×768 图形包 |
| `vga.big` | 960×640 图形包，1024×768 下也会用到 |
| `en.big` | 英文文本包 |
| `res_map.dat` | 资源名映射表 |
| `glu_logo_landscape.m4v` | Glu 片头视频 |
| `ck_logo_1024x768.m4v` | Contract Killer 片头视频 |

所需文件统一定义在 [`AssetPaths.h`](CK_source/pc_port/platform_sdl/AssetPaths.h)。启动时逐个检查，缺哪个就报哪个的完整路径。片头还会播放 `glu_logo_audio.wav`，建议把 app 里的其余素材文件也一并复制。

素材目录和存档目录也可以在命令行指定：`BountyHunter.exe [素材目录] [存档目录]`。

运行需要支持桌面 OpenGL 兼容上下文和帧缓冲对象的显卡驱动，以及 x86 版 Visual C++ 运行库。

## CFG配置

设置写在 EXE 同目录的 `BountyHunter.cfg`。

| 键 | 默认 | 作用 |
| --- | --- | --- |
| `[common] Title` | `Contract Killer` | 窗口标题 |
| `[graph] Engine` | `m3g` | 3D 引擎：`m3g` 为原生 m3gcore；`swerve` 为 Unicorn 模拟的原版 Swerve，较慢，用于对照 |
| `[graph] WindowWidth` / `WindowHeight` | `2048` / `1536` | 窗口尺寸，超出桌面时按比例缩小 |
| `[graph] FullScreen` | `0` | 无边框全屏，画面保持 4:3，两侧留黑边 |
| `[graph] VSync` | `1` | 垂直同步 |
| `[game] DrawFPS` | `1` | 在画面左上角显示帧率 |
| `[debug] DebugMode` | `0` | 打开控制台黑框，并启用调试热键 F9（导出下一帧） |
| `[debug] FpsLog` | `0` | 每秒记录帧率、帧耗时和 CPU 时间，需 `DebugMode=1` |

## 键位配置

战斗中鼠标被锁定并隐藏；菜单和地图里左键就是点击。键位定义在 [`KeyBindings.h`](CK_source/pc_port/platform_sdl/KeyBindings.h)，改键后重新编译即可。

| 输入 | 操作 |
| --- | --- |
| 移动鼠标 | 转动视角 |
| 鼠标左键 | 开火（按住） |
| 鼠标右键 | 开镜 / 关镜 |
| 滚轮 | 开镜后缩放 |
| `Shift` | 屏息稳定（按住） |
| `R` | 装弹 |
| `Q` / `E` | 切换武器 |
| `C` | 进入 / 离开掩体 |
| `H` | 医疗包 |
| `F` | 慢动作药片 |
| `G` | 超声波发射器 |
| `Esc` | 暂停 |

## 作弊码

游戏加载完后直接输入字母即可，没有输入框。`C`、`H` 同时是战斗按键，建议在地图界面输入。

| 作弊码 | 效果 |
| --- | --- |
| `chm` | 现金 +9,999,999，黄金设为 99,999（地图只显示五位） |
| `chxp` | 等级 +1 |
| `chxxp` | 等级 +10 |
| `chxxxp` | 直接满级（70） |

## 免责声明

- 本项目与 Glu Mobile 及其关联方没有任何关系，也未获得其授权或认可。
- Contract Killer 系列的名称、商标、美术、音频、关卡等游戏内容，以及原版程序，版权均归其权利人所有。
- 本仓库包含恢复出的源码、构建脚本，以及**从原版二进制生成的 Swerve 引擎核心汇编块**。不包含原版可执行文件和游戏数据文件，运行需要自备合法取得的游戏素材。
- 本项目仅用于学习、研究和保存目的，不得用于商业用途。
- 代码按“现状”提供，不提供任何形式的担保；使用风险自负。
