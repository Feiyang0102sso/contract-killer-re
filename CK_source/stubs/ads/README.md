# 广告与统计接入

本目录保留 AdColony 头文件的转发路径。广告及统计代码位于 `CK_source/packages/` 下各服务目录。应用从 `Info.plist` 读取下列配置。

| 服务 | 配置 | 当前行为 |
| --- | --- | --- |
| PlayHaven | `CKPlayHavenBaseURL`、`CKPlayHavenToken`、`CKPlayHavenSecret`。`CKPlayHavenInstallationID` 用于安装标识及奖励签名验证。 | 请求内容和通知角标，显示及关闭内容，处理子内容、跳转与奖励回执。 |
| Tapjoy | `CKTapjoyBaseURL`、`CKTapjoyAppID`、`CKTapjoySecret`、`CKTapjoyInstallationID`。可选 `CKTapjoyAlternateBaseURL`、`CKTapjoyPadAppID`、`CKTapjoyUserID` 和布尔值 `CKTapjoyVideoEnabled`。 | 查询积分、展示横幅和广告墙、查询精选应用、缓存及播放视频。积分变更以服务响应为准。 |
| AdMarvel | `CKAdMarvelBannerURL`、`CKAdMarvelInterstitialURL`，可分别配置。 | 向 HTTPS 网关发送 `site_id` 和 `partner_id`，接收图片内容及可选的 `X-AdMarvel-Click-URL` 响应头。 |
| AdColony | `CKAdColonyAppID`、`CKAdColonyZoneIDs` 字符串数组。 | 使用已加载的旧版 SDK 配置广告区，查询视频和奖励。第一个有效广告区为默认区。 |
| Flurry | 沿用游戏已有会话入口。 | 保存最近 256 条本地事件，可按会话设置写入 `Library/Caches/CKFlurrySession.plist`。 |

网络服务需要可用的 HTTPS 端点和对应账户配置。当前构建提供 AdMarvel 网关和 Flurry 本地统计兼容层。AdColony 旧版 SDK 需另行提供。恢复日志记录了各批 ARMv7 编译、正式链接和函数比对结果。广告服务与界面尚未在设备上联调。

## 恢复到此为止（2026-11-11）

广告/统计 SDK 不再继续恢复。已按原版重写的 Tapjoy、PlayHaven 保持现状，不再追加函数，也不再追 DIFF。

AdMarvel、Flurry、AdColony 三处替代层**不是原版代码**，由 [`CKAdsCompat.h`](CKAdsCompat.h) 的 `CK_ADS_COMPAT` 统一开关：

| 替代层 | `CK_ADS_COMPAT 0`（默认） | `CK_ADS_COMPAT 1` |
| --- | --- | --- |
| `packages/AdManager/code/cocoa/native/AdMarvelView.m` | 不发请求，立即走原版的失败回调 | 上表的 HTTPS 网关 |
| `packages/flurry/code/cocoa/native/FlurryAnalytics.m` | 5 个接口全部空实现 | 上表的本地会话记录 |
| `packages/adcolony/src/AdColony_Facade.mm` 的 `initFromBundle` | 直接返回，AdColony 始终未就绪，商店入口显示“不可用” | 读取 `CKAdColony*` 配置并在运行时查找旧版 SDK |

打开方式：编译时加 `-DCK_ADS_COMPAT=1`，或改 `CKAdsCompat.h` 里的默认值。

账本里剩下的广告 SDK 函数（AdColony、Flurry、AdMarvel 的 SDK 内部代码、`b_prebuilt`，以及 `0x181a34–0x218854` 的 AdMob 等无符号段）只被上述接入层调用，不恢复，也不补空实现。
