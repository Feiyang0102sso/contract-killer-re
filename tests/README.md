# 测试

`tests/` 是 PC 移植的测试程序 `BountyHunter_tests.exe`（工程 [`vs/ck_tests.vcxproj`](../vs/ck_tests.vcxproj)），照 CKZ 的测试程序搭建，用 [GoogleTest 1.15.2](../CK_source/pc_port/3rd_party/googletest-1.15.2/) 写断言。它和游戏放在同一个目录，共用素材、存档和 `BountyHunter.cfg`，日志写在同目录的 `BountyHunter_tests.log`。

```bat
build_pc.bat tests
```

## 运行

| 命令 | 作用 |
| --- | --- |
| `BountyHunter_tests.exe` | 跑全部自动化测试 |
| `BountyHunter_tests.exe --gtest_filter=*M3gBackends*` | 两个 Swerve 后端的逐文件对拍（234 个 M3G，约 15 分钟） |
| `BountyHunter_tests.exe --gtest_filter=*M3gRender.*` | 两个后端画出来的画面逐文件对拍 |
| `BountyHunter_tests.exe --gtest_filter=*vga_*` | 只对拍 `vga.big` 的 33 个 M3G（数据和画面，约 3 分钟） |
| `BountyHunter_tests.exe --gtest_list_tests` | 列出所有测试（输出在控制台，不进日志） |
| `BountyHunter_tests.exe --play` | 正常玩游戏，附带 [`tests_tools/TestTools.h`](tests_tools/TestTools.h) 的工具 |

自动化测试**在后台运行**：窗口启动即最小化，任务栏上有它的按钮，右键关闭就会立刻结束整轮测试；声音走 OpenAL Soft 的空设备，全程静音。帧率由测试程序限制在 60 帧（窗口最小化后垂直同步不再起作用）。

`--play` 默认是前台、有声音。设环境变量 `CK_HIDDEN=1` 可以让它也进入后台模式（最小化 + 静音），`CK_MUTE=1` 只静音。

| 环境变量 | 作用 |
| --- | --- |
| `CK_ENGINE=swerve` | 覆盖 `BountyHunter.cfg` 的 `Engine` |
| `CK_DIFF_ALL=1` | 对拍时输出每一条差异（默认每个快照只列前 12 条），排查单个文件时用 |
| `CK_ARM_PROFILE=1` | 原版 Swerve（`Engine=swerve`）的热点分析，F8 开始/停止，结果写 `arm_profile.txt`、`arm_callers.txt` |
| `CK_M3G_LOADTEST=<目录>` | 不启动游戏，用 m3gcore 加载目录里所有 `.m3g` 并逐个打印结果 |

想同时跑两个实例，把 exe 复制成另一个名字再运行：日志按 exe 名分开，重新编译时也不会因为 exe 被占用而链接失败。

测试结果在日志里：每个测试一行 `[PASS]`/`[FAIL]`/`[SKIP]`，失败时上面有 gtest 的详细输出；访问违例会记下模块偏移和栈顶，可以用 `llvm-symbolizer --obj=BountyHunter_tests.exe 0x<偏移+0x10000000>` 还原。

## 测试内容

### `M3gBackendTest.cpp`：原生 m3gcore 与原版 Swerve 对拍

对 `BountyHunter.big`（201 个）和 `vga.big`（33 个）里的每个 M3G（其中 78 个是 Swerve 扩展格式），分别用原生后端和 Unicorn 里的原版 Swerve 加载（`loader_loadBuffer`，与 `CSwerve::LoadM3GRoot` 相同），再用游戏调用的同一批槽位逐项比较。以原版为准，任何差异都算原生后端的错误：

- 节点类型、userID、子节点、动画轨道（控制器权重/速度/位置、序列时长/重复模式）
- 平移、缩放、朝向（换算成四元数比较）、`getTransform`（游戏 tws.h 叫它 getCompositeTransform）与真正的合成矩阵、到根节点的矩阵、可渲染/可拾取开关、相机投影
- 网格数据：顶点数、位置缩放偏移和数值、每个子网格的三角形索引（游戏的碰撞网格 `CCollision::BuildMesh` 就从这里来）
- 在 5 个世界时间 `animate()` 之后，再比较上面全部内容
- World 根节点：经活动相机在 9×9 屏幕网格上的拾取（`group_pickCamera`，射击判定用），以及从相机出发的射线拾取（`group_pickNode`，`SwerveHelper::RayPick` 用）；比较是否命中、距离、命中节点、法线和射线

另有「单控制器」阶段：重新加载场景，每次只把一个动画控制器权重设为 1、从小数序列位置 0.6 开始播放，在 95/455/1305 ms 后比较（游戏 `CDH_Animation` 就是这样播动画的）。

所有调用都经 `ck::arm::Method` 和 [`SwerveSlots.h`](../CK_source/pc_port/swerve_native/SwerveSlots.h) 的槽号（由 `scripts/make_swerve_slots.py` 从原版引擎的 vtable 初始化函数解出），ARM 对象和原生对象走同一份代码；槽号若有错，原版那边会读出别的函数的结果，对拍就会报差异。

比较规则：

- 数值按相对误差 2e-3；四元数 q 与 -q 视为相同；动画轨道按集合比较（顺序两边不同，游戏不依赖顺序）。
- **不比较返回码**：CK 的 Swerve 里简单的 getter（`object3d_getUserID`、`node_isPickingEnabled` 等）不设置 r0，原版的“返回码”是寄存器里的残值，游戏也从不读它。
- 拾取结果里命中与否、距离、射线严格比较，法线允许约 2°；两边命中**同一距离的不同节点**（墙上的贴花等共面面片）记为平局并在日志里计数，不算失败。

