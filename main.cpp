#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QStyleOption>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class FocusRing : public QWidget
{
public:
    explicit FocusRing(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumSize(260, 260);
    }

    void setProgress(double value)
    {
        m_progress = qBound(0.0, value, 1.0);
        update();
    }

    void setText(const QString &timeText, const QString &stateText)
    {
        m_timeText = timeText;
        m_stateText = stateText;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const int side = qMin(width(), height());
        const QRectF bounds((width() - side) / 2.0 + 16, (height() - side) / 2.0 + 16,
                            side - 32, side - 32);

        QPen basePen(QColor("#243044"), 16, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(basePen);
        painter.drawArc(bounds, 0, 360 * 16);

        QConicalGradient gradient(bounds.center(), -90);
        gradient.setColorAt(0.00, QColor("#7dd3fc"));
        gradient.setColorAt(0.45, QColor("#a7f3d0"));
        gradient.setColorAt(1.00, QColor("#c4b5fd"));

        QPen progressPen(QBrush(gradient), 16, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(progressPen);
        painter.drawArc(bounds, 90 * 16, static_cast<int>(-360 * 16 * m_progress));

        painter.setPen(QColor("#eef6ff"));
        QFont timeFont = painter.font();
        timeFont.setPixelSize(44);
        timeFont.setWeight(QFont::DemiBold);
        painter.setFont(timeFont);
        painter.drawText(bounds, Qt::AlignCenter, m_timeText);

        QRectF subBounds = bounds.adjusted(0, bounds.height() * 0.58, 0, 0);
        painter.setPen(QColor("#8ea0b8"));
        QFont stateFont = painter.font();
        stateFont.setPixelSize(14);
        stateFont.setWeight(QFont::Medium);
        painter.setFont(stateFont);
        painter.drawText(subBounds, Qt::AlignHCenter | Qt::AlignTop, m_stateText);
    }

private:
    double m_progress = 1.0;
    QString m_timeText = "25:00";
    QString m_stateText = "专注模式";
};

class MainWindow : public QWidget
{
public:
    explicit MainWindow(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("StudyWithMe");
        resize(1180, 760);
        setObjectName("root");

        m_timer.setInterval(1000);
        connect(&m_timer, &QTimer::timeout, this, &MainWindow::tick);

        buildUi();
        resetTimer();
    }

private:
    QFrame *card(const QString &title, const QString &caption)
    {
        auto *frame = new QFrame;
        frame->setObjectName("card");
        auto *layout = new QVBoxLayout(frame);
        layout->setContentsMargins(24, 22, 24, 24);
        layout->setSpacing(16);

        auto *titleLabel = new QLabel(title);
        titleLabel->setObjectName("cardTitle");
        auto *captionLabel = new QLabel(caption);
        captionLabel->setObjectName("muted");
        captionLabel->setWordWrap(true);

        layout->addWidget(titleLabel);
        layout->addWidget(captionLabel);
        return frame;
    }

    QPushButton *toolButton(const QString &text, const QString &objectName = "ghostButton")
    {
        auto *button = new QPushButton(text);
        button->setCursor(Qt::PointingHandCursor);
        button->setObjectName(objectName);
        button->setMinimumHeight(42);
        return button;
    }

    void buildUi()
    {
        auto *shell = new QHBoxLayout(this);
        shell->setContentsMargins(24, 24, 24, 24);
        shell->setSpacing(18);

        auto *sidebar = new QFrame;
        sidebar->setObjectName("sidebar");
        sidebar->setFixedWidth(230);
        auto *sideLayout = new QVBoxLayout(sidebar);
        sideLayout->setContentsMargins(22, 24, 22, 24);
        sideLayout->setSpacing(18);

        auto *brand = new QLabel("StudyWithMe");
        brand->setObjectName("brand");
        auto *date = new QLabel(QDate::currentDate().toString("yyyy.MM.dd  dddd"));
        date->setObjectName("muted");

        sideLayout->addWidget(brand);
        sideLayout->addWidget(date);
        sideLayout->addSpacing(16);

        const QStringList navItems = {"专注计时", "待办清单", "环境氛围", "统计概览"};
        for (const QString &item : navItems) {
            auto *label = new QLabel(item);
            label->setObjectName(item == "专注计时" ? "navActive" : "navItem");
            label->setMinimumHeight(42);
            label->setAlignment(Qt::AlignVCenter);
            sideLayout->addWidget(label);
        }
        sideLayout->addStretch();

        auto *statusCard = new QFrame;
        statusCard->setObjectName("miniCard");
        auto *statusLayout = new QVBoxLayout(statusCard);
        statusLayout->setContentsMargins(16, 16, 16, 16);
        statusLayout->setSpacing(8);
        auto *statusTitle = new QLabel("今日节奏");
        statusTitle->setObjectName("miniTitle");
        auto *statusText = new QLabel("2 个专注块 / 4 项任务");
        statusText->setObjectName("muted");
        statusLayout->addWidget(statusTitle);
        statusLayout->addWidget(statusText);
        sideLayout->addWidget(statusCard);

        shell->addWidget(sidebar);

        auto *content = new QVBoxLayout;
        content->setSpacing(18);
        shell->addLayout(content, 1);

        auto *header = new QHBoxLayout;
        auto *headlineBox = new QVBoxLayout;
        auto *eyebrow = new QLabel("FOCUS DASHBOARD");
        eyebrow->setObjectName("eyebrow");
        auto *headline = new QLabel("安静、高效、可控的学习空间");
        headline->setObjectName("headline");
        headlineBox->addWidget(eyebrow);
        headlineBox->addWidget(headline);
        header->addLayout(headlineBox);
        header->addStretch();

        auto *profile = toolButton("深度学习中", "pillButton");
        header->addWidget(profile);
        content->addLayout(header);

        auto *grid = new QGridLayout;
        grid->setSpacing(18);
        content->addLayout(grid, 1);

        buildTimerCard(grid);
        buildTodoCard(grid);
        buildAmbienceCard(grid);
        buildOverviewCard(grid);

        setStyleSheet(R"(
            QWidget#root {
                background: #0b1020;
                color: #edf4ff;
                font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
                font-size: 14px;
            }
            QFrame#sidebar, QFrame#card {
                background: rgba(17, 25, 44, 0.96);
                border: 1px solid #243044;
                border-radius: 18px;
            }
            QFrame#miniCard {
                background: #121c31;
                border: 1px solid #2c3a53;
                border-radius: 14px;
            }
            QLabel#brand {
                font-size: 24px;
                font-weight: 700;
                color: #ffffff;
            }
            QLabel#headline {
                font-size: 30px;
                font-weight: 700;
                color: #ffffff;
            }
            QLabel#eyebrow {
                color: #7dd3fc;
                font-size: 12px;
                font-weight: 700;
                letter-spacing: 0px;
            }
            QLabel#cardTitle {
                font-size: 20px;
                font-weight: 700;
                color: #f8fbff;
            }
            QLabel#miniTitle {
                font-size: 15px;
                font-weight: 700;
                color: #f8fbff;
            }
            QLabel#muted {
                color: #8ea0b8;
                line-height: 140%;
            }
            QLabel#navItem, QLabel#navActive {
                padding-left: 14px;
                border-radius: 12px;
                color: #9aacbf;
            }
            QLabel#navActive {
                background: #1d2a42;
                color: #ffffff;
                font-weight: 700;
                border: 1px solid #2e405f;
            }
            QPushButton {
                border: none;
                border-radius: 12px;
                padding: 0 18px;
                font-weight: 700;
            }
            QPushButton#primaryButton {
                background: #7dd3fc;
                color: #07111f;
            }
            QPushButton#primaryButton:hover {
                background: #a7e3ff;
            }
            QPushButton#ghostButton, QPushButton#pillButton {
                background: #1b263b;
                color: #d8e5f4;
                border: 1px solid #2c3a53;
            }
            QPushButton#ghostButton:hover, QPushButton#pillButton:hover {
                background: #24324b;
            }
            QPushButton#ambientButton {
                background: #121c31;
                color: #cfe0f3;
                border: 1px solid #2c3a53;
                text-align: left;
                padding-left: 18px;
            }
            QPushButton#ambientButton:checked {
                background: #203957;
                color: #ffffff;
                border: 1px solid #7dd3fc;
            }
            QSpinBox, QLineEdit, QComboBox {
                background: #0f1729;
                color: #eef6ff;
                border: 1px solid #2c3a53;
                border-radius: 10px;
                min-height: 40px;
                padding: 0 12px;
                selection-background-color: #7dd3fc;
                selection-color: #07111f;
            }
            QListWidget {
                background: transparent;
                border: none;
                outline: none;
                color: #eef6ff;
            }
            QListWidget::item {
                background: #101a2d;
                border: 1px solid #243044;
                border-radius: 10px;
                margin: 5px 0;
                padding: 10px;
            }
            QSlider::groove:horizontal {
                height: 8px;
                background: #243044;
                border-radius: 4px;
            }
            QSlider::sub-page:horizontal {
                background: #a7f3d0;
                border-radius: 4px;
            }
            QSlider::handle:horizontal {
                width: 18px;
                height: 18px;
                margin: -5px 0;
                border-radius: 9px;
                background: #ffffff;
            }
            QProgressBar {
                background: #243044;
                border: none;
                border-radius: 5px;
                height: 10px;
                text-align: center;
            }
            QProgressBar::chunk {
                background: #a7f3d0;
                border-radius: 5px;
            }
        )");
    }

    void buildTimerCard(QGridLayout *grid)
    {
        auto *frame = card("番茄钟", "设置你的专注时长，开始后界面会显示实时倒计时和进度环。");
        grid->addWidget(frame, 0, 0, 2, 1);
        auto *layout = qobject_cast<QVBoxLayout *>(frame->layout());

        m_ring = new FocusRing;
        layout->addWidget(m_ring, 1, Qt::AlignCenter);

        auto *settings = new QHBoxLayout;
        settings->setSpacing(12);
        m_focusSpin = new QSpinBox;
        m_focusSpin->setRange(1, 180);
        m_focusSpin->setValue(25);
        m_focusSpin->setSuffix(" 分钟专注");

        m_breakSpin = new QSpinBox;
        m_breakSpin->setRange(1, 60);
        m_breakSpin->setValue(5);
        m_breakSpin->setSuffix(" 分钟休息");
        settings->addWidget(m_focusSpin);
        settings->addWidget(m_breakSpin);
        layout->addLayout(settings);

        auto *actions = new QHBoxLayout;
        actions->setSpacing(12);
        m_startButton = toolButton("开始", "primaryButton");
        auto *reset = toolButton("重置");
        actions->addWidget(m_startButton, 1);
        actions->addWidget(reset);
        layout->addLayout(actions);

        connect(m_startButton, &QPushButton::clicked, this, &MainWindow::toggleTimer);
        connect(reset, &QPushButton::clicked, this, &MainWindow::resetTimer);
        connect(m_focusSpin, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::resetTimer);
    }

    void buildTodoCard(QGridLayout *grid)
    {
        auto *frame = card("待办事项", "把今天要完成的学习任务放进清单，完成后可直接勾选。");
        grid->addWidget(frame, 0, 1);
        auto *layout = qobject_cast<QVBoxLayout *>(frame->layout());

        auto *inputRow = new QHBoxLayout;
        m_todoInput = new QLineEdit;
        m_todoInput->setPlaceholderText("添加一个任务，例如：复习线性代数第三章");
        auto *add = toolButton("添加", "primaryButton");
        inputRow->addWidget(m_todoInput, 1);
        inputRow->addWidget(add);
        layout->addLayout(inputRow);

        m_todoList = new QListWidget;
        m_todoList->setMinimumHeight(210);
        layout->addWidget(m_todoList, 1);

        auto *clean = toolButton("清除已完成");
        layout->addWidget(clean);

        addTodo("完成一组英语听力训练");
        addTodo("整理今天的错题");
        addTodo("阅读论文摘要并记录关键词");

        connect(add, &QPushButton::clicked, this, &MainWindow::addTodoFromInput);
        connect(m_todoInput, &QLineEdit::returnPressed, this, &MainWindow::addTodoFromInput);
        connect(clean, &QPushButton::clicked, this, &MainWindow::removeCompletedTodos);
    }

    void buildAmbienceCard(QGridLayout *grid)
    {
        auto *frame = card("环境氛围", "选择适合当前学习阶段的背景氛围，并调整音量与沉浸强度。");
        grid->addWidget(frame, 1, 1);
        auto *layout = qobject_cast<QVBoxLayout *>(frame->layout());

        auto *modes = new QGridLayout;
        modes->setSpacing(10);
        auto *group = new QButtonGroup(this);
        group->setExclusive(true);

        const QStringList ambienceItems = {"雨夜书桌", "咖啡馆", "白噪音", "森林晨光"};
        for (int i = 0; i < ambienceItems.size(); ++i) {
            auto *button = toolButton(ambienceItems.at(i), "ambientButton");
            button->setCheckable(true);
            if (i == 0) {
                button->setChecked(true);
            }
            group->addButton(button, i);
            modes->addWidget(button, i / 2, i % 2);
        }
        layout->addLayout(modes);

        auto *volumeLabel = new QLabel("氛围音量");
        volumeLabel->setObjectName("muted");
        auto *volume = new QSlider(Qt::Horizontal);
        volume->setRange(0, 100);
        volume->setValue(64);

        auto *depthLabel = new QLabel("沉浸强度");
        depthLabel->setObjectName("muted");
        auto *depth = new QSlider(Qt::Horizontal);
        depth->setRange(0, 100);
        depth->setValue(72);

        layout->addWidget(volumeLabel);
        layout->addWidget(volume);
        layout->addWidget(depthLabel);
        layout->addWidget(depth);
    }

    void buildOverviewCard(QGridLayout *grid)
    {
        auto *frame = card("状态概览", "保持轻量反馈，专注于下一步行动。");
        grid->addWidget(frame, 2, 0, 1, 2);
        auto *layout = qobject_cast<QVBoxLayout *>(frame->layout());

        auto *row = new QHBoxLayout;
        row->setSpacing(16);
        layout->addLayout(row);

        addMetric(row, "今日专注", "50 分钟", 50);
        addMetric(row, "任务完成", "1 / 4", 25);
        addMetric(row, "连续学习", "7 天", 76);
    }

    void addMetric(QHBoxLayout *row, const QString &name, const QString &value, int progress)
    {
        auto *box = new QFrame;
        box->setObjectName("miniCard");
        auto *layout = new QVBoxLayout(box);
        layout->setContentsMargins(16, 14, 16, 14);
        layout->setSpacing(8);

        auto *label = new QLabel(name);
        label->setObjectName("muted");
        auto *number = new QLabel(value);
        number->setObjectName("miniTitle");
        auto *bar = new QProgressBar;
        bar->setRange(0, 100);
        bar->setValue(progress);
        bar->setTextVisible(false);

        layout->addWidget(label);
        layout->addWidget(number);
        layout->addWidget(bar);
        row->addWidget(box, 1);
    }

    void addTodo(const QString &text)
    {
        if (text.trimmed().isEmpty()) {
            return;
        }

        auto *item = new QListWidgetItem(text.trimmed(), m_todoList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }

    void addTodoFromInput()
    {
        addTodo(m_todoInput->text());
        m_todoInput->clear();
    }

    void removeCompletedTodos()
    {
        for (int i = m_todoList->count() - 1; i >= 0; --i) {
            QListWidgetItem *item = m_todoList->item(i);
            if (item->checkState() == Qt::Checked) {
                delete m_todoList->takeItem(i);
            }
        }
    }

    void toggleTimer()
    {
        if (m_timer.isActive()) {
            m_timer.stop();
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

    void resetTimer()
    {
        m_timer.stop();
        m_totalSeconds = m_focusSpin ? m_focusSpin->value() * 60 : 25 * 60;
        m_remainingSeconds = m_totalSeconds;
        m_startButton->setText("开始");
        updateRing();
    }

    void tick()
    {
        if (m_remainingSeconds > 0) {
            --m_remainingSeconds;
        }

        updateRing();

        if (m_remainingSeconds <= 0) {
            m_timer.stop();
            m_startButton->setText("再来一轮");
            m_ring->setText("00:00", "完成，休息一下");
        }
    }

    void updateRing()
    {
        const double progress = m_totalSeconds == 0
            ? 0.0
            : static_cast<double>(m_remainingSeconds) / static_cast<double>(m_totalSeconds);
        m_ring->setProgress(progress);
        m_ring->setText(formatTime(m_remainingSeconds), "专注模式");
    }

    QString formatTime(int seconds) const
    {
        const int minutes = seconds / 60;
        const int secs = seconds % 60;
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }

    QTimer m_timer;
    FocusRing *m_ring = nullptr;
    QSpinBox *m_focusSpin = nullptr;
    QSpinBox *m_breakSpin = nullptr;
    QPushButton *m_startButton = nullptr;
    QLineEdit *m_todoInput = nullptr;
    QListWidget *m_todoList = nullptr;
    int m_totalSeconds = 25 * 60;
    int m_remainingSeconds = 25 * 60;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
