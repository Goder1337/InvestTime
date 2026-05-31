#pragma once

#include <QList>
#include <QMap>
#include <QDate>
#include <QDir>
#include <QFileInfo>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QTimer>
#include <QWidget>

class FocusRing;
class QEvent;
class QFrame;
class QHBoxLayout;
class QLineEdit;
class QListWidget;
class QObject;
class QPushButton;
class QResizeEvent;
class QCloseEvent;
class QMouseEvent;
class QProgressBar;
class QShowEvent;
class QSpinBox;
class QStackedWidget;
class QSystemTrayIcon;
class QVBoxLayout;
class QWidget;
class QLabel;
class QMediaPlayer;
class QAudioOutput;
class QUrl;
class QVariantAnimation;

class MainWindow : public QWidget
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QFrame *createCard(const QString &title, const QString &caption);
    QPushButton *createButton(const QString &text, const QString &objectName = "ghostButton");
    QWidget *createTabPage();
    QWidget *buildTitleBar();

    void buildUi();
    void buildTimerCard(QVBoxLayout *pageLayout);
    void buildTodoCard(QVBoxLayout *pageLayout);
    void buildAmbienceCard(QVBoxLayout *pageLayout);
    void buildOverviewCard(QVBoxLayout *pageLayout);
    void buildProfileCard(QVBoxLayout *pageLayout);
    void buildThemeSwitch(QHBoxLayout *header);
    QLabel *addMetric(QHBoxLayout *row, const QString &name, const QString &value, int progress);
    void showGoalDialog();

    void addTodo(const QString &text, bool checked = false);
    void addTodoFromInput();
    void removeCompletedTodos();
    void toggleTimer();
    void resetTimer();
    void tick();
    void updateRing();
    void selectTab(int index);
    void moveTabBubble(int index, bool animated);
    void moveThemeBubble(bool animated);
    void applyTheme(bool nightMode);
    void applyShadow(QWidget *widget);
    void installButtonAnimation(QPushButton *button);
    void animateButton(QPushButton *button, bool hovered, bool pressed);
    void animateButtonBubble(QPushButton *button, bool pressed);
    void fadeButtonBubble(QPushButton *button);
    void animateEntrance();
    void toggleAlwaysOnTop();
    void toggleTodoCompact();
    void setTodoCompact(bool compact);
    void updateTodoCompactSize();
    void setupPersistence();
    QString settingsPath() const;
    void loadState();
    void saveState();
    void loadTodos();
    void saveTodos();
    void addStudySecond();
    void flushStudyStats();
    void updateStatsLabels();
    int studySecondsForDay(const QDate &date) const;
    int studySecondsForCurrentWeek() const;
    int studySecondsForCurrentMonth() const;
    QString formatDuration(int seconds) const;
    void setupTrayIcon();
    void showPomodoroFinishedMessage();
    void setupAudio();
    void selectAmbience(int index);
    void toggleAmbiencePlayback();
    void changeAmbienceTrack(int direction);
    void updateAmbienceButton(bool playing);
    void updateAmbienceNowLabel(bool playing);
    QFileInfoList ambienceFiles(int index) const;
    QUrl ambienceUrl(int index) const;
    QString ambienceName(int index) const;
    QString formatTime(int seconds) const;

    QTimer m_timer;                              // 番茄鐘每秒觸發一次 tick()。
    QVariantAnimation *m_progressAnimation = nullptr; // 圓環進度平滑動畫。
    QVariantAnimation *m_glowAnimation = nullptr;     // 保留的光暈動畫入口。
    QList<QWidget *> m_animatedWidgets;          // 啟動淡入等動畫控件集合。
    QList<QWidget *> m_shadowWidgets;            // 需要套卡片陰影的控件集合。
    QList<QPushButton *> m_tabButtons;           // 五個功能標籤按鈕。

    FocusRing *m_ring = nullptr;                 // 番茄鐘中央圓環。
    QFrame *m_tabBar = nullptr;                  // 自訂標籤列外框。
    QFrame *m_tabBubble = nullptr;               // 標籤列滑動氣泡。
    QFrame *m_themeSwitchFrame = nullptr;        // 白天/夜晚切換外框。
    QFrame *m_themeBubble = nullptr;             // 白天/夜晚滑動氣泡。
    QStackedWidget *m_featureStack = nullptr;    // 五個功能頁容器。
    QWidget *m_contentWidget = nullptr;          // 主內容區。
    QWidget *m_headerWidget = nullptr;           // 頂部標題與快捷按鈕列。
    QWidget *m_titleBar = nullptr;               // 自繪視窗標題列。
    QWidget *m_todoTitleWidget = nullptr;        // 待辦卡片標題，小窗模式隱藏。
    QWidget *m_todoCaptionWidget = nullptr;      // 待辦卡片描述，小窗模式隱藏。
    QFrame *m_todoCard = nullptr;                // 待辦頁外層線框。
    QSpinBox *m_focusSpin = nullptr;             // 專注時長設定。
    QSpinBox *m_breakSpin = nullptr;             // 休息時長設定。
    QPushButton *m_startButton = nullptr;        // 番茄鐘開始/暫停/繼續。
    QPushButton *m_pinButton = nullptr;          // 視窗置頂按鈕。
    QPushButton *m_minimizeButton = nullptr;     // 視窗最小化按鈕。
    QPushButton *m_closeButton = nullptr;        // 視窗關閉按鈕。
    QPushButton *m_compactButton = nullptr;      // 切換待辦小窗按鈕。
    QPushButton *m_restoreButton = nullptr;      // 待辦小窗還原按鈕。
    QPushButton *m_dayButton = nullptr;          // 白天主題按鈕。
    QPushButton *m_nightButton = nullptr;        // 夜晚主題按鈕。
    QPushButton *m_playSoundButton = nullptr;    // 環境音播放/暫停按鈕。
    QPushButton *m_prevSoundButton = nullptr;    // 環境音上一首按鈕。
    QPushButton *m_nextSoundButton = nullptr;    // 環境音下一首按鈕。
    QList<QPushButton *> m_ambienceButtons;      // 環境音來源按鈕，索引用於同步音源與選中樣式。
    QLabel *m_currentSoundLabel = nullptr;       // 顯示目前選擇或正在播放的環境音。
    QPushButton *m_goalButton = nullptr;         // 學習目標設定按鈕。
    QLineEdit *m_todoInput = nullptr;            // 待辦輸入框。
    QWidget *m_todoInputRowWidget = nullptr;     // 待辦輸入列，小窗模式隱藏。
    QWidget *m_todoRestoreRowWidget = nullptr;   // 小窗還原列。
    QPushButton *m_cleanTodoButton = nullptr;    // 清除已完成待辦按鈕。
    QListWidget *m_todoList = nullptr;           // 待辦列表。
    QLabel *m_todayStatsLabel = nullptr;         // 今日學習時長文字。
    QLabel *m_weekStatsLabel = nullptr;          // 本週學習時長文字。
    QLabel *m_monthStatsLabel = nullptr;         // 本月學習時長文字。
    QLabel *m_totalStatsLabel = nullptr;         // 累計學習時長文字。
    QProgressBar *m_todayProgressBar = nullptr;  // 今日目標進度條。
    QProgressBar *m_weekProgressBar = nullptr;   // 本週目標進度條。
    QProgressBar *m_monthProgressBar = nullptr;  // 本月目標進度條。
    QProgressBar *m_totalProgressBar = nullptr;  // 累計階段進度條。
    QSystemTrayIcon *m_trayIcon = nullptr;       // Windows 通知與托盤入口。
    QMediaPlayer *m_audioPlayer = nullptr;       // 環境音播放器。
    QAudioOutput *m_audioOutput = nullptr;       // 環境音輸出與音量控制。

    bool m_nightMode = false;                    // 目前是否為夜晚主題。
    bool m_entrancePlayed = false;               // 啟動淡入動畫是否已播放。
    bool m_alwaysOnTop = false;                  // 視窗置頂狀態。
    bool m_todoCompact = false;                  // 是否處於待辦小窗模式。
    bool m_draggingWindow = false;               // 主視窗標題列拖動狀態。
    QPoint m_dragStartPosition;                  // 主視窗拖動起點。
    QRect m_normalGeometry;                      // 進入小窗前的完整視窗位置。
    QMap<QDate, int> m_dailyStudySeconds;        // 每日學習秒數，寫入 ini 的 studyDays。
    int m_unsavedStudySeconds = 0;               // 尚未落盤的學習秒數。
    int m_ambienceIndex = 0;                     // 當前環境音索引。
    int m_ambienceTrackIndex = 0;                // 當前環境音資料夾中的曲目索引。
    int m_dailyGoalSeconds = 120 * 60;           // 每日目標，預設 120 分鐘。
    int m_weeklyGoalSeconds = 720 * 60;          // 每週目標，預設 720 分鐘。
    int m_monthlyGoalSeconds = 21 * 60 * 60;     // 每月目標，預設 21 小時。
    int m_totalSeconds = 25 * 60;                // 當前番茄鐘總秒數。
    int m_remainingSeconds = 25 * 60;            // 當前番茄鐘剩餘秒數。
};
