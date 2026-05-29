#pragma once

#include <QColor>
#include <QString>
#include <QWidget>

class FocusRing : public QWidget
{
public:
    explicit FocusRing(QWidget *parent = nullptr);

    double progress() const;
    void setProgress(double value);
    void setGlow(double value);
    void setNightMode(bool nightMode);
    void setText(const QString &timeText, const QString &stateText);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // 番茄鐘圓環狀態：進度、光暈、主題與中央文字都由 MainWindow 更新。
    double m_progress = 1.0;
    double m_glow = 0.0;
    bool m_nightMode = false;
    QString m_timeText = "25:00";
    QString m_stateText = "专注模式";
};
