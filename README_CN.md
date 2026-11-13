# Contract Killer Reverse

[English](README.md) | **中文**

## 概述

本项目基于 Glu Mobile《Contract Killer》iOS 1.5.2 版本的主程序 `BountyHunter`。原版是 armv6 + armv7 的 fat 文件，本项目**只还原 armv7 切片**。

armv7 切片没有剥离符号，还保留了完整的 STABS 调试映射：763 个编译单元的源文件路径、目标文件名、每个函数的归属和原版链接顺序都有据可查。源码目录结构、文件划分和函数顺序都按这份映射还原。

本项目的目标是 **1:1 还原源码**：用原版工具链编译恢复出的源码，得到与原版 armv7 切片逐字节一致的 `BountyHunter`。修复游戏原本的 bug 不在本项目范围内，原版行为（包括原版自身的 bug）一律按原样保留。

只有源码具备极高的可信度，后续的修复和移植才有实际意义。

## 编译器、SDK 与第三方库

原版二进制里没有编译器版本记录。下表的版本依据来自逐函数的机器码比对、调试映射里的构建路径，以及二进制内的版本字符串。工具链的判定先在 CKZ 上完成，再拿到 CK 上复核：同一套工具链编出的 Lua、TinyXML 等目标文件，直接与 CK 原版比对，绝大多数函数逐字节一致。

> ✅ 已对齐　⚠️ 版本一致，但有一项无法完全核实　❓ 原版版本未确定

### 工具链

| 组件 | 原版 | 本项目 | 对齐 | 备注 |
| --- | --- | --- | :---: | --- |
| 编译器 | Apple clang 3.0 | clang-211.12（Xcode 4.2.1 4D502 自带） | ⚠️ | 按代码生成特征判定，依据见 CKZ；CK 的自家代码与第三方库同样以这个版本对齐 |
| 汇编器 | cctools as | cctools-809（同一 Xcode 的外部 `as`，clang 加 `-no-integrated-as`） | ⚠️ | 按编码判定：IT 块里低寄存器 `add` 原版是 T1 编码，clang 内置汇编器只出 T2 |
| 链接器 | ld64 | ld64-127.2（同一 Xcode） | ⚠️ | 原版最低系统版本是 iOS 3.0，用老式 LINKEDIT（`__DATA,__dyld` 节、入口 `crt1.o`）；dylib 列表和顺序取自原版；整文件尚未逐字节一致 |
| iOS SDK | — | iPhoneOS 5.0 SDK（同一 Xcode） | ⚠️ | `-miphoneos-version-min=3.0`；原版 `LC_VERSION_MIN` 里 sdk 字段为 0，无法从二进制核实 |
| 编译参数 | — | `-Os -g -fno-strict-aliasing -fvisibility=hidden`<br>C++：`-fno-rtti -fno-exceptions`<br>Objective-C++：`-fno-rtti` | ✅ | 由逐字节一致的函数反推；原版 Glu 符号全是 private extern，可证明开了 `-fvisibility=hidden` |
| 构建路径 | `/Applications/buildAgent/work/1365410bc639d1a3/` | 同左（bind mount） | ✅ | `__FILE__` 字符串和调试信息里的编译目录与原版相同 |

