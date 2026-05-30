#include "MainWindow.h"

#include "FocusRing.h"

#include <QAbstractItemView>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDate>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEasingCurve>
#include <QEvent>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPalette>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QShowEvent>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStringList>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <QWidget>
#include <QAudioOutput>
#include <QApplication>
#include <QCoreApplication>

namespace {
class DialogDragFilter : public QObject
{
public:
    explicit DialogDragFilter(QWidget *target, QObject *parent = nullptr)
        : QObject(parent), m_target(target)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        Q_UNUSED(watched);
        if (!m_target) {
            return false;
        }

        if (event->type() == QEvent::MouseButtonPress) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                // 學習目標彈窗是自繪標題列，這裡記錄滑鼠與視窗左上角的距離。
                m_dragging = true;
                m_dragOffset = mouseEvent->globalPosition().toPoint() - m_target->frameGeometry().topLeft();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove && m_dragging) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            // 拖動標題列時移動整個彈窗。
            m_target->move(mouseEvent->globalPosition().toPoint() - m_dragOffset);
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            m_dragging = false;
            return true;
        }

        return false;
    }

private:
    QWidget *m_target = nullptr;
    bool m_dragging = false;
    QPoint m_dragOffset;
};
}

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    // 主視窗使用無邊框自繪標題列，因此關閉系統標題列。
    setWindowTitle("StudyWithMe");
    resize(1120, 720);
    setMinimumSize(920, 620);
    setObjectName("root");
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setWindowOpacity(0.0);

    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &MainWindow::tick);

    // 先建立所有控件，再讀取持久化狀態。
    buildUi();

    // 番茄鐘進度動畫：只在進度變化時播放，避免長時間佔用 CPU。
    m_progressAnimation = new QVariantAnimation(this);
    m_progressAnimation->setDuration(520);
    m_progressAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_progressAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        if (m_ring) {
            m_ring->setProgress(value.toDouble());
        }
    });

    setupPersistence();
    setupTrayIcon();
    setupAudio();
    applyTheme(false);
    loadState();
    resetTimer();
    updateStatsLabels();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_entrancePlayed) {
        m_entrancePlayed = true;
        animateEntrance();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveState();
    QWidget::closeEvent(event);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    auto *button = qobject_cast<QPushButton *>(watched);
    if (!button) {
        if (watched == m_titleBar) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            switch (event->type()) {
            case QEvent::MouseButtonPress:
                if (mouseEvent->button() == Qt::LeftButton) {
                    m_draggingWindow = true;
                    m_dragStartPosition = mouseEvent->globalPosition().toPoint() - frameGeometry().topLeft();
                    return true;
                }
                break;
            case QEvent::MouseMove:
                if (m_draggingWindow && (mouseEvent->buttons() & Qt::LeftButton)) {
                    move(mouseEvent->globalPosition().toPoint() - m_dragStartPosition);
                    return true;
                }
                break;
            case QEvent::MouseButtonRelease:
                m_draggingWindow = false;
                break;
            default:
                break;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    return QWidget::eventFilter(watched, event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    QTimer::singleShot(0, this, [this]() {
        moveTabBubble(m_featureStack ? m_featureStack->currentIndex() : 0, false);
        moveThemeBubble(false);
    });
}

QFrame *MainWindow::createCard(const QString &title, const QString &caption)
{
    // 通用玻璃卡片：番茄鐘、待辦、環境、狀態、簡介頁都使用這個外框。
    auto *frame = new QFrame;
    frame->setObjectName("card");
    frame->setAttribute(Qt::WA_StyledBackground, true);
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(24, 22, 24, 24);
    layout->setSpacing(14);

    auto *titleLabel = new QLabel(title);
    titleLabel->setObjectName("cardTitle");
    auto *captionLabel = new QLabel(caption);
    captionLabel->setObjectName("muted");
    captionLabel->setWordWrap(true);

    // 卡片上方固定放標題與說明文字，後面的功能控件會由各 buildXXXCard 加入。
    layout->addWidget(titleLabel);
    layout->addWidget(captionLabel);

    m_animatedWidgets.append(frame);
    m_shadowWidgets.append(frame);
    return frame;
}

QPushButton *MainWindow::createButton(const QString &text, const QString &objectName)
{
    // 全域按鈕工廠：objectName 會對應 applyTheme() 裡的 QSS 樣式。
    auto *button = new QPushButton(text);
    button->setCursor(Qt::PointingHandCursor);
    button->setObjectName(objectName);
    button->setMinimumHeight(42);
    installButtonAnimation(button);
    return button;
}

QWidget *MainWindow::createTabPage()
{
    // 每個標籤頁的基礎容器，內部再塞一張功能卡片。
    auto *page = new QWidget;
    page->setObjectName("tabPage");
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);
    return page;
}

QWidget *MainWindow::buildTitleBar()
{
    // 自繪標題列：因為主視窗是 FramelessWindowHint，所以拖動與按鈕都由我們處理。
    auto *bar = new QWidget;
    bar->setObjectName("titleBar");
    bar->setFixedHeight(30);
    bar->installEventFilter(this);
    m_titleBar = bar;

    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(12, 3, 6, 3);
    layout->setSpacing(6);

    auto *title = new QLabel("StudyWithMe");
    title->setObjectName("titleBarText");
    // 左側標題文字；右側 stretch 後才放置頂/最小化/關閉。
    layout->addWidget(title);
    layout->addStretch();

    auto makeTitleButton = [this](const QString &text, const QString &objectName) {
        // 標題列小按鈕固定尺寸，避免影響拖動區域。
        auto *button = new QPushButton(text);
        button->setObjectName(objectName);
        button->setCursor(Qt::PointingHandCursor);
        button->setFixedSize(30, 22);
        button->setFocusPolicy(Qt::NoFocus);
        return button;
    };

    m_pinButton = makeTitleButton("置", "titleButton");
    m_pinButton->setCheckable(true);
    m_pinButton->setToolTip("窗口置顶");
    // 置頂、最小化、關閉三個視窗控制按鈕。
    m_minimizeButton = makeTitleButton("—", "titleButton");
    m_minimizeButton->setToolTip("最小化");
    m_closeButton = makeTitleButton("×", "titleCloseButton");
    m_closeButton->setToolTip("关闭");

    layout->addWidget(m_pinButton);
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_closeButton);

    connect(m_pinButton, &QPushButton::clicked, this, &MainWindow::toggleAlwaysOnTop);
    connect(m_minimizeButton, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(m_closeButton, &QPushButton::clicked, this, &MainWindow::close);

    return bar;
}

void MainWindow::buildUi()
{
    // 主視窗最外層：上方自繪標題列，下方是所有內容。
    auto *shell = new QVBoxLayout(this);
    shell->setContentsMargins(1, 1, 1, 1);
    shell->setSpacing(0);
    shell->addWidget(buildTitleBar());

    m_contentWidget = new QWidget;
    m_contentWidget->setObjectName("contentWidget");
    // contentLayout 控制完整主視窗內容區的外距。
    auto *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(21, 12, 21, 21);
    contentLayout->setSpacing(14);
    shell->addWidget(m_contentWidget, 1);

    m_headerWidget = new QWidget;
    m_headerWidget->setObjectName("headerWidget");
    // 頂部狀態列：左邊是標題文案，右邊是待辦小窗與日夜切換。
    auto *header = new QHBoxLayout(m_headerWidget);
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(12);
    auto *headlineBox = new QVBoxLayout;
    headlineBox->setSpacing(4);
    auto *eyebrow = new QLabel("FOCUS DASHBOARD");
    eyebrow->setObjectName("eyebrow");
    auto *headline = new QLabel("Together We Learn, Together We Rise");
    headline->setObjectName("headline");
    auto *subtitle = new QLabel(QDate::currentDate().toString("yyyy.MM.dd  dddd"));
    subtitle->setObjectName("muted");
    // 頂部三行文字：英文小標、主標題、日期。
    headlineBox->addWidget(eyebrow);
    headlineBox->addWidget(headline);
    headlineBox->addWidget(subtitle);
    header->addLayout(headlineBox);
    header->addStretch();
    m_compactButton = createButton("待办小窗", "ghostButton");
    m_compactButton->setCheckable(true);
    // 待辦小窗按鈕：切換完整視窗與只顯示待辦的小視窗。
    header->addWidget(m_compactButton);
    buildThemeSwitch(header);
    contentLayout->addWidget(m_headerWidget);

    connect(m_compactButton, &QPushButton::clicked, this, &MainWindow::toggleTodoCompact);

    auto *tabShell = new QFrame;
    tabShell->setObjectName("tabShell");
    tabShell->setAttribute(Qt::WA_StyledBackground, true);
    auto *tabShellLayout = new QVBoxLayout(tabShell);
    tabShellLayout->setContentsMargins(0, 0, 0, 0);
    tabShellLayout->setSpacing(12);
    m_animatedWidgets.append(tabShell);

    m_tabBar = new QFrame;
    m_tabBar->setObjectName("customTabBar");
    m_tabBar->setAttribute(Qt::WA_StyledBackground, true);

    m_tabBubble = new QFrame(m_tabBar);
    m_tabBubble->setObjectName("tabBubble");
    m_tabBubble->setAttribute(Qt::WA_StyledBackground, true);
    m_tabBubble->hide();

    auto *tabBarLayout = new QHBoxLayout(m_tabBar);
    tabBarLayout->setContentsMargins(5, 5, 5, 5);
    tabBarLayout->setSpacing(6);

    m_featureStack = new QStackedWidget;
    m_featureStack->setObjectName("featureStack");

    const QStringList tabNames = {"番茄钟", "待办事项", "环境氛围", "状态概览", "个人简介"};
    auto *tabGroup = new QButtonGroup(this);
    tabGroup->setExclusive(true);
    // 五個功能標籤按鈕平均分配寬度，氣泡由 moveTabBubble() 跟隨目前選中項。
    for (int i = 0; i < tabNames.size(); ++i) {
        auto *button = createButton(tabNames.at(i), "tabButton");
        button->setCheckable(true);
        button->setMinimumHeight(40);
        tabGroup->addButton(button, i);
        tabBarLayout->addWidget(button, 1);
        m_tabButtons.append(button);
        if (i == 0) {
            button->setChecked(true);
        }
    }

    QWidget *timerPage = createTabPage();
    // 標籤頁 0：番茄鐘。
    buildTimerCard(static_cast<QVBoxLayout *>(timerPage->layout()));
    m_featureStack->addWidget(timerPage);

    QWidget *todoPage = createTabPage();
    // 標籤頁 1：待辦事項。
    buildTodoCard(static_cast<QVBoxLayout *>(todoPage->layout()));
    m_featureStack->addWidget(todoPage);

    QWidget *ambiencePage = createTabPage();
    // 標籤頁 2：環境氛圍。
    buildAmbienceCard(static_cast<QVBoxLayout *>(ambiencePage->layout()));
    m_featureStack->addWidget(ambiencePage);

    QWidget *overviewPage = createTabPage();
    // 標籤頁 3：狀態概覽。
    buildOverviewCard(static_cast<QVBoxLayout *>(overviewPage->layout()));
    m_featureStack->addWidget(overviewPage);

    QWidget *profilePage = createTabPage();
    // 標籤頁 4：個人簡介。
    buildProfileCard(static_cast<QVBoxLayout *>(profilePage->layout()));
    m_featureStack->addWidget(profilePage);

    connect(tabGroup, qOverload<int>(&QButtonGroup::idClicked), this, &MainWindow::selectTab);
    QTimer::singleShot(0, this, [this]() {
        moveTabBubble(0, false);
    });

    tabShellLayout->addWidget(m_tabBar);
    tabShellLayout->addWidget(m_featureStack, 1);
    contentLayout->addWidget(tabShell, 1);
}

void MainWindow::buildThemeSwitch(QHBoxLayout *header)
{
    // 白天/夜晚切換外殼；高度與旁邊按鈕保持一致。
    auto *switchFrame = new QFrame;
    switchFrame->setObjectName("themeSwitch");
    switchFrame->setAttribute(Qt::WA_StyledBackground, true);
    switchFrame->setFixedHeight(42);
    m_themeSwitchFrame = switchFrame;

    m_themeBubble = new QFrame(switchFrame);
    m_themeBubble->setObjectName("themeBubble");
    m_themeBubble->setAttribute(Qt::WA_StyledBackground, true);
    m_themeBubble->hide();

    auto *layout = new QHBoxLayout(switchFrame);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    m_dayButton = createButton("白天", "themeButton");
    m_nightButton = createButton("夜晚", "themeButton");
    // 兩個主題按鈕由同一個氣泡背景滑動表示選中狀態。
    m_dayButton->setFixedHeight(34);
    m_nightButton->setFixedHeight(34);
    m_dayButton->setCheckable(true);
    m_nightButton->setCheckable(true);
    m_dayButton->setChecked(true);

    auto *group = new QButtonGroup(this);
    group->setExclusive(true);
    group->addButton(m_dayButton);
    group->addButton(m_nightButton);

    layout->addWidget(m_dayButton);
    layout->addWidget(m_nightButton);
    header->addWidget(switchFrame);

    connect(m_dayButton, &QPushButton::clicked, this, [this]() { applyTheme(false); });
    connect(m_nightButton, &QPushButton::clicked, this, [this]() { applyTheme(true); });
    QTimer::singleShot(0, this, [this]() {
        moveThemeBubble(false);
    });
}

void MainWindow::buildTimerCard(QVBoxLayout *pageLayout)
{
    // 番茄鐘頁：圓環、專注/休息時長、開始/重置按鈕。
    auto *frame = createCard("番茄钟", "设置专注与休息时长，进度环会以平滑动画反馈当前节奏。");
    pageLayout->addWidget(frame, 1);
    auto *layout = static_cast<QVBoxLayout *>(frame->layout());

    m_ring = new FocusRing;
    // 中央倒數圓環，實際繪製在 FocusRing::paintEvent()。
    layout->addWidget(m_ring, 1, Qt::AlignCenter);

    auto *settings = new QHBoxLayout;
    settings->setSpacing(12);

    auto *focusBox = new QFrame;
    focusBox->setObjectName("miniCard");
    // 專注時長卡片：QSpinBox 控制番茄鐘主時長。
    auto *focusLayout = new QVBoxLayout(focusBox);
    focusLayout->setContentsMargins(14, 10, 14, 12);
    focusLayout->setSpacing(6);
    auto *focusLabel = new QLabel("专注时长");
    focusLabel->setObjectName("muted");
    m_focusSpin = new QSpinBox;
    m_focusSpin->setRange(1, 180);
    m_focusSpin->setValue(25);
    m_focusSpin->setSuffix(" 分钟");
    m_focusSpin->setMinimumWidth(120);
    m_focusSpin->setAlignment(Qt::AlignCenter);
    focusLayout->addWidget(focusLabel);
    focusLayout->addWidget(m_focusSpin);

    auto *breakBox = new QFrame;
    breakBox->setObjectName("miniCard");
    // 休息時長卡片：目前保留設定 UI，方便後續擴充休息流程。
    auto *breakLayout = new QVBoxLayout(breakBox);
    breakLayout->setContentsMargins(14, 10, 14, 12);
    breakLayout->setSpacing(6);
    auto *breakLabel = new QLabel("休息时长");
    breakLabel->setObjectName("muted");
    m_breakSpin = new QSpinBox;
    m_breakSpin->setRange(1, 60);
    m_breakSpin->setValue(5);
    m_breakSpin->setSuffix(" 分钟");
    m_breakSpin->setMinimumWidth(120);
    m_breakSpin->setAlignment(Qt::AlignCenter);
    breakLayout->addWidget(breakLabel);
    breakLayout->addWidget(m_breakSpin);

    settings->addWidget(focusBox, 1);
    settings->addWidget(breakBox, 1);
    layout->addLayout(settings);

    auto *actions = new QHBoxLayout;
    actions->setSpacing(12);
    m_startButton = createButton("开始", "primaryButton");
    auto *reset = createButton("重置");
    // 底部操作按鈕：開始/暫停共用 m_startButton，reset 回到設定時長。
    actions->addWidget(m_startButton, 1);
    actions->addWidget(reset);
    layout->addLayout(actions);

    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::toggleTimer);
    connect(reset, &QPushButton::clicked, this, &MainWindow::resetTimer);
    connect(m_focusSpin, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::resetTimer);
}

void MainWindow::buildTodoCard(QVBoxLayout *pageLayout)
{
    // 待辦頁：完整視窗與待辦小窗共用同一張卡片。
    auto *frame = createCard("待办事项", "把今天要完成的学习任务放进清单，完成后可直接勾选。");
    m_todoCard = frame;
    pageLayout->addWidget(frame, 1);
    auto *layout = static_cast<QVBoxLayout *>(frame->layout());
    m_todoTitleWidget = layout->itemAt(0) ? layout->itemAt(0)->widget() : nullptr;
    m_todoCaptionWidget = layout->itemAt(1) ? layout->itemAt(1)->widget() : nullptr;

    m_todoRestoreRowWidget = new QWidget;
    m_todoRestoreRowWidget->setFixedHeight(34);
    // 小窗模式專用的還原列，完整模式下隱藏。
    auto *restoreLayout = new QHBoxLayout(m_todoRestoreRowWidget);
    restoreLayout->setContentsMargins(0, 0, 0, 4);
    restoreLayout->setSpacing(0);
    m_restoreButton = createButton("还原", "ghostButton");
    m_restoreButton->setFixedHeight(28);
    m_restoreButton->hide();
    restoreLayout->addWidget(m_restoreButton, 1);
    m_todoRestoreRowWidget->hide();
    layout->addWidget(m_todoRestoreRowWidget);
    connect(m_restoreButton, &QPushButton::clicked, this, [this]() {
        setTodoCompact(false);
    });

    m_todoInputRowWidget = new QWidget;
    auto *inputRow = new QHBoxLayout(m_todoInputRowWidget);
    inputRow->setContentsMargins(0, 0, 0, 0);
    inputRow->setSpacing(12);
    m_todoInput = new QLineEdit;
    m_todoInput->setPlaceholderText("添加一个任务，例如：复习线性代数第三章");
    auto *add = createButton("添加", "primaryButton");
    // 新增待辦輸入列：按 Enter 或「添加」都會建立待辦。
    inputRow->addWidget(m_todoInput, 1);
    inputRow->addWidget(add);
    layout->addWidget(m_todoInputRowWidget);

    m_todoList = new QListWidget;
    m_todoList->setObjectName("todoList");
    // 待辦列表：每個 item 內嵌一個 QCheckBox，便於保存勾選狀態。
    m_todoList->setMinimumHeight(360);
    m_todoList->setSpacing(8);
    m_todoList->setUniformItemSizes(true);
    m_todoList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_todoList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_todoList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    layout->addWidget(m_todoList, 1);
    connect(m_todoList, &QListWidget::itemDoubleClicked, this, [this]() {
        if (m_todoCompact) {
            setTodoCompact(false);
        }
    });

    m_cleanTodoButton = createButton("清除已完成");
    // 清除已完成只移除勾選項，不影響未完成任務。
    layout->addWidget(m_cleanTodoButton);

    addTodo("完成一组英语听力训练");
    addTodo("整理今天的错题");
    addTodo("阅读论文摘要并记录关键词");

    connect(add, &QPushButton::clicked, this, &MainWindow::addTodoFromInput);
    connect(m_todoInput, &QLineEdit::returnPressed, this, &MainWindow::addTodoFromInput);
    connect(m_cleanTodoButton, &QPushButton::clicked, this, &MainWindow::removeCompletedTodos);
}

void MainWindow::buildAmbienceCard(QVBoxLayout *pageLayout)
{
    // 環境氛圍頁：模式按鈕、音量、沉浸強度與播放控制。
    auto *frame = createCard("环境氛围", "选择适合当前学习阶段的背景氛围，并调整音量与沉浸强度。");
    pageLayout->addWidget(frame, 1);
    auto *layout = static_cast<QVBoxLayout *>(frame->layout());

    auto *modes = new QHBoxLayout;
    modes->setSpacing(12);
    auto *group = new QButtonGroup(this);
    group->setExclusive(true);

    const QStringList ambienceItems = {"雨夜书桌", "咖啡馆", "白噪音", "森林晨光"};
    for (int i = 0; i < ambienceItems.size(); ++i) {
        // 四個環境音模式按鈕，id 對應 ambienceUrl() 的音源列表。
        auto *button = createButton(ambienceItems.at(i), "ambientButton");
        button->setCheckable(true);
        button->setMinimumHeight(58);
        if (i == 0) {
            button->setChecked(true);
        }
        group->addButton(button, i);
        modes->addWidget(button, 1);
    }
    connect(group, qOverload<int>(&QButtonGroup::idClicked), this, &MainWindow::selectAmbience);
    layout->addLayout(modes);
    layout->addSpacing(22);

    auto *volumeLabel = new QLabel("氛围音量");
    volumeLabel->setObjectName("muted");
    auto *volume = new QSlider(Qt::Horizontal);
    // 音量滑桿直接控制 QAudioOutput 音量。
    volume->setRange(0, 100);
    volume->setValue(64);
    connect(volume, &QSlider::valueChanged, this, [this](int value) {
        if (m_audioOutput) {
            m_audioOutput->setVolume(value / 100.0);
        }
    });

    auto *depthLabel = new QLabel("沉浸强度");
    depthLabel->setObjectName("muted");
    auto *depth = new QSlider(Qt::Horizontal);
    // 沉浸強度目前是展示控件，保留給後續視覺/音效強度擴充。
    depth->setRange(0, 100);
    depth->setValue(72);

    layout->addWidget(volumeLabel);
    layout->addWidget(volume);
    layout->addSpacing(18);
    layout->addWidget(depthLabel);
    layout->addWidget(depth);
    m_playSoundButton = createButton("播放环境音", "soundButton");
    m_playSoundButton->setProperty("playing", false);
    m_playSoundButton->setMinimumHeight(46);
    // 播放/暫停環境音，音源優先使用本地 assets，否則 fallback 到網路音源。
    layout->addSpacing(18);
    layout->addWidget(m_playSoundButton);
    connect(m_playSoundButton, &QPushButton::clicked, this, &MainWindow::toggleAmbiencePlayback);
    layout->addStretch();
}

void MainWindow::buildOverviewCard(QVBoxLayout *pageLayout)
{
    // 狀態概覽頁：學習目標、清空進度與四個統計卡片。
    auto *frame = createCard("状态概览", "保持轻量反馈，专注于下一步行动。");
    pageLayout->addWidget(frame, 1);
    auto *layout = static_cast<QVBoxLayout *>(frame->layout());

    auto *goalRow = new QHBoxLayout;
    goalRow->setContentsMargins(0, 0, 0, 0);
    goalRow->addStretch();
    auto *clearProgressButton = createButton("清空学习进度", "dangerButton");
    clearProgressButton->setMinimumWidth(136);
    // 危險操作按鈕：會二次確認後清空 studyDays。
    goalRow->addWidget(clearProgressButton);
    m_goalButton = createButton("学习目标", "ghostButton");
    m_goalButton->setMinimumWidth(120);
    goalRow->addWidget(m_goalButton);
    layout->addLayout(goalRow);

    auto *grid = new QGridLayout;
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(16);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    layout->addLayout(grid, 1);

    auto addOverviewMetric = [this, grid](const QString &name, const QString &value, int progress, int row, int column, QProgressBar **progressBar) {
        // 統計小卡：標題、時間數值、進度條。
        auto *box = new QFrame;
        box->setObjectName("miniCard");
        box->setAttribute(Qt::WA_StyledBackground, true);
        m_shadowWidgets.append(box);

        auto *boxLayout = new QVBoxLayout(box);
        boxLayout->setContentsMargins(22, 20, 22, 20);
        boxLayout->setSpacing(12);

        auto *label = new QLabel(name);
        label->setObjectName("muted");
        auto *number = new QLabel(value);
        number->setObjectName("metricValue");
        auto *bar = new QProgressBar;
        bar->setRange(0, 100);
        bar->setValue(progress);
        bar->setTextVisible(false);
        if (progressBar) {
            *progressBar = bar;
        }

        boxLayout->addWidget(label);
        boxLayout->addStretch();
        boxLayout->addWidget(number);
        boxLayout->addStretch();
        boxLayout->addWidget(bar);
        grid->addWidget(box, row, column);
        return number;
    };

    m_todayStatsLabel = addOverviewMetric("今日学习", "0 分钟", 0, 0, 0, &m_todayProgressBar);
    // 四個統計卡片分成 2x2，進度條在 updateStatsLabels() 裡更新。
    m_weekStatsLabel = addOverviewMetric("本周学习", "0 分钟", 0, 0, 1, &m_weekProgressBar);
    m_monthStatsLabel = addOverviewMetric("本月学习", "0 分钟", 0, 1, 0, &m_monthProgressBar);
    m_totalStatsLabel = addOverviewMetric("累计学习", "0 分钟", 0, 1, 1, &m_totalProgressBar);

    connect(m_goalButton, &QPushButton::clicked, this, &MainWindow::showGoalDialog);
    connect(clearProgressButton, &QPushButton::clicked, this, [this]() {
        const auto result = QMessageBox::warning(
            this,
            "清空学习进度",
            "确定要清空所有学习时长记录吗？\n\n这个操作会清空今日、本周、本月和累计学习时长。",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (result != QMessageBox::Yes) {
            return;
        }

        m_dailyStudySeconds.clear();
        m_unsavedStudySeconds = 0;

        QSettings settings(settingsPath(), QSettings::IniFormat);
        settings.remove("studyDays");
        settings.sync();

        updateStatsLabels();
    });
}

void MainWindow::buildProfileCard(QVBoxLayout *pageLayout)
{
    // 個人簡介頁：左側介紹工具，右側顯示給使用者的話。
    auto *frame = createCard("About Me", "Nothing");
    pageLayout->addWidget(frame, 1);
    auto *layout = static_cast<QVBoxLayout *>(frame->layout());

    auto *row = new QHBoxLayout;
    row->setSpacing(16);
    layout->addLayout(row);

    auto *introBox = new QFrame;
    introBox->setObjectName("miniCard");
    introBox->setAttribute(Qt::WA_StyledBackground, true);
    m_shadowWidgets.append(introBox);
    auto *introLayout = new QVBoxLayout(introBox);
    introLayout->setContentsMargins(22, 20, 22, 20);
    introLayout->setSpacing(12);
    auto *introTitle = new QLabel("Find Me");
    introTitle->setObjectName("metricValue");
    introLayout->addWidget(introTitle);

    // 修改這裡為你的個人 Telegram 與 GitHub 首頁連結。
    const QUrl telegramUrl("https://t.me/Kino1337_Main");
    const QUrl githubUrl("https://github.com/Goder1337");
    auto *telegramButton = createButton("Telegram", "linkButton");
    auto *githubButton = createButton("GitHub", "linkButton");
    telegramButton->setMinimumHeight(38);
    githubButton->setMinimumHeight(38);
    introLayout->addWidget(telegramButton);
    introLayout->addWidget(githubButton);
    connect(telegramButton, &QPushButton::clicked, this, [telegramUrl]() {
        QDesktopServices::openUrl(telegramUrl);
    });
    connect(githubButton, &QPushButton::clicked, this, [githubUrl]() {
        QDesktopServices::openUrl(githubUrl);
    });

    introLayout->addStretch();
    row->addWidget(introBox, 1);

    auto *noteBox = new QFrame;
    noteBox->setObjectName("miniCard");
    // 右側寄語卡片，可在 noteText 文字處修改內容。
    noteBox->setAttribute(Qt::WA_StyledBackground, true);
    m_shadowWidgets.append(noteBox);
    auto *noteLayout = new QVBoxLayout(noteBox);
    noteLayout->setContentsMargins(22, 20, 22, 20);
    noteLayout->setSpacing(12);
    auto *noteTitle = new QLabel("To Users:");
    noteTitle->setObjectName("metricValue");
    auto *noteText = new QLabel(
        "该软件目前仅支持本地使用，作者只为统计学习时长因此暂未考虑线上自习室等联机功能。\r\n如果在使用过程中遇到问题或需要新功能，请给我留言，非常感谢你的建议");
    noteText->setWordWrap(true);
    noteText->setObjectName("profileText");
    noteLayout->addWidget(noteTitle);
    noteLayout->addWidget(noteText);
    noteLayout->addStretch();
    row->addWidget(noteBox, 1);

    layout->addStretch();
}

QLabel *MainWindow::addMetric(QHBoxLayout *row, const QString &name, const QString &value, int progress)
{
    // 舊版橫向統計卡片工具函式，保留給日後若要改回橫排使用。
    auto *box = new QFrame;
    box->setObjectName("miniCard");
    box->setAttribute(Qt::WA_StyledBackground, true);
    m_shadowWidgets.append(box);
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(10);

    auto *label = new QLabel(name);
    label->setObjectName("muted");
    auto *number = new QLabel(value);
    number->setObjectName("metricValue");
    auto *bar = new QProgressBar;
    bar->setRange(0, 100);
    bar->setValue(progress);
    bar->setTextVisible(false);

    layout->addWidget(label);
    layout->addWidget(number);
    layout->addWidget(bar);
    row->addWidget(box, 1);
    return number;
}

void MainWindow::showGoalDialog()
{
    // 學習目標彈窗：使用主視窗 QSS，並用自繪標題列保持白天/夜晚一致。
    QDialog dialog(this);
    dialog.setObjectName("root");
    dialog.setAttribute(Qt::WA_StyledBackground, true);
    dialog.setStyleSheet(styleSheet());
    dialog.setWindowFlags(dialog.windowFlags() | Qt::FramelessWindowHint);
    dialog.setWindowTitle("学习目标");
    dialog.setModal(true);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(0, 0, 0, 18);
    layout->setSpacing(14);

    auto *titleBar = new QWidget;
    titleBar->setObjectName("dialogTitleBar");
    titleBar->setFixedHeight(38);
    titleBar->setCursor(Qt::SizeAllCursor);
    titleBar->installEventFilter(new DialogDragFilter(&dialog, &dialog));
    // titleBar 安裝 DialogDragFilter 後即可按住拖動彈窗。
    auto *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(16, 4, 8, 4);
    titleLayout->setSpacing(8);
    auto *titleLabel = new QLabel("学习目标");
    titleLabel->setObjectName("titleBarText");
    auto *closeButton = new QPushButton("×");
    closeButton->setObjectName("titleCloseButton");
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setFixedSize(28, 28);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(closeButton);
    layout->addWidget(titleBar);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    auto *form = new QFormLayout;
    form->setContentsMargins(18, 0, 18, 0);
    form->setSpacing(12);

    auto *dailySpin = new QSpinBox;
    // 每日目標，UI 以分鐘顯示，保存時轉成秒。
    dailySpin->setRange(1, 24 * 60);
    dailySpin->setSuffix(" 分钟");
    dailySpin->setValue(qMax(1, m_dailyGoalSeconds / 60));
    dailySpin->setAlignment(Qt::AlignCenter);

    auto *weeklySpin = new QSpinBox;
    // 每週目標，UI 以分鐘顯示，保存時轉成秒。
    weeklySpin->setRange(1, 7 * 24 * 60);
    weeklySpin->setSuffix(" 分钟");
    weeklySpin->setValue(qMax(1, m_weeklyGoalSeconds / 60));
    weeklySpin->setAlignment(Qt::AlignCenter);

    auto *monthlySpin = new QSpinBox;
    // 每月目標，UI 以小時顯示，保存時轉成秒。
    monthlySpin->setRange(1, 31 * 24);
    monthlySpin->setSuffix(" 小时");
    monthlySpin->setValue(qMax(1, m_monthlyGoalSeconds / 3600));
    monthlySpin->setAlignment(Qt::AlignCenter);

    auto makeFormLabel = [](const QString &text) {
        auto *label = new QLabel(text);
        label->setObjectName("muted");
        return label;
    };
    form->addRow(makeFormLabel("每日目标"), dailySpin);
    form->addRow(makeFormLabel("每周目标"), weeklySpin);
    form->addRow(makeFormLabel("每月目标"), monthlySpin);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    // 保存/取消沿用主視窗的 primaryButton 與 ghostButton 樣式。
    buttons->button(QDialogButtonBox::Ok)->setText("保存");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    buttons->button(QDialogButtonBox::Ok)->setObjectName("primaryButton");
    buttons->button(QDialogButtonBox::Cancel)->setObjectName("ghostButton");
    buttons->button(QDialogButtonBox::Ok)->setMinimumHeight(38);
    buttons->button(QDialogButtonBox::Cancel)->setMinimumHeight(38);
    installButtonAnimation(buttons->button(QDialogButtonBox::Ok));
    installButtonAnimation(buttons->button(QDialogButtonBox::Cancel));
    layout->setContentsMargins(0, 0, 0, 18);
    buttons->setContentsMargins(18, 0, 18, 0);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_dailyGoalSeconds = dailySpin->value() * 60;
    m_weeklyGoalSeconds = weeklySpin->value() * 60;
    m_monthlyGoalSeconds = monthlySpin->value() * 3600;
    updateStatsLabels();
    saveState();
}

void MainWindow::addTodo(const QString &text, bool checked)
{
    if (text.trimmed().isEmpty()) {
        return;
    }

    // 每個待辦項目由 QListWidgetItem 承載，實際可點擊控件是 QCheckBox。
    auto *item = new QListWidgetItem;
    auto *checkBox = new QCheckBox(text.trimmed());
    checkBox->setObjectName("todoCheck");
    checkBox->setCursor(Qt::PointingHandCursor);
    if (m_todoCompact) {
        // 小窗模式使用 compact 屬性觸發較緊湊的 QSS。
        checkBox->setProperty("compact", true);
        checkBox->setFixedWidth(190);
    }
    checkBox->setChecked(checked);
    connect(checkBox, &QCheckBox::toggled, this, &MainWindow::saveTodos);

    item->setSizeHint(QSize(0, m_todoCompact ? 44 : 60));
    m_todoList->addItem(item);
    m_todoList->setItemWidget(item, checkBox);
    updateTodoCompactSize();
}

void MainWindow::addTodoFromInput()
{
    addTodo(m_todoInput->text());
    m_todoInput->clear();
    saveTodos();
}

void MainWindow::removeCompletedTodos()
{
    // 從尾端開始刪除，避免刪除過程中索引前移造成漏刪。
    for (int i = m_todoList->count() - 1; i >= 0; --i) {
        QListWidgetItem *item = m_todoList->item(i);
        auto *checkBox = qobject_cast<QCheckBox *>(m_todoList->itemWidget(item));
        if (checkBox && checkBox->isChecked()) {
            delete m_todoList->takeItem(i);
        }
    }
    saveTodos();
    updateTodoCompactSize();
}

void MainWindow::toggleTimer()
{
    // 開始/暫停/繼續共用同一顆按鈕，根據 timer 狀態切換文字。
    if (m_timer.isActive()) {
        m_timer.stop();
        if (m_ring) {
            m_ring->setGlow(0.0);
        }
        flushStudyStats();
        m_startButton->setText("继续");
        m_ring->setText(formatTime(m_remainingSeconds), "已暂停");
        return;
    }

    if (m_remainingSeconds <= 0) {
        resetTimer();
    }

    m_timer.start();
    m_startButton->setText("暂停");
    m_ring->setText(formatTime(m_remainingSeconds), "专注模式");
}

void MainWindow::resetTimer()
{
    // 重置為目前專注時長，並刷新圓環顯示。
    m_timer.stop();
    if (m_ring) {
        m_ring->setGlow(0.0);
    }
    flushStudyStats();
    m_totalSeconds = m_focusSpin ? m_focusSpin->value() * 60 : 25 * 60;
    m_remainingSeconds = m_totalSeconds;

    if (m_startButton) {
        m_startButton->setText("开始");
    }

    updateRing();
}

void MainWindow::tick()
{
    // 每秒倒數，並把專注秒數累加到今日統計。
    if (m_remainingSeconds > 0) {
        --m_remainingSeconds;
        addStudySecond();
    }

    updateRing();

    if (m_remainingSeconds <= 0) {
        // 倒數完成：停止計時、保存統計、更新文字並發出 Windows 通知。
        m_timer.stop();
        if (m_ring) {
            m_ring->setGlow(0.0);
        }
        flushStudyStats();
        m_startButton->setText("再来一轮");
        m_ring->setText("00:00", "完成，休息一下");
        showPomodoroFinishedMessage();
    }
}

void MainWindow::updateRing()
{
    // 讓 FocusRing 的文字與進度同步目前倒數狀態。
    if (!m_ring) {
        return;
    }

    const double progress = m_totalSeconds == 0
        ? 0.0
        : static_cast<double>(m_remainingSeconds) / static_cast<double>(m_totalSeconds);

    m_ring->setText(formatTime(m_remainingSeconds), m_timer.isActive() ? "专注模式" : "准备开始");
    if (!m_progressAnimation) {
        m_ring->setProgress(progress);
        return;
    }

    m_progressAnimation->stop();
    m_progressAnimation->setStartValue(m_ring->progress());
    m_progressAnimation->setEndValue(progress);
    m_progressAnimation->start();
}

void MainWindow::selectTab(int index)
{
    // 切換 QStackedWidget 頁面，同時播放標籤氣泡移動動畫。
    if (!m_featureStack || index < 0 || index >= m_featureStack->count()) {
        return;
    }

    m_featureStack->setCurrentIndex(index);
    moveTabBubble(index, true);
}

void MainWindow::moveTabBubble(int index, bool animated)
{
    // 標籤列的藍色氣泡，目標位置跟隨目前選中的 tabButton。
    if (!m_tabBubble || index < 0 || index >= m_tabButtons.size()) {
        return;
    }

    QPushButton *button = m_tabButtons.at(index);
    const QRect target = button->geometry().adjusted(0, 0, 0, 0);

    m_tabBubble->show();
    m_tabBubble->lower();

    if (!animated || m_tabBubble->geometry().isNull()) {
        m_tabBubble->setGeometry(target);
        return;
    }

    auto *animation = new QPropertyAnimation(m_tabBubble, "geometry", m_tabBubble);
    animation->setDuration(360);
    animation->setStartValue(m_tabBubble->geometry());
    animation->setEndValue(target);
    animation->setEasingCurve(QEasingCurve::OutBack);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::moveThemeBubble(bool animated)
{
    // 白天/夜晚切換的氣泡，依 m_nightMode 移到對應按鈕下方。
    if (!m_themeBubble || !m_dayButton || !m_nightButton) {
        return;
    }

    QPushButton *button = m_nightMode ? m_nightButton : m_dayButton;
    const QRect target = button->geometry();
    m_themeBubble->show();
    m_themeBubble->lower();

    if (!animated || m_themeBubble->geometry().isNull()) {
        m_themeBubble->setGeometry(target);
        return;
    }

    auto *animation = new QPropertyAnimation(m_themeBubble, "geometry", m_themeBubble);
    animation->setDuration(300);
    animation->setStartValue(m_themeBubble->geometry());
    animation->setEndValue(target);
    animation->setEasingCurve(QEasingCurve::OutBack);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::toggleAlwaysOnTop()
{
    // 標題列「置」按鈕：切換 Windows 置頂旗標並保存。
    m_alwaysOnTop = m_pinButton && m_pinButton->isChecked();
    setWindowFlag(Qt::WindowStaysOnTopHint, m_alwaysOnTop);
    show();
    saveState();
}

void MainWindow::toggleTodoCompact()
{
    setTodoCompact(m_compactButton && m_compactButton->isChecked());
}

void MainWindow::setTodoCompact(bool compact)
{
    // 待辦小窗模式：隱藏主介面，只留下標題列、還原按鈕與待辦列表。
    if (compact == m_todoCompact) {
        return;
    }

    m_todoCompact = compact;
    if (m_compactButton) {
        m_compactButton->setChecked(compact);
    }

    if (compact) {
        // 進入小窗前保存完整視窗位置，還原時用 m_normalGeometry 回去。
        m_normalGeometry = geometry();
        if (m_featureStack) {
            m_featureStack->setCurrentIndex(1);
        }
        if (m_headerWidget) {
            m_headerWidget->hide();
        }
        if (m_tabBar) {
            m_tabBar->hide();
        }
        if (m_contentWidget && m_contentWidget->layout()) {
            m_contentWidget->layout()->setContentsMargins(6, 4, 6, 6);
        }
        if (m_todoCard && m_todoCard->layout()) {
            m_todoCard->layout()->setContentsMargins(8, 8, 8, 24);
            m_todoCard->layout()->setSpacing(6);
        }
        if (m_todoTitleWidget) {
            m_todoTitleWidget->hide();
        }
        if (m_todoCaptionWidget) {
            m_todoCaptionWidget->hide();
        }
        if (m_todoInputRowWidget) {
            m_todoInputRowWidget->hide();
        }
        if (m_cleanTodoButton) {
            m_cleanTodoButton->hide();
        }
        if (m_todoRestoreRowWidget) {
            m_todoRestoreRowWidget->show();
        }
        if (m_restoreButton) {
            m_restoreButton->show();
        }
        if (m_todoList) {
            // 小窗模式下待辦更窄、更緊湊，最多顯示三條。
            m_todoList->setSpacing(4);
            m_todoList->setMinimumHeight(56);
            m_todoList->setMaximumHeight(QWIDGETSIZE_MAX);
            m_todoList->setToolTip("双击待办列表恢复完整窗口");
            for (int i = 0; i < m_todoList->count(); ++i) {
                QListWidgetItem *item = m_todoList->item(i);
                item->setSizeHint(QSize(0, 44));
                if (auto *checkBox = qobject_cast<QCheckBox *>(m_todoList->itemWidget(item))) {
                    checkBox->setProperty("compact", true);
                    checkBox->setFixedWidth(190);
                    checkBox->style()->unpolish(checkBox);
                    checkBox->style()->polish(checkBox);
                }
            }
        }
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
        show();
        setWindowTitle("StudyWithMe - 双击恢复");
        updateTodoCompactSize();
    } else {
        // 退出小窗：恢復完整視窗所有標題、輸入列與清除按鈕。
        if (m_headerWidget) {
            m_headerWidget->show();
        }
        if (m_tabBar) {
            m_tabBar->show();
        }
        if (m_contentWidget && m_contentWidget->layout()) {
            m_contentWidget->layout()->setContentsMargins(21, 12, 21, 21);
        }
        if (m_todoCard && m_todoCard->layout()) {
            m_todoCard->layout()->setContentsMargins(24, 22, 24, 24);
            m_todoCard->layout()->setSpacing(14);
        }
        if (m_todoTitleWidget) {
            m_todoTitleWidget->show();
        }
        if (m_todoCaptionWidget) {
            m_todoCaptionWidget->show();
        }
        if (m_todoInputRowWidget) {
            m_todoInputRowWidget->show();
        }
        if (m_cleanTodoButton) {
            m_cleanTodoButton->show();
        }
        if (m_todoRestoreRowWidget) {
            m_todoRestoreRowWidget->hide();
        }
        if (m_restoreButton) {
            m_restoreButton->hide();
        }
        if (m_todoList) {
            m_todoList->setSpacing(8);
            m_todoList->setMinimumHeight(360);
            m_todoList->setMaximumHeight(QWIDGETSIZE_MAX);
            m_todoList->setToolTip("");
            for (int i = 0; i < m_todoList->count(); ++i) {
                QListWidgetItem *item = m_todoList->item(i);
                item->setSizeHint(QSize(0, 60));
                if (auto *checkBox = qobject_cast<QCheckBox *>(m_todoList->itemWidget(item))) {
                    checkBox->setProperty("compact", false);
                    checkBox->setMinimumWidth(0);
                    checkBox->setMaximumWidth(QWIDGETSIZE_MAX);
                    checkBox->setMinimumHeight(0);
                    checkBox->setMaximumHeight(QWIDGETSIZE_MAX);
                    checkBox->style()->unpolish(checkBox);
                    checkBox->style()->polish(checkBox);
                }
            }
        }
        setWindowFlag(Qt::WindowStaysOnTopHint, m_alwaysOnTop);
        show();
        setMinimumSize(920, 620);
        if (m_normalGeometry.isValid()) {
            setGeometry(m_normalGeometry);
        } else {
            resize(1120, 720);
        }
        setWindowTitle("StudyWithMe");
    }

    QTimer::singleShot(0, this, [this]() {
        moveTabBubble(m_featureStack ? m_featureStack->currentIndex() : 0, false);
    });
    saveState();
}

void MainWindow::updateTodoCompactSize()
{
    // 小窗尺寸計算：依待辦數量自適應，超過三條固定為三條高度並用滾輪瀏覽。
    if (!m_todoCompact || !m_todoList) {
        return;
    }

    const int visibleItems = qBound(1, m_todoList->count(), 3);
    const int itemHeight = 44;
    const int itemSpacing = m_todoList->spacing();
    const int titleHeight = m_titleBar ? m_titleBar->height() : 30;
    const int contentMargins = 28;
    const int restoreArea = 38;
    const int listHeight = visibleItems * itemHeight + (visibleItems - 1) * itemSpacing + 42;
    const int windowHeight = titleHeight + contentMargins + restoreArea + listHeight + 18;

    m_todoList->setMinimumHeight(listHeight);
    m_todoList->setMaximumHeight(listHeight);
    setMinimumSize(214, windowHeight);
    resize(248, windowHeight);
}

void MainWindow::setupPersistence()
{
    // 建立 ini 檔所在資料夾，所有狀態都寫入 settingsPath()。
    const QFileInfo info(settingsPath());
    QDir().mkpath(info.absolutePath());
}

QString MainWindow::settingsPath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + "/AppData/Roaming/StudyWithMe";
    }
    QDir().mkpath(dir);
    return dir + "/settings.ini";
}

void MainWindow::loadState()
{
    // 啟動時讀取 UI、番茄鐘、學習目標、音訊與待辦狀態。
    QSettings settings(settingsPath(), QSettings::IniFormat);

    m_nightMode = settings.value("ui/nightMode", false).toBool();
    applyTheme(m_nightMode);

    m_alwaysOnTop = settings.value("ui/alwaysOnTop", false).toBool();
    if (m_pinButton) {
        m_pinButton->setChecked(m_alwaysOnTop);
    }
    setWindowFlag(Qt::WindowStaysOnTopHint, m_alwaysOnTop);
    show();
    const QByteArray geometry = settings.value("ui/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }

    if (m_focusSpin) {
        m_focusSpin->setValue(settings.value("timer/focusMinutes", m_focusSpin->value()).toInt());
    }
    if (m_breakSpin) {
        m_breakSpin->setValue(settings.value("timer/breakMinutes", m_breakSpin->value()).toInt());
    }
    m_dailyGoalSeconds = settings.value("goals/dailySeconds", 120 * 60).toInt();
    m_weeklyGoalSeconds = settings.value("goals/weeklySeconds", 720 * 60).toInt();
    m_monthlyGoalSeconds = settings.value("goals/monthlySeconds", 21 * 60 * 60).toInt();

    m_dailyStudySeconds.clear();
    settings.beginGroup("studyDays");
    const QStringList days = settings.childKeys();
    for (const QString &day : days) {
        const QDate date = QDate::fromString(day, Qt::ISODate);
        if (date.isValid()) {
            m_dailyStudySeconds[date] = settings.value(day, 0).toInt();
        }
    }
    settings.endGroup();

    m_ambienceIndex = settings.value("audio/ambienceIndex", 0).toInt();
    if (m_audioOutput) {
        m_audioOutput->setVolume(settings.value("audio/volume", 0.64).toDouble());
    }
    selectAmbience(m_ambienceIndex);

    loadTodos();
    updateStatsLabels();
    if (settings.value("ui/todoCompact", false).toBool()) {
        setTodoCompact(true);
    }
}

void MainWindow::saveState()
{
    // 關閉或切換重要狀態時保存設定，避免重啟後丟失。
    flushStudyStats();
    saveTodos();

    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue("ui/nightMode", m_nightMode);
    settings.setValue("ui/alwaysOnTop", m_alwaysOnTop);
    settings.setValue("ui/todoCompact", m_todoCompact);
    if (!m_todoCompact) {
        settings.setValue("ui/geometry", saveGeometry());
    }
    if (m_focusSpin) {
        settings.setValue("timer/focusMinutes", m_focusSpin->value());
    }
    if (m_breakSpin) {
        settings.setValue("timer/breakMinutes", m_breakSpin->value());
    }
    settings.setValue("goals/dailySeconds", m_dailyGoalSeconds);
    settings.setValue("goals/weeklySeconds", m_weeklyGoalSeconds);
    settings.setValue("goals/monthlySeconds", m_monthlyGoalSeconds);
    settings.setValue("audio/ambienceIndex", m_ambienceIndex);
    if (m_audioOutput) {
        settings.setValue("audio/volume", m_audioOutput->volume());
    }
    settings.sync();
}

void MainWindow::loadTodos()
{
    // 從 ini 的 todos/items 陣列讀回待辦文字與勾選狀態。
    QSettings settings(settingsPath(), QSettings::IniFormat);
    if (!settings.value("todos/saved", false).toBool() || !m_todoList) {
        return;
    }

    m_todoList->clear();
    const int count = settings.beginReadArray("todos/items");
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        addTodo(settings.value("text").toString(), settings.value("checked", false).toBool());
    }
    settings.endArray();
}

void MainWindow::saveTodos()
{
    // 將目前列表完整寫回 ini；每條待辦保存 text 與 checked。
    if (!m_todoList) {
        return;
    }

    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue("todos/saved", true);
    settings.beginWriteArray("todos/items");
    for (int i = 0; i < m_todoList->count(); ++i) {
        QListWidgetItem *item = m_todoList->item(i);
        auto *checkBox = qobject_cast<QCheckBox *>(m_todoList->itemWidget(item));
        if (!checkBox) {
            continue;
        }
        settings.setArrayIndex(i);
        settings.setValue("text", checkBox->text());
        settings.setValue("checked", checkBox->isChecked());
    }
    settings.endArray();
    settings.sync();
}

void MainWindow::addStudySecond()
{
    // 番茄鐘運行時每秒累加今日學習時間，累計 10 秒後落盤一次。
    m_dailyStudySeconds[QDate::currentDate()] += 1;
    ++m_unsavedStudySeconds;
    updateStatsLabels();
    if (m_unsavedStudySeconds >= 10) {
        flushStudyStats();
    }
}

void MainWindow::flushStudyStats()
{
    // 將每日學習秒數寫入 studyDays 群組。
    if (m_unsavedStudySeconds <= 0 && !m_dailyStudySeconds.isEmpty()) {
        return;
    }

    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.beginGroup("studyDays");
    for (auto it = m_dailyStudySeconds.cbegin(); it != m_dailyStudySeconds.cend(); ++it) {
        settings.setValue(it.key().toString(Qt::ISODate), it.value());
    }
    settings.endGroup();
    settings.sync();
    m_unsavedStudySeconds = 0;
}

void MainWindow::updateStatsLabels()
{
    // 更新四個統計數值與進度條：今日、本週、本月、累計。
    const int today = studySecondsForDay(QDate::currentDate());
    const int week = studySecondsForCurrentWeek();
    const int month = studySecondsForCurrentMonth();
    int total = 0;
    for (int seconds : m_dailyStudySeconds) {
        total += seconds;
    }

    if (m_todayStatsLabel) {
        m_todayStatsLabel->setText(formatDuration(today));
    }
    if (m_weekStatsLabel) {
        m_weekStatsLabel->setText(formatDuration(week));
    }
    if (m_monthStatsLabel) {
        m_monthStatsLabel->setText(formatDuration(month));
    }
    if (m_totalStatsLabel) {
        m_totalStatsLabel->setText(formatDuration(total));
    }

    auto progress = [](int seconds, int goalSeconds) {
        // 將實際秒數轉成 0~100 的進度條百分比。
        if (goalSeconds <= 0) {
            return 0;
        }
        return qBound(0, static_cast<int>((static_cast<qint64>(seconds) * 100) / goalSeconds), 100);
    };
    if (m_todayProgressBar) {
        m_todayProgressBar->setValue(progress(today, m_dailyGoalSeconds));
    }
    if (m_weekProgressBar) {
        m_weekProgressBar->setValue(progress(week, m_weeklyGoalSeconds));
    }
    if (m_monthProgressBar) {
        m_monthProgressBar->setValue(progress(month, m_monthlyGoalSeconds));
    }
    if (m_totalProgressBar) {
        m_totalProgressBar->setValue(progress(total, m_monthlyGoalSeconds));
    }
}

int MainWindow::studySecondsForDay(const QDate &date) const
{
    return m_dailyStudySeconds.value(date, 0);
}

int MainWindow::studySecondsForCurrentWeek() const
{
    // 以 Qt 的 dayOfWeek() 計算本週一到今天的總秒數。
    const QDate today = QDate::currentDate();
    const QDate weekStart = today.addDays(1 - today.dayOfWeek());
    int total = 0;
    for (auto it = m_dailyStudySeconds.cbegin(); it != m_dailyStudySeconds.cend(); ++it) {
        if (it.key() >= weekStart && it.key() <= today) {
            total += it.value();
        }
    }
    return total;
}

int MainWindow::studySecondsForCurrentMonth() const
{
    // 只累加當前年月的學習秒數。
    const QDate today = QDate::currentDate();
    int total = 0;
    for (auto it = m_dailyStudySeconds.cbegin(); it != m_dailyStudySeconds.cend(); ++it) {
        if (it.key().year() == today.year() && it.key().month() == today.month()) {
            total += it.value();
        }
    }
    return total;
}

QString MainWindow::formatDuration(int seconds) const
{
    // 統一格式化學習時長，超過一小時顯示「小時 + 分鐘」。
    const int hours = seconds / 3600;
    const int minutes = (seconds % 3600) / 60;
    if (hours > 0) {
        return QString("%1 小时 %2 分钟").arg(hours).arg(minutes);
    }
    return QString("%1 分钟").arg(minutes);
}

void MainWindow::setupTrayIcon()
{
    // 系統托盤用於番茄鐘完成通知與雙擊還原視窗。
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    m_trayIcon = new QSystemTrayIcon(style()->standardIcon(QStyle::SP_ComputerIcon), this);
    m_trayIcon->setToolTip("StudyWithMe");
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
            if (m_todoCompact) {
                setTodoCompact(false);
            }
            showNormal();
            raise();
            activateWindow();
        }
    });
    m_trayIcon->show();
}

