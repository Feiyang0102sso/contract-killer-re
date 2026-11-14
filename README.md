# Contract Killer Reverse

**English** | [中文](README_CN.md)

## Overview

This project rebuilds the game for PC on top of the iOS source code.

The game source is a 1:1 reconstruction of the main program `BountyHunter` from the iOS 1.5.2 release, see the [original iOS reconstruction (`ori_IOS` branch)](https://github.com/Feiyang0102sso/contract-killer-re/tree/ori_IOS). On PC it is compiled natively for x86. The parts of the original that depend on iOS are replaced by the bridge layer below.

## Bridge layer

The game code (`CK_source/src/`, `CK_source/packages/`) is compiled as is; the few changes it needs are wrapped in `#ifdef CK_PC` and marked `PORT:`. The bridge layer lives in [`CK_source/pc_port/`](CK_source/pc_port/) and replaces everything that is specific to the iOS platform:

| Original iOS | PC version | Location |
| --- | --- | --- |
| App lifecycle, touch | SDL3 window and events; mouse and keyboard are turned into the original touch controls | `platform_sdl/Applet.cpp`, `Controls.cpp` |
| OpenGL ES 1.1 | Desktop OpenGL compatibility context. The game draws into a 1024×768 framebuffer, which is scaled to the window keeping its aspect ratio | `platform_sdl/GlesDesktop.cpp`, `Graphics.cpp` |
| OpenAL | OpenAL Soft | `platform_sdl/Audio.cpp` |
| Intro movie player | Decoded by Windows' built-in Media Foundation; one left click skips it | `platform_sdl/Movie.cpp` |
| System font (dynamic text) | Windows system font; the original bitmap fonts are unchanged | `platform_sdl/SystemFont.cpp` |
| Files and saves | Win32 wide-character paths, so the game can live in a non-ASCII directory; saves go to `saves/` next to the EXE | `platform_sdl/Files.cpp` |
| Ads, analytics, online services, in-app purchases | Empty implementations, the game runs offline | `stubs/` |
| Swerve 3D engine | Two switchable backends, see below | `swerve_compat/`, `swerve_native/` |

### m3g engine

The original 3D engine, Swerve, is a precompiled ARM library without source. The game only calls it through function tables: the first word of every object points to a table of functions, and the game calls them by slot. Putting our own functions in the same slots therefore replaces the whole engine.

CK uses an older Swerve than CKZ, with a different slot layout. The slot numbers are not guessed: [`make_swerve_slots.py`](scripts/make_swerve_slots.py) reads the vtable initialiser of every class in the original engine core and generates [`SwerveSlots.h`](CK_source/pc_port/swerve_native/SwerveSlots.h) (slot number → original function name); the native backend fills each slot by function name.

`[graph] Engine` in `BountyHunter.cfg` selects the backend:

| Value | Backend | Notes |
| --- | --- | --- |
| `m3g` (default) | Native m3gcore | Swerve's object interface reimplemented (`swerve_native/`) on [m3gcore](CK_source/pc_port/3rd_party/m3gcore/), the C implementation of Nokia M3G 1.1. Runs natively on x86 at a high frame rate |
| `swerve` | Original Swerve | Unicorn emulates, inside the process, the original ARMv7 engine core `swerve-arm.bin` (`swerve_compat/`; assembled from `swerve_core.s` by the build). Behaves exactly like the original but is slower; mainly used for comparison |

Both backends have been compared on all 234 M3G files of the game (scene data, animation, picking and rendered pictures); results and known differences are in [`tests/README.md`](tests/README.md).

## Windows x86 build

### Toolchain

| Component | Version | How to get it |
| --- | --- | --- |
| Visual Studio | 2022 or 2026 | Install it yourself with the Windows C++ x86 tools and the Windows SDK; it provides MSBuild, headers, libraries and `rc.exe` |
| Python | 3.11 or later | Install it yourself; a `.venv` in the repository or its parent directory is preferred, or set the MSBuild property `CkPython` |
| LLVM (`clang-cl`, `lld-link`) | 21.1.8 | Downloaded, verified and installed to `.cache/deps/` by [`prepare_toolchain.py`](scripts/prepare_toolchain.py) on the first build; needs internet access |
| SDL3, OpenAL Soft, Unicorn, m3gcore, GoogleTest | See [`3rd_party/README.md`](CK_source/pc_port/3rd_party/README.md) | Included in the repository |

Double-click `build_pc.bat` in the repository root to build; it finds the installed Visual Studio through `vswhere`. You can also open `vs/ck_pc.sln` and build `Release | Win32`.

The result is `bin/Win32/Release/BountyHunter.exe`. The build puts `SDL3.dll`, `OpenAL32.dll` and `BountyHunter.cfg` next to the EXE, and [`make_swerve_image.py`](scripts/make_swerve_image.py) assembles `swerve-arm.bin` there from `swerve_core.s`.

`build_pc.bat tests` builds the test program `BountyHunter_tests.exe`, see [`tests/README.md`](tests/README.md).

### Game assets

The assets are not included. Take them from the original ipa (`Payload/BountyHunter.app/`) and copy them to `bin/Win32/Release/assets/` by hand:

| File | Contents |
| --- | --- |
| `BountyHunter.big` | Game data archive |
| `xga.big` | 1024×768 graphics archive |
| `vga.big` | 960×640 graphics archive, also used at 1024×768 |
| `en.big` | English text archive |
| `res_map.dat` | Resource name map |
| `glu_logo_landscape.m4v` | Glu intro movie |
| `ck_logo_1024x768.m4v` | Contract Killer intro movie |

The required files are all listed in [`AssetPaths.h`](CK_source/pc_port/platform_sdl/AssetPaths.h). Startup checks each of them and reports the full path of every missing one. The intro also plays `glu_logo_audio.wav`, so copying the app's other asset files along is recommended.

The assets and saves folders can also be given on the command line: `BountyHunter.exe [assets folder] [saves folder]`.

Running needs a graphics driver with a desktop OpenGL compatibility context and framebuffer objects, plus the x86 Visual C++ runtime.

## Configuration

Settings are read from `BountyHunter.cfg` next to the EXE.

| Key | Default | Effect |
| --- | --- | --- |
| `[common] Title` | `Contract Killer` | Window title |
| `[graph] Engine` | `m3g` | 3D engine: `m3g` is the native m3gcore; `swerve` is the original Swerve emulated by Unicorn, slower, used for comparison |
| `[graph] WindowWidth` / `WindowHeight` | `2048` / `1536` | Window size, scaled down proportionally when larger than the desktop |
| `[graph] FullScreen` | `0` | Borderless full screen; the picture stays 4:3 with black bars at the sides |
| `[graph] VSync` | `1` | Vertical sync |
| `[game] DrawFPS` | `1` | Show the frame rate in the top-left corner |
| `[game] KeyHints` | `1` | Show the bound key on each mission HUD button (a blood splat badge) |
| `[debug] DebugMode` | `0` | Open a console window and enable the debug hotkey F9 (dump the next frame) |
| `[debug] FpsLog` | `0` | Log frame rate, frame time and CPU time every second; needs `DebugMode=1` |

## Controls

During a mission the mouse is captured and hidden; in menus and on the map the left button clicks. Bindings are defined in [`KeyBindings.h`](CK_source/pc_port/platform_sdl/KeyBindings.h); rebuild after changing them.

| Input | Action |
| --- | --- |
| Move mouse | Turn the view |
| Left button | Fire (hold) |
| Right button | Scope in / out |
| Wheel | Zoom while scoped |
| `Shift` | Steady (hold breath, hold) |
| `R` | Reload |
| `Q` / `E` | Switch weapon |
| `C` | Take / leave cover |
| `H` | Medkit |
| `F` | Slow-motion pill |
| `G` | Ultrasound emitter |
| `Esc` | Pause |
| `S` | In-mission shop |

## Cheat codes

Type the letters once the game has loaded; there is no input box. `C` and `H` are also mission keys, so typing them on the map is best.

| Code | Effect |
| --- | --- |
| `chm` | Cash +9,999,999, gold set to 99,999 (the map shows five digits of gold) |
| `chxp` | Level +1 |
| `chxxp` | Level +10 |
| `chxxxp` | Top level (70) |

## Disclaimer

- This project has no affiliation with Glu Mobile or its affiliates, and is not authorized or endorsed by them.
- The names, trademarks, art, audio, levels and other game content of the Contract Killer series, as well as the original program, are the property of their respective owners.
- This repository contains recovered source, build scripts and **the Swerve engine core assembly generated from the original binary**. It contains no original executable and no game data; you need legally obtained game assets to run it.
- This project is for learning, research and preservation only, and must not be used commercially.
- The code is provided "as is", without warranty of any kind; use at your own risk.
