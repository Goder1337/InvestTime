# StudyWithMe Visual Studio Project

这是一个 Visual Studio `.sln` 版 Qt Widgets 项目，适合直接打开、下断点、单步调试。

## 打开方式

1. 用 Visual Studio 2019 打开 `StudyWithMeVS.sln`。
2. 确认配置为 `Debug | x64`。
3. 本机项目已默认使用 `D:\Qt\6.7.3\msvc2019_64`。如果你换电脑或换 Qt 版本，再设置环境变量 `QTDIR`，指向你的 Qt MSVC 2019 kit，例如：

   ```powershell
   setx QTDIR C:\Qt\6.6.3\msvc2019_64
   ```

4. 重启 Visual Studio，然后按 `F5` 调试。

## Qt 版本

项目默认链接 Qt 6：

```xml
<QtMajorVersion>6</QtMajorVersion>
```

如果你使用 Qt 5，可以在项目属性或 `.vcxproj` 里把 `QtMajorVersion` 改成 `5`。

## 适合下断点的位置

- `MainWindow::toggleTimer()`：开始、暂停番茄钟。
- `MainWindow::tick()`：每秒倒计时逻辑。
- `MainWindow::addTodoFromInput()`：添加待办事项。
- `MainWindow::removeCompletedTodos()`：清除已完成任务。
- `FocusRing::paintEvent()`：自定义进度环绘制。
- `MainWindow::applyTheme()`：白天 / 夜晚主题切换。
- `MainWindow::animateEntrance()`：窗口淡入和卡片进入动画。
- `FocusRing::setGlow()`：番茄钟呼吸光晕动画。

## 数据存储

学习时长、待办事项、置顶状态、主题、音量和番茄钟设置使用 INI 文件保存：

```text
%APPDATA%\StudyWithMe\StudyWithMe\settings.ini
```

每日学习时长按日期保存，软件重启或电脑重启后会自动读取，并汇总今日、本周、本月和累计学习时长。

## 环境音

程序优先读取本地文件：

```text
assets/audio/rain.ogg
assets/audio/cafe.wav
assets/audio/white_noise.ogg
assets/audio/forest.ogg
```

如果本地文件不存在，会使用 Wikimedia Commons 的远程音频地址作为后备来源。当前来源包括：

- Rain and thunder.ogg
- Restaurant ambience, early morning, A.wav
- White noise.ogg
- Blackbird song.ogg