void MainWindow::showPomodoroFinishedMessage()
{
    // 番茄鐘結束時右下角 Windows 通知。
    if (m_trayIcon) {
        m_trayIcon->showMessage("番茄钟完成", "这一轮专注结束了，休息一下再继续。", QSystemTrayIcon::Information, 6000);
    }
}

void MainWindow::setupAudio()
{
    // Qt Multimedia 音訊初始化：播放器、輸出與循環播放。
    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(0.64f);
    m_audioPlayer = new QMediaPlayer(this);
    m_audioPlayer->setAudioOutput(m_audioOutput);
    m_audioPlayer->setLoops(QMediaPlayer::Infinite);
    connect(m_audioPlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        // 播放狀態可能由按鈕、音源切換或播放器內部改變，統一從這裡刷新 UI。
        updateAmbienceButton(state == QMediaPlayer::PlayingState);
    });
    connect(m_audioPlayer, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &) {
        // 網路音源或解碼失敗時，立即把按鈕恢復成可播放狀態，避免 UI 卡在「暫停環境音」。
        updateAmbienceButton(false);
    });
    selectAmbience(0);
    updateAmbienceButton(false);
}

void MainWindow::selectAmbience(int index)
{
    // 切換環境音，若原本正在播放則換源後繼續播放。
    m_ambienceIndex = qBound(0, index, 3);
    if (!m_audioPlayer) {
        return;
    }

    const bool wasPlaying = m_audioPlayer->playbackState() == QMediaPlayer::PlayingState;
    m_audioPlayer->setSource(ambienceUrl(m_ambienceIndex));
    if (wasPlaying) {
        m_audioPlayer->play();
    } else {
        updateAmbienceButton(false);
    }
}