需要 `BountyHunter.cfg` 的 `Engine=m3g` 和 exe 旁边的 `swerve-arm.bin`（构建时生成）。整轮约 15 分钟。

2026-11-13 的结果：234 个文件中 208 个完全一致。对拍找到并已修正的差异：SkinnedMesh 拾取按绑定姿势（CK 补丁，见 `CK_source/pc_port/3rd_party/README.md`）。剩下 26 个文件的差异分三类，都没有模拟：

| 类别 | 文件 | 现象 | 原因 |
| --- | --- | --- | --- |
| 拾取精度 | 8 KB 的小模型（`0037d4ac` 等）及若干关卡里的个别点 | 同一网格命中距离差约 0.2%、插值法线差约 0.3°；两个面挨得很近时最近命中的节点不同 | 原版拾取精度较低：用网格顶点精确求交，原生与精确值一致（差 1e-6），原版偏 0.2% |
| 隐藏特效片的拾取 | `0b23756c`、`06dd6383`、`0c6127e9`、`09931c52` | 动画后，原生命中 alpha 因子为 0、渲染关闭的组下的叠加发光四边形，原版没命中 | 规则未查明：同样结构（alpha 0、渲染关闭、ALPHA_ADD）在别的文件里原版照样拾取；按 alpha 或混合模式跳过都会让更多文件不一致 |
| 跨近 360° 的四元数插值 | `0f2718a3`、`vga_05d4965c` 等（朝向、合成矩阵、到根节点矩阵） | 同一 SLERP 段原版转得比原生慢约 9%（原版 353.62° 对原生 353.14°） | 两个关键帧几乎互为相反数（点积 -0.99998），SLERP 走长路，sinθ 极小，结果对点积末位极敏感；原版等效 θ = 3.1343，精确值 3.1353 |

### `M3gRenderTest.cpp`：两个后端画出来的画面对拍

同样对 234 个 M3G，各用两个后端画进同一个 320×240 的离屏帧缓冲（流程同 `CSwerveGraphics3D::BindTarget`：`setCroppedBackBufferData` 后 `renderWorld`），逐像素比较。CK 没有固定的生存模式场景可以用来测模型，所以：

- 根节点是 World 的文件（关卡、武器视图等）用它自己的活动相机画；
- 其他模型挂进一个**占位 World**：相机按模型包围盒摆在正前方，加一盏环境光、一盏沿视线的平行光。

每个场景画两个机位（原位、绕 Y 轴转 1.5°）。某颜色通道差 48 以上算一个不同像素，超过 2% 就失败。World 还要过一项**稳定性**检查：相机每次转 0.02°（远小于一个像素）连画 6 帧，分别统计两个后端相邻两帧间变化的像素比例；原生明显比原版跳得多，就说明有闪烁（绘制顺序或深度竞争随视锥剔除结果变化）。

| 环境变量 | 作用 |
| --- | --- |
| `CK_RENDER_SAVE=1` | 保存每张画面；不设时只保存差异超过 0.2% 的。保存在 exe 旁边的 `render_compare\<文件>_root<n>_pose<n>.bmp`，左原版、中原生、右差异 |
| `CK_RENDER_ISOLATE=<文件名>` | 运行 `M3gRenderIsolate.DifferingMeshes`：World 里的网格逐个单独画，列出两边差异最大的网格及其外观（混合、alpha 阈值、深度写、图层、剔除、材质、纹理格式） |
| `CK_RENDER_ACCUMULATE=<文件名>` | 运行 `M3gRenderIsolate.AccumulatingMeshes`：按遍历顺序逐个打开网格，在差异跳升的那一步存图并列出网格；单独画一致、合起来不一致的问题（绘制顺序、状态遗留）靠它定位。再设 `CK_RENDER_LIST=1` 列出每一步 |

文件名就是测试参数名，如 `vga_m3g_05d4965c`。

对拍找到并修正：m3gcore 渲染队列在同一图层、同一混合类别内按渲染状态哈希排序，同键物体的位置取决于视锥放进来多少物体，镜头一动顺序就变；CK 关卡的树是“ALPHA 混合 + 写深度 + 不做 alpha 测试”的面片，先画的树连透明纹素也写深度、挡住后画的树，于是树叶一闪一闪。改为：不透明物体保持场景图顺序，半透明物体按节点深度由远到近（CK 补丁，`m3g_renderqueue.c`）。

2026-11-13 的结果：234 个中 224 个通过；110 个单独模型（占位场景）两边像素完全一致；稳定性检查里原生没有一个比原版跳得多。没通过的 10 个是 5 个大关卡在两个包里的各一份（`0a216e16`/`vga_000b3af6`、`0a545f6b`/`vga_00aca9d4`、`0c1dc22d`/`vga_03ec02a2`、`0f2718a3`/`vga_05d4965c`、`09651973`/`vga_06ff4eb4`），差异 2%–5%。

剩下的差异：关卡 World 有 0.1%–5% 的像素不同，主要是**共面面片的深度竞争**（两块几乎重合的面谁赢取决于深度值的末位，两边矩阵运算精度不同；原版里本身就是点状闪烁的那种），以及树叶整体亮度略有差别。每个网格单独画时两边一致（`CK_RENDER_ISOLATE` 验证）。

### 尚未接到 CK 的部分

CKZ 测试程序里驱动游戏流程的部分（`GameHarness` 从地图进生存模式、`GameplayTest` 的游戏内断言、`TestTools` 的自动开局和爆头拾取自检）依赖 CKZ 的地图和关卡结构，还没有按 CK 重写；`GameHarness` 目前只负责启动游戏和推进帧，`TestTools` 只有 F8 的 ARM 热点分析。
