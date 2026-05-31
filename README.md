# InvestTime / StudyWithMe

**中文** | [English](#english)

InvestTime 是一款使用 C++ 与 Qt Widgets 开发的本地学习陪伴工具。它把番茄钟、待办事项、环境音、学习统计和个人简介放在一个轻量的桌面窗口里，适合自习、复习、写作和长时间专注。

## 功能特性

- 番茄钟：支持专注时长设置、倒计时圆环、开始、暂停与重置。
- 待办事项：支持添加、勾选、清除已完成任务，并提供待办小窗模式。
- 环境音：支持白噪音与雨声分类、本地音频优先、上一首/下一首、播放状态提示和音量控制。
- 学习统计：记录今日、本周、本月和累计学习时长，并保存每日学习数据。
- 主题与窗口：支持白天/夜晚主题、窗口置顶、无边框标题栏和托盘通知。
- 本地存储：数据保存在本机 INI 文件中，不依赖在线账号。

## 项目结构

```text
.
├── StudyWithMeVS/                 # Visual Studio / Qt Widgets project
│   ├── StudyWithMeVS.sln
│   └── StudyWithMe/
│       ├── StudyWithMe.vcxproj
│       ├── assets/audio/          # Optional local ambience audio
│       └── src/
├── CMakeLists.txt                 # Legacy simple CMake entry
├── StudyWithMe.pro                # Legacy simple qmake entry
└── README.md
```

## 开发环境

- Windows 10/11
- Visual Studio 2019 或更新版本
- Qt 6，MSVC 64-bit kit
- C++17

当前 Visual Studio 工程默认面向 Qt 6。若你的 Qt 安装路径不同，请设置 `QTDIR` 指向本机 Qt MSVC kit，例如：

```powershell
setx QTDIR C:\Qt\6.7.3\msvc2019_64
```

设置后重新打开 Visual Studio。

## 运行方式

1. 打开 `StudyWithMeVS/StudyWithMeVS.sln`。
2. 选择 `Debug | x64` 或对应的 x64 配置。
3. 确认 Qt VS Tools 能找到 Qt kit。
4. 按 `F5` 构建并运行。

## 本地数据

应用会把学习时长、待办事项、窗口状态、主题、目标和环境音设置保存到：

```text
%APPDATA%\StudyWithMe\StudyWithMe\settings.ini
```

这些数据只保存在本机。

## 环境音资源

程序会优先读取部署目录或项目目录中的本地音频：

```text
assets/audio/white_noise/
assets/audio/rain/
```

支持的格式包括 `.mp3`、`.ogg`、`.wav`、`.flac`。由于 GitHub 对单文件大小有限制，仓库默认不跟踪 `.flac` 大文件。需要更高质量音源时，可在本地把音频放进上述目录。

如果本地音频不存在，程序会使用 Wikimedia Commons 上的网络音源作为后备。

## Release

Beta 测试版使用 `v*-beta*` 标签触发 GitHub Actions 自动创建 prerelease。

```powershell
git tag -a v0.1.0-beta.2 -m "Beta 测试版 v0.1.0-beta.2"
git push origin v0.1.0-beta.2
```

## English

InvestTime is a local study companion built with C++ and Qt Widgets. It combines a Pomodoro timer, todo list, ambience audio, study statistics, and a small profile page in a lightweight desktop window for focused study, review, writing, and long work sessions.

## Features

- Pomodoro timer: configurable focus duration, circular countdown, start, pause, and reset.
- Todo list: add tasks, check items, clear completed tasks, and switch to a compact todo-only window.
- Ambience audio: white noise and rain categories, local audio first, previous/next track controls, playback status, and volume control.
- Study statistics: tracks today, this week, this month, and total study time.
- Themes and window controls: day/night themes, always-on-top mode, frameless title bar, and tray notifications.
- Local storage: settings and study records are stored locally without an online account.

## Project Layout

```text
.
├── StudyWithMeVS/                 # Visual Studio / Qt Widgets project
│   ├── StudyWithMeVS.sln
│   └── StudyWithMe/
│       ├── StudyWithMe.vcxproj
│       ├── assets/audio/          # Optional local ambience audio
│       └── src/
├── CMakeLists.txt                 # Legacy simple CMake entry
├── StudyWithMe.pro                # Legacy simple qmake entry
└── README.md
```

## Development Environment

- Windows 10/11
- Visual Studio 2019 or newer
- Qt 6, MSVC 64-bit kit
- C++17

The Visual Studio project targets Qt 6 by default. If your Qt installation is in a different location, set `QTDIR` to your local Qt MSVC kit:

```powershell
setx QTDIR C:\Qt\6.7.3\msvc2019_64
```

Restart Visual Studio after changing the environment variable.

## Run

1. Open `StudyWithMeVS/StudyWithMeVS.sln`.
2. Select `Debug | x64` or the matching x64 configuration.
3. Make sure Qt VS Tools can find your Qt kit.
4. Press `F5` to build and run.

## Local Data

The app stores study time, todos, window state, theme, goals, and ambience settings in:

```text
%APPDATA%\StudyWithMe\StudyWithMe\settings.ini
```

All data stays on the local machine.

## Ambience Audio

The app first looks for local audio files in the deployment directory or project directory:

```text
assets/audio/white_noise/
assets/audio/rain/
```

Supported formats include `.mp3`, `.ogg`, `.wav`, and `.flac`. Because GitHub enforces file size limits, this repository ignores large `.flac` files by default. For higher-quality local ambience, place your own audio files in the folders above.

If no local file is available, the app falls back to Wikimedia Commons audio sources.

## Releases

Beta releases are published as GitHub prereleases by pushing `v*-beta*` tags.