void MainWindow::toggleAmbiencePlayback()
{
    // 播放/暫停環境音按鈕；點擊判斷優先看目前 UI 狀態，避免播放器緩衝/停止時第二次點擊仍走播放分支。
    if (!m_audioPlayer || !m_playSoundButton) {
        return;
    }
    const bool shownAsPlaying = m_playSoundButton->property("playing").toBool();
    if (shownAsPlaying || m_audioPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_audioPlayer->pause();
        updateAmbienceButton(false);
    } else {
        if (m_audioPlayer->source().isEmpty()) {
            m_audioPlayer->setSource(ambienceUrl(m_ambienceIndex));
        }
        m_audioPlayer->play();
        updateAmbienceButton(true);
    }
}

void MainWindow::updateAmbienceButton(bool playing)
{
    // 只由此函式更新環境音按鈕文字與動態屬性，避免再次點擊時文字、顏色不同步。
    if (!m_playSoundButton) {
        return;
    }
    m_playSoundButton->setText(playing ? "暂停环境音" : "播放环境音");
    m_playSoundButton->setProperty("playing", playing);
    m_playSoundButton->style()->unpolish(m_playSoundButton);
    m_playSoundButton->style()->polish(m_playSoundButton);
    m_playSoundButton->update();
}

QUrl MainWindow::ambienceUrl(int index) const
{
    // 優先使用部署目錄 assets/audio 下的本地音檔。
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList localFiles = {
        appDir + "/assets/audio/rain.ogg",
        appDir + "/assets/audio/cafe.wav",
        appDir + "/assets/audio/white_noise.ogg",
        appDir + "/assets/audio/forest.ogg"
    };
    if (index >= 0 && index < localFiles.size() && QFileInfo::exists(localFiles.at(index))) {
        return QUrl::fromLocalFile(localFiles.at(index));
    }

    const QStringList remoteUrls = {
        // 本地檔不存在時使用網路 fallback，方便專案先能播放。
        "https://commons.wikimedia.org/wiki/Special:Redirect/file/Rain%20and%20thunder.ogg",
        "https://commons.wikimedia.org/wiki/Special:Redirect/file/Restaurant%20ambience%2C%20early%20morning%2C%20A.wav",
        "https://commons.wikimedia.org/wiki/Special:Redirect/file/White%20noise.ogg",
        "https://commons.wikimedia.org/wiki/Special:Redirect/file/Blackbird%20song.ogg"
    };
    return QUrl(remoteUrls.value(index, remoteUrls.first()));
}

