# Contract Killer Reverse

**English** | [中文](README_CN.md)

## Overview

Source recovery of `BountyHunter`, the main executable of Glu Mobile's *Contract Killer* for iOS, version 1.5.2. The original is a fat armv6 + armv7 file; this project **recovers the armv7 slice only**.

The armv7 slice is not stripped and keeps its full STABS debug map: the source path, object name and function ownership of all 763 compile units, plus the original link order. The source tree, file split and function order follow that map.

The goal is a **1:1 source recovery**: compiling the recovered source with the original toolchain must give a `BountyHunter` that is byte-identical to the original armv7 slice. Original behavior, bugs included, is kept as is.

The sister project [Contract Killer: Zombies Reverse](https://github.com/Feiyang0102sso/contract-killer-zombie-re) uses the same Glu engine and the same toolchain. The two repositories are independent.

> The original toolchain links an armv7 candidate. Gameplay and interface recovery and per-function verification are ongoing.

## Toolchain

Same as CKZ: Xcode 4.2.1 (4D502) clang-211.12, its external cctools-809 `as`, ld64-127.2 and the iPhoneOS 5.0 SDK, run on Linux through a patched [maloader](https://github.com/shinh/maloader). Flags: `-Os -g -fno-strict-aliasing -fvisibility=hidden -miphoneos-version-min=3.0`, C++ `-fno-rtti -fno-exceptions`, Objective-C++ `-fno-rtti`. Build root `/Applications/buildAgent/work/1365410bc639d1a3/` (bind mount), as in the original debug map.

## Status

![CK 1.5.2 function recovery overview](docs/piechart/function-overview.en.png)

| Item | State |
| --- | --- |
| Sources | All tracked Glu gameplay, engine, and ad integration functions have compiled counterparts. The formal link uses 699 object files, including the Swerve engine core assembly |
| Per-function verification | The function table tracks normalized object comparisons and independently verified assembly ranges, deduplicated by original address |
| Swerve engine core | a prebuilt library in the original; lifted from the original binary as assembly (`packages/swerve/code/cocoa/libs/swerve_core.s`), byte-identical (380,452 B) after relocation |
| Ported from CKZ | the platform layer, plus gServe and gameplay units whose original machine code is at least 20% identical to CK's; namespaces removed; CApp, Window, the 2D graphics interface and the resource manager already follow CK |
| Link | `ld64` produces `_output/BountyHunter`. The candidate still differs from the original armv7 slice. On-device runtime verification is pending |

Per-function status: [function table](docs/function-table.en.html).

## Build

Windows: put your own `installxcode_421_lion.dmg` in `toolchain\xcode\`, run `setup.bat` once, then `build.bat` (`-c` compiles only, `-B` rebuilds everything). The product is `_output\BountyHunter`. If you already have a WSL distro with this toolchain (for example the CKZ one), skip setup and set `CK_DISTRO=<distro name>`.

Linux (x86_64 Ubuntu 24.04): `sudo bash toolchain/setup_distro.sh <dmg>`, then `sudo bash toolchain/build.sh`.

## Disclaimer

This project is not affiliated with or endorsed by Glu Mobile. All game content and the original program belong to their owners. The repository contains recovered source and build scripts only, no original executable, game data or Xcode. For study, research and preservation only; provided as is, without warranty.