Xcode 4.2.1 的 clang、as 和 ld 是 x86_64 Mach-O 程序。本项目用打了 3 处补丁的 [maloader](https://github.com/shinh/maloader) 在 Linux 上直接运行它们，补丁见 `toolchain/patches/maloader.patch`。

### 第三方库

下表的“函数比对”是与原版逐函数比对的结果（一致 / 不一致）。

| 库 | 版本 | 来源 | 函数比对 | 备注 |
| --- | --- | --- | --- | --- |
| Lua | 5.1.4 | 官方源码 + Glu 定制 | 466 / 10 | 按 C++ 编译 |
| LuaBridge | 0.2 | 官方源码 + Glu 改动 | 4 / 1 | CK 仍是 0.2 的 `indexer` / `newindexer`（CKZ 已改为 `__index`） |
| TinyXML | 2.5 | 官方源码 | 114 / 0 | |
| libjpeg | 6b | 官方源码 + Glu 定制 | 152 / 0 | |
| libpng | 1.2.41 | 官方源码 + Glu 定制 | 97 / 8 | |
| libogg | 1.1.3 | 官方源码 | 28 / 1 | |
| libvorbis | 1.2.0 | 官方源码 + Glu 私改 | 133 / 0 | |
| zlib（inflate） | 1.2.3 | 官方源码 | 12 / 0 | 原版只编进了 inflate 部分 |
| Facebook iOS SDK（FBConnect） | ❓ | 官方源码 | 158 / 1 | |
| ASIHTTPRequest | ❓ | 官方源码 | 138 / 1 | |
| Reachability | ❓ | Apple 示例代码 + Glu 私改 | 12 / 0 | |
| Swerve 引擎核心 | — | **从原版二进制搬出的汇编** | 逐字节一致 | 原版是无符号的预编译库（380,452 字节），见下文说明 |
| Tapjoy | 8.1.5 | 按机器码恢复 | 166 / 266 | Glu 以源码形式编进项目；另有 124 个函数未比较 |
| PlayHaven | 2.9.0 | 按机器码恢复 | 81 / 130 | 同上；另有 48 个函数未比较 |
| AdColony、Flurry、AdMarvel、AdMob | — | 未恢复 | — | 广告/统计 SDK 的内部代码不恢复。AdColony、Flurry、AdMarvel 各有一层**不是原版代码**的替代层，由 `CK_ADS_COMPAT` 开关，默认关闭，走原版的失败回调；在 `CK_source/stubs/ads/` |

Swerve 引擎核心在原版里是预编译静态库。它以汇编源码的形式放在 `CK_source/packages/swerve/code/cocoa/libs/swerve_core.s`，按原地址重定位后与原版逐字节相同。

Glu 自己写的广告接入层（`AdManager`、`TapjoyManager`、`CPHManager` 等）属于自家代码，计入下面的结果。

## 结果

![CK 1.5.2 函数恢复概览](docs/piechart/function-overview.cn.png)

除广告 SDK 外的所有代码都完成编写，其中 90.29% 的函数和原版一样。若以字节为单位比较，有 71.52% 的字节与原版一样。

数据和链接两个层面另有校验：

- **数据级**：除广告 SDK 外的 1,751 个数据符号（常量、虚表、ObjC 元数据等）没有不一致项，94.97% 逐字节一致，其余是尾部无法核实或原版没有对应名字。
- **链接级**：链接产物里的 11,219 个源码函数，2,789 个与原版原始字节一致，另有 6,774 个仅地址不同、归一化后一致。

每个函数的状态见 [函数表](docs/function-table.cn.html)。

## Rebuild

构建在 Linux 里进行。Windows 上由脚本自动创建一个专用的 WSL 发行版，放在本仓库的 `.wsl\` 目录下，不占 C 盘，也不影响已有的 WSL 发行版。

### 环境要求

| 项目 | 要求 |
| --- | --- |
| 系统 | Windows 10 / 11（x64），已启用 WSL 2；或者 x86_64 的 Ubuntu 24.04 |
| WSL | 未安装时，在管理员 PowerShell 中运行 `wsl --install --no-distribution` 后重启 |
| Xcode 4.2.1 | **需要自备**：`installxcode_421_lion.dmg`（4D502，1.8 GB，SHA-256 `a9bb2633…a52ef835`），下载需要 Apple ID，见下文 |
| 自动下载 | Ubuntu 24.04 WSL 镜像、apt 软件包、maloader 源码 |
| 磁盘 | 约 8 GB（WSL 发行版的虚拟磁盘；解 dmg 时的临时文件删除后，虚拟磁盘不会自动缩小） |

Xcode 4.2.1 的下载方法：登录 Apple Developer 后，在浏览器中打开

```
https://developer.apple.com/services-account/download?path=/Developer_Tools/xcode_4.2.1_for_lion_21265/installxcode_421_lion.dmg
```

直接打开 More Downloads 页面的话，它会在几个页面之间无限重定向，所以要用这个地址。下载后把文件放到 `toolchain\xcode\` 下。脚本只解出其中 5 个组件包：`clang`、`iPhoneSDK5_0`，以及 ld、as 等工具所在的 `iPhoneHostSideTools`、`iPhoneSDKTools`、`DeveloperToolsCLI`。

### 步骤（Windows）

1. **准备环境**（只需做一次）：双击 `setup.bat`。如果 dmg 放在别处，在命令行中运行 `setup.bat <dmg 路径>`。脚本会：
   - 下载 Ubuntu 24.04 WSL 镜像，导入为发行版 `ck-build`（磁盘在 `.wsl\` 下）；
   - 安装编译依赖，按固定提交编译 maloader 并打补丁；
   - 核对 dmg 的 SHA-256，解出需要的 Xcode 组件，最后用 maloader 试运行 clang 和 ld 做自检。
2. **构建**：双击 `build.bat`。它会编译全部 701 个目标文件（只重编改动过的文件），再链接出 `BountyHunter`。`build.bat -c` 只编译不链接，`build.bat -B` 全部重新编译。
3. 产物在 `_output\BountyHunter`，旁边还有 `BountyHunter.sha256`、链接参数 `link.args` 和链接日志 `link.log`。

已经有一个装好这套工具链的 WSL 发行版（比如构建 CKZ 用的那个）时，不用重新 setup，设置环境变量 `CK_DISTRO=<发行版名>` 后直接运行 `build.bat`。两个项目的目录互不冲突：CK 用 `/work/ck_src`、`/work/ck_build`，Xcode 和 maloader 两边共用。

目标文件是可复现的：同一份源码强制重编后，701 个 `.o` 逐字节相同。`BountyHunter` 的哈希则每次重新编译后都会变，因为 ld64 会把每个目标文件的修改时间写进调试映射（N_OSO 条目）；除此之外的部分逐字节相同。

### 步骤（Linux）

在 x86_64 的 Ubuntu 24.04 上可以直接运行同样的脚本：

```bash
sudo bash toolchain/setup_distro.sh /path/to/installxcode_421_lion.dmg
```

```bash
sudo bash toolchain/build.sh
```

构建会使用 `/work`，并在 `/Applications/buildAgent/...` 和 `/Developer/...` 创建原版构建路径（bind mount 和符号链接），所以建议在专用的虚拟机或容器里运行。

### 运行

需要一台越狱的 armv7 iOS 设备，用产物替换 Contract Killer 1.5.2 里的主程序 `BountyHunter`。产物只含 armv7 切片，不支持 armv6 设备。已在 iPad（iOS 6.1.3）上试玩。

## 目录

| 路径 | 内容 |
| --- | --- |
| `CK_source/` | 恢复的源码，对应原版构建机根目录，`packages/`、`src/` 的相对路径与原版一致 |
| `CK_source/stubs/ads/` | 广告/统计替代层的开关 `CKAdsCompat.h` 与说明（替代层不是原版代码） |
| `toolchain/` | 环境搭建与构建脚本、maloader 补丁、原版链接顺序 `link_order.txt` |
| `docs/` | 函数表与概览图 |
| `setup.bat` / `build.bat` | Windows 一键入口 |

## 免责声明

- 本项目与 Glu Mobile 及其关联方没有任何关系，也未获得其授权或认可。
- Contract Killer 系列的名称、商标、美术、音频、关卡等游戏内容，以及原版程序，版权均归其权利人所有。
- 本仓库包含恢复出的源码、构建脚本，以及**从原版二进制生成的 Swerve 引擎核心汇编块**。不包含原版可执行文件、游戏数据文件和 Xcode。运行需要自备合法取得的游戏。
- 本项目仅用于学习、研究和保存目的，不得用于商业用途。
- 代码按“现状”提供，不提供任何形式的担保；使用风险自负。