void MainWindow::applyTheme(bool nightMode)
{
    // 全域主題入口：集中控制所有控件顏色與關鍵渲染樣式。
    m_nightMode = nightMode;
    if (m_dayButton) {
        m_dayButton->setChecked(!nightMode);
    }
    if (m_nightButton) {
        m_nightButton->setChecked(nightMode);
    }
    if (m_ring) {
        m_ring->setNightMode(nightMode);
    }

    // 主背景、卡片、邊框與按鈕底色；白天模式刻意壓低亮度以凸顯控件邊界。
    const QString rootBg = nightMode
        ? "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #0f1015, stop:0.48 #191a22, stop:1 #101116)"
        : "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #f2f5f9, stop:0.46 #e5edf7, stop:1 #f0edf7)";
    const QString rootText = nightMode ? "#f5f5f7" : "#1d1d1f";
    const QString cardBg = nightMode ? "rgba(32, 33, 39, 235)" : "rgba(250, 252, 255, 236)";
    const QString cardBorder = nightMode ? "rgba(255, 255, 255, 52)" : "rgba(138, 154, 178, 120)";
    const QString miniBg = nightMode
        ? "rgba(42, 43, 49, 230)"
        : "rgba(245, 248, 252, 224)";
    const QString muted = nightMode ? "#a1a1aa" : "#6e6e73";
    const QString controlBg = nightMode ? "#2c2c30" : "#f8fafc";
    const QString controlBorder = nightMode ? "#46464d" : "#b7c1cf";
    const QString activeBg = nightMode ? "#323238" : "#dceeff";
    const QString glassButtonBg = nightMode ? "rgba(255, 255, 255, 28)" : "rgba(236, 242, 250, 205)";
    const QString glassButtonHover = nightMode ? "rgba(255, 255, 255, 45)" : "rgba(224, 237, 249, 236)";
    // 標籤列與白天/夜晚切換共用的滑動氣泡。
    const QString bubbleBg = nightMode
        ? "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 rgba(118, 214, 255, 190), stop:0.55 rgba(89, 196, 246, 174), stop:1 rgba(146, 212, 255, 160))"
        : "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 rgba(144, 224, 255, 196), stop:0.55 rgba(94, 203, 255, 180), stop:1 rgba(185, 236, 255, 168))";

    // 主要 QSS：若要改 UI 色彩、圓角、邊框、間距，大多從這段開始調。
    setStyleSheet(QString(R"(
        /* 根節點：主視窗與學習目標彈窗共用。 */
        QWidget#root {
            background: %1;
            color: %2;
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
            font-size: 14px;
            border: 1px solid %4;
            border-radius: 10px;
        }
        /* 自繪標題列：主視窗與學習目標彈窗共用。 */
        QWidget#titleBar, QWidget#dialogTitleBar {
            background: %5;
            border-top-left-radius: 10px;
            border-top-right-radius: 10px;
        }
        QLabel#titleBarText {
            color: %2;
            font-size: 13px;
            font-weight: 700;
        }
        QPushButton#titleButton, QPushButton#titleCloseButton {
            background: transparent;
            border: none;
            border-radius: 7px;
            color: %6;
            font-weight: 700;
            padding: 0;
        }
        QPushButton#titleButton:hover, QPushButton#titleButton:checked {
            background: %8;
            color: %2;
        }
        QPushButton#titleCloseButton:hover {
            background: #ff453a;
            color: #ffffff;
        }
        QWidget#tabPage {
            background: transparent;
        }
        QFrame#tabShell {
            background: transparent;
            border: none;
        }
        /* 五個功能標籤與滑動氣泡。 */
        QFrame#customTabBar {
            background: %5;
            border: 1px solid %4;
            border-radius: 20px;
        }
        QFrame#tabBubble {
            background: %10;
            border: 1px solid %4;
            border-radius: 16px;
        }
        QFrame#themeBubble {
            background: %10;
            border: 1px solid %4;
            border-radius: 12px;
        }
        QWidget#buttonBubble {
            background: %10;
            border: 1px solid %4;
            border-radius: 13px;
        }
        QStackedWidget#featureStack {
            background: transparent;
            border: none;
        }
        /* 主要玻璃卡片與小卡片外觀。 */
        QFrame#card {
            background: %3;
            border: 1px solid %4;
            border-radius: 26px;
        }
        QFrame#miniCard, QFrame#themeSwitch {
            background: %5;
            border: 1px solid %4;
            border-radius: 18px;
        }
        QDialogButtonBox {
            background: transparent;
        }
        /* 文字層級：頂部主標、卡片標題、統計數字、弱化文字。 */
        QLabel#headline {
            font-size: 30px;
            font-weight: 700;
            color: %2;
        }
        QLabel#eyebrow {
            color: #36bdf2;
            font-size: 12px;
            font-weight: 700;
            letter-spacing: 0px;
        }
        QLabel#cardTitle {
            font-size: 22px;
            font-weight: 700;
            color: %2;
        }
        QLabel#metricValue {
            font-size: 26px;
            font-weight: 700;
            color: %2;
        }
        QLabel#profileText {
            color: %6;
            font-size: 15px;
            line-height: 150%;
        }
        QLabel#muted {
            color: %6;
        }
        /* 標籤按鈕：checked 背景由 tabBubble 呈現。 */
        QPushButton#tabButton {
            min-width: 112px;
            min-height: 40px;
            padding: 0 18px;
            border-radius: 16px;
            background: %11;
            color: %6;
            font-weight: 700;
            border: 1px solid %9;
        }
        QPushButton#tabButton:checked {
            background: transparent;
            color: #ffffff;
            border: 1px solid transparent;
        }
        QPushButton#tabButton:hover:!checked {
            background: %12;
            color: %2;
        }
        /* 通用按鈕與各類按鈕變體。 */
        QPushButton {
            border: none;
            border-radius: 14px;
            padding: 0 18px;
            font-weight: 700;
        }
        QPushButton#primaryButton {
            background: rgba(94, 203, 255, 185);
            color: #ffffff;
            border: 1px solid rgba(35, 157, 214, 130);
        }
        QPushButton#primaryButton:hover {
            background: rgba(141, 222, 255, 220);
        }
        QPushButton#soundButton {
            background: rgba(94, 203, 255, 155);
            color: %2;
            border: 1px solid rgba(35, 157, 214, 120);
            min-height: 46px;
        }
        QPushButton#soundButton:hover {
            background: rgba(141, 222, 255, 205);
        }
        QPushButton#soundButton[playing="true"] {
            background: rgba(255, 149, 0, 190);
            color: #ffffff;
            border: 1px solid rgba(210, 118, 0, 150);
        }
        QPushButton#soundButton[playing="true"]:hover {
            background: rgba(255, 168, 42, 220);
        }
        QPushButton#ghostButton, QPushButton#pillButton, QPushButton#profileButton {
            background: %11;
            color: %2;
            border: 1px solid %9;
        }
        QPushButton#linkButton {
            background: %11;
            color: %2;
            border: 1px solid %9;
            text-align: left;
            padding: 0 16px;
        }
        QPushButton#linkButton:hover {
            background: %12;
            border: 1px solid rgba(94, 203, 255, 190);
        }
        QPushButton#dangerButton {
            background: #ff3b30;
            color: #ffffff;
            border: 1px solid rgba(255, 255, 255, 120);
        }
        QPushButton#dangerButton:hover {
            background: #ff6b61;
        }
        QPushButton#themeButton {
            min-width: 58px;
            min-height: 34px;
            max-height: 34px;
            background: transparent;
            color: %6;
            border-radius: 12px;
            padding: 0 12px;
        }
        QPushButton#themeButton:checked {
            background: transparent;
            color: #ffffff;
        }
        QPushButton#ambientButton {
            background: %11;
            color: %2;
            border: 1px solid %9;
            text-align: center;
            padding: 0 16px;
        }
        QPushButton#ambientButton:checked {
            background: %12;
            color: %2;
            border: 1px solid rgba(94, 203, 255, 190);
        }
        /* 輸入控件：番茄鐘 SpinBox 與待辦輸入框。 */
        QSpinBox, QLineEdit {
            background: %7;
            color: %2;
            border: 1px solid %9;
            border-radius: 14px;
            min-height: 42px;
            padding: 0 12px;
            selection-background-color: rgba(94, 203, 255, 210);
            selection-color: #ffffff;
        }
        QSpinBox {
            padding-right: 42px;
        }
        QSpinBox::up-button {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 34px;
            border-left: 1px solid %9;
            border-bottom: 1px solid %9;
            border-top-right-radius: 14px;
            background: %11;
        }
        QSpinBox::down-button {
            subcontrol-origin: border;
            subcontrol-position: bottom right;
            width: 34px;
            border-left: 1px solid %9;
            border-bottom-right-radius: 14px;
            background: %11;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background: %12;
        }
        QSpinBox::up-arrow {
            width: 10px;
            height: 10px;
        }
        QSpinBox::down-arrow {
            width: 10px;
            height: 10px;
        }
        /* 待辦列表：小窗模式隱藏捲軸，但保留滾輪操作。 */
        QListWidget {
            background: transparent;
            border: none;
            outline: none;
            color: %2;
            padding: 4px 10px 12px 0;
        }
        QListWidget::item {
            background: transparent;
            border: none;
            margin: 0;
        }
        QScrollBar:vertical {
            width: 7px;
            margin: 2px 0 2px 0;
            background: transparent;
            border: none;
        }
        QScrollBar::handle:vertical {
            min-height: 34px;
            border-radius: 3px;
            background: %9;
        }
        QScrollBar::handle:vertical:hover {
            background: rgba(94, 203, 255, 210);
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
            background: transparent;
            border: none;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
        }
        /* 待辦事件復選框；compact=true 時代表待辦小窗樣式。 */
        QCheckBox#todoCheck {
            background: %7;
            border: 1px solid %4;
            border-radius: 14px;
            padding: 10px 14px;
            margin-right: 14px;
            color: %2;
            spacing: 12px;
        }
        QCheckBox#todoCheck[compact="true"] {
            border-radius: 12px;
            padding: 3px 12px;
            margin-right: 12px;
            spacing: 10px;
        }
        QCheckBox#todoCheck[compact="true"]::indicator {
            width: 18px;
            height: 18px;
            border-radius: 9px;
        }
        QCheckBox#todoCheck:hover {
            background: %8;
        }
        QCheckBox#todoCheck::indicator {
            width: 20px;
            height: 20px;
            border-radius: 10px;
            border: 1px solid %9;
            background: transparent;
        }
        QCheckBox#todoCheck::indicator:checked {
            background: #34c759;
            border: 1px solid #34c759;
        }
        /* 環境音滑桿。 */
        QSlider::groove:horizontal {
            height: 7px;
            background: %9;
            border-radius: 4px;
        }
        QSlider::sub-page:horizontal {
            background: rgba(94, 203, 255, 210);
            border-radius: 4px;
        }
        QSlider::handle:horizontal {
            width: 20px;
            height: 20px;
            margin: -7px 0;
            border-radius: 10px;
            background: #ffffff;
            border: 1px solid %4;
        }
        /* 狀態概覽進度條。 */
        QProgressBar {
            background: %9;
            border: none;
            border-radius: 5px;
            height: 10px;
        }
        QProgressBar::chunk {
            background: rgba(94, 203, 255, 210);
            border-radius: 5px;
        }
    )").arg(rootBg)
        .arg(rootText)
        .arg(cardBg)
        .arg(cardBorder)
        .arg(miniBg)
        .arg(muted)
        .arg(controlBg)
        .arg(activeBg)
        .arg(controlBorder)
        .arg(bubbleBg)
        .arg(glassButtonBg)
        .arg(glassButtonHover));

    for (QWidget *widget : m_shadowWidgets) {
        // 主題切換後重新套陰影，讓夜晚模式陰影更重、白天模式更淡。
        applyShadow(widget);
    }
    moveTabBubble(m_featureStack ? m_featureStack->currentIndex() : 0, false);
    QTimer::singleShot(0, this, [this]() {
        moveThemeBubble(true);
    });
}

