#pragma once

#include <QList>
#include <QMap>
#include <QDate>
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
    QUrl ambienceUrl(int index) const;
    QString formatTime(int seconds) const;

    QTimer m_timer;
    QVariantAnimation *m_progressAnimation = nullptr;
    QVariantAnimation *m_glowAnimation = nullptr;
    QList<QWidget *> m_animatedWidgets;
    QList<QWidget *> m_shadowWidgets;
    QList<QPushButton *> m_tabButtons;

    FocusRing *m_ring = nullptr;
    QFrame *m_tabBar = nullptr;
    QFrame *m_tabBubble = nullptr;
    QFrame *m_themeSwitchFrame = nullptr;
    QFrame *m_themeBubble = nullptr;
    QStackedWidget *m_featureStack = nullptr;
    QWidget *m_contentWidget = nullptr;
    QWidget *m_headerWidget = nullptr;
    QWidget *m_titleBar = nullptr;
    QWidget *m_todoTitleWidget = nullptr;
    QWidget *m_todoCaptionWidget = nullptr;
    QFrame *m_todoCard = nullptr;
    QSpinBox *m_focusSpin = nullptr;
    QSpinBox *m_breakSpin = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_pinButton = nullptr;
    QPushButton *m_minimizeButton = nullptr;
    QPushButton *m_closeButton = nullptr;
    QPushButton *m_compactButton = nullptr;
    QPushButton *m_restoreButton = nullptr;
    QPushButton *m_dayButton = nullptr;
    QPushButton *m_nightButton = nullptr;
    QPushButton *m_playSoundButton = nullptr;
    QPushButton *m_goalButton = nullptr;
    QLineEdit *m_todoInput = nullptr;
    QWidget *m_todoInputRowWidget = nullptr;
    QWidget *m_todoRestoreRowWidget = nullptr;
    QPushButton *m_cleanTodoButton = nullptr;
    QListWidget *m_todoList = nullptr;
    QLabel *m_todayStatsLabel = nullptr;
    QLabel *m_weekStatsLabel = nullptr;
    QLabel *m_monthStatsLabel = nullptr;
    QLabel *m_totalStatsLabel = nullptr;
    QProgressBar *m_todayProgressBar = nullptr;
    QProgressBar *m_weekProgressBar = nullptr;
    QProgressBar *m_monthProgressBar = nullptr;
    QProgressBar *m_totalProgressBar = nullptr;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMediaPlayer *m_audioPlayer = nullptr;
    QAudioOutput *m_audioOutput = nullptr;

    bool m_nightMode = false;
    bool m_entrancePlayed = false;
    bool m_alwaysOnTop = false;
    bool m_todoCompact = false;
    bool m_draggingWindow = false;
    QPoint m_dragStartPosition;
    QRect m_normalGeometry;
    QMap<QDate, int> m_dailyStudySeconds;
    int m_unsavedStudySeconds = 0;
    int m_ambienceIndex = 0;
    int m_dailyGoalSeconds = 120 * 60;
    int m_weeklyGoalSeconds = 720 * 60;
    int m_monthlyGoalSeconds = 21 * 60 * 60;
    int m_totalSeconds = 25 * 60;
    int m_remainingSeconds = 25 * 60;
};
