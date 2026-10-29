# fooyin Renumber Tracks

A fooyin plugin that adds a **Renumber Tracks** action to the right-click context menu. Select tracks in a playlist, right-click, and choose **Tagging > Renumber tracks...** to open a dialog where you can reorder and renumber them.

**Supported: fooyin 0.14.1.** Linux (Flatpak) and Windows x64. This is an independent GPL extension, not an official fooyin release.

## Features

- Renumber selected tracks with a preview before applying
- Tracks grouped by disc number, each disc renumbers independently
- Reorder tracks with Move Up / Move Down buttons
- Configurable start number (default: 1)
- Optional zero-padding (01, 02... vs 1, 2...)
- Only selected tracks are affected; unselected tracks keep their numbers

## Build from source (Linux)

Requirements:
- fooyin installed via Flatpak
- `org.kde.Sdk//6.11` Flatpak runtime
- fooyin v0.14.1 source tree

```bash
git clone --branch v0.14.1 --depth 1 https://github.com/fooyin/fooyin.git ../fooyin
./build.sh
./install.sh
```

## Build from source (Windows)

Requirements:
- Visual Studio 2022 C++ Build Tools
- Qt 6.12.0 MSVC2022 x64 SDK
- Python 3, CMake 3.25+, Ninja
- Installed fooyin 0.14.1 runtime
- fooyin v0.14.1 source at commit `5a373aa9763d6f8d86cc749c68971a0f3b1d8918`

From an x64 Native Tools Command Prompt:

```powershell
.\build.ps1 -QtDirectory 'C:\Qt\6.12.0\msvc2022_64' -FooyinSource '..\fooyin'
```

## Credits and license

By **Gene Dupre**. Licensed under **GPL-3.0-or-later**; see [LICENSE](LICENSE).