void MainWindow::applyShadow(QWidget *widget)
{
    // 卡片陰影渲染：配合 m_nightMode 調整模糊半徑、位移與透明度。
    auto *effect = new QGraphicsDropShadowEffect(widget);
    effect->setBlurRadius(m_nightMode ? 18.0 : 20.0);
    effect->setOffset(0, m_nightMode ? 8 : 7);
    effect->setColor(m_nightMode ? QColor(0, 0, 0, 110) : QColor(31, 35, 45, 28));
    widget->setGraphicsEffect(effect);
}

void MainWindow::installButtonAnimation(QPushButton *button)
{
    // 目前保留按鈕動畫入口；為了降低 CPU，預設只啟用 QSS 背景。
    button->setAttribute(Qt::WA_StyledBackground, true);
}

void MainWindow::animateButton(QPushButton *button, bool hovered, bool pressed)
{
    // 舊版按鈕陰影動畫，若日後想恢復所有按鈕動效可從這裡調整。
    auto *effect = qobject_cast<QGraphicsDropShadowEffect *>(button->graphicsEffect());
    if (!effect) {
        installButtonAnimation(button);
        effect = qobject_cast<QGraphicsDropShadowEffect *>(button->graphicsEffect());
    }
    if (!effect) {
        return;
    }

    const double blur = pressed ? 8.0 : (hovered ? 24.0 : 6.0);
    const QPointF offset(0, pressed ? 1.0 : (hovered ? 8.0 : 2.0));
    const QColor color = m_nightMode
        ? QColor(0, 0, 0, pressed ? 85 : (hovered ? 150 : 55))
        : QColor(31, 35, 45, pressed ? 36 : (hovered ? 70 : 28));

    const auto animations = effect->findChildren<QPropertyAnimation *>();
    for (QPropertyAnimation *animation : animations) {
        animation->stop();
        animation->deleteLater();
    }

    auto *blurAnimation = new QPropertyAnimation(effect, "blurRadius", effect);
    blurAnimation->setDuration(pressed ? 90 : 180);
    blurAnimation->setEasingCurve(QEasingCurve::OutCubic);
    blurAnimation->setStartValue(effect->blurRadius());
    blurAnimation->setEndValue(blur);
    blurAnimation->start(QAbstractAnimation::DeleteWhenStopped);

    auto *offsetAnimation = new QPropertyAnimation(effect, "offset", effect);
    offsetAnimation->setDuration(pressed ? 90 : 180);
    offsetAnimation->setEasingCurve(QEasingCurve::OutCubic);
    offsetAnimation->setStartValue(effect->offset());
    offsetAnimation->setEndValue(offset);
    offsetAnimation->start(QAbstractAnimation::DeleteWhenStopped);

    auto *colorAnimation = new QPropertyAnimation(effect, "color", effect);
    colorAnimation->setDuration(pressed ? 90 : 180);
    colorAnimation->setEasingCurve(QEasingCurve::OutCubic);
    colorAnimation->setStartValue(effect->color());
    colorAnimation->setEndValue(color);
    colorAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::animateButtonBubble(QPushButton *button, bool pressed)
{
    // 舊版按鈕氣泡動畫；目前主要保留給後續擴充。
    QWidget *bubble = button->findChild<QWidget *>("buttonBubble");
    if (!bubble) {
        return;
    }

    auto *opacity = qobject_cast<QGraphicsOpacityEffect *>(bubble->graphicsEffect());
    if (!opacity) {
        return;
    }

    const int inset = pressed ? 8 : 3;
    const QRect endRect = button->rect().adjusted(inset, inset, -inset, -inset);
    const QPoint center = button->rect().center();
    const int startSize = pressed ? 18 : 10;
    const QRect startRect(center.x() - startSize / 2, center.y() - startSize / 2, startSize, startSize);

    bubble->setProperty("resting", !pressed);
    bubble->show();
    bubble->raise();

    const auto geometryAnimations = bubble->findChildren<QPropertyAnimation *>();
    for (QPropertyAnimation *animation : geometryAnimations) {
        animation->stop();
        animation->deleteLater();
    }
    const auto opacityAnimations = opacity->findChildren<QPropertyAnimation *>();
    for (QPropertyAnimation *animation : opacityAnimations) {
        animation->stop();
        animation->deleteLater();
    }

    auto *geometryAnimation = new QPropertyAnimation(bubble, "geometry", bubble);
    geometryAnimation->setDuration(pressed ? 140 : 240);
    geometryAnimation->setEasingCurve(pressed ? QEasingCurve::OutCubic : QEasingCurve::OutBack);
    geometryAnimation->setStartValue(pressed ? startRect : bubble->geometry());
    geometryAnimation->setEndValue(endRect);
    geometryAnimation->start(QAbstractAnimation::DeleteWhenStopped);

    auto *opacityAnimation = new QPropertyAnimation(opacity, "opacity", opacity);
    opacityAnimation->setDuration(pressed ? 120 : 220);
    opacityAnimation->setEasingCurve(QEasingCurve::OutCubic);
    opacityAnimation->setStartValue(opacity->opacity());
    opacityAnimation->setEndValue(pressed ? 0.36 : 0.18);
    opacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::fadeButtonBubble(QPushButton *button)
{
    // 舊版按鈕氣泡淡出動畫。
    QWidget *bubble = button->findChild<QWidget *>("buttonBubble");
    if (!bubble) {
        return;
    }

    auto *opacity = qobject_cast<QGraphicsOpacityEffect *>(bubble->graphicsEffect());
    if (!opacity) {
        bubble->hide();
        return;
    }

    bubble->setProperty("resting", false);
    auto *opacityAnimation = new QPropertyAnimation(opacity, "opacity", opacity);
    opacityAnimation->setDuration(180);
    opacityAnimation->setEasingCurve(QEasingCurve::OutCubic);
    opacityAnimation->setStartValue(opacity->opacity());
    opacityAnimation->setEndValue(0.0);
    connect(opacityAnimation, &QPropertyAnimation::finished, bubble, [bubble]() {
        bubble->hide();
    });
    opacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::animateEntrance()
{
    // 啟動淡入動畫，只改變 windowOpacity，不影響布局。
    auto *fade = new QPropertyAnimation(this, "windowOpacity", this);
    fade->setDuration(360);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->setEasingCurve(QEasingCurve::OutCubic);
    fade->start(QAbstractAnimation::DeleteWhenStopped);

    for (int i = 0; i < m_animatedWidgets.size(); ++i) {
        QWidget *widget = m_animatedWidgets.at(i);
        const QPoint endPos = widget->pos();
        widget->move(endPos + QPoint(0, 18));

        auto *slide = new QPropertyAnimation(widget, "pos", widget);
        slide->setDuration(520);
        slide->setStartValue(widget->pos());
        slide->setEndValue(endPos);
        slide->setEasingCurve(QEasingCurve::OutCubic);
        QTimer::singleShot(i * 55, slide, [slide]() {
            slide->start(QAbstractAnimation::DeleteWhenStopped);
        });
    }
}

QString MainWindow::formatTime(int seconds) const
{
    const int minutes = seconds / 60;
    const int secs = seconds % 60;
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}
