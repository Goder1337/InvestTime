#include "FocusRing.h"

#include <QBrush>
#include <QColor>
#include <QConicalGradient>
#include <QFont>
#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <QRadialGradient>
#include <QtGlobal>

FocusRing::FocusRing(QWidget *parent) : QWidget(parent)
{
    // 番茄鐘圓環的基準尺寸；主視窗縮放時 Qt 會依版面自動調整。
    setMinimumSize(280, 280);
}

double FocusRing::progress() const
{
    return m_progress;
}

void FocusRing::setProgress(double value)
{
    // 進度值固定在 0~1，避免倒數動畫超出圓環範圍。
    m_progress = qBound(0.0, value, 1.0);
    update();
}

void FocusRing::setGlow(double value)
{
    // 光暈強度同樣限制在 0~1，數值越高外圈越亮。
    m_glow = qBound(0.0, value, 1.0);
    update();
}

void FocusRing::setNightMode(bool nightMode)
{
    m_nightMode = nightMode;
    update();
}

void FocusRing::setText(const QString &timeText, const QString &stateText)
{
    m_timeText = timeText;
    m_stateText = stateText;
    update();
}

void FocusRing::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 計算圓環位置：outer 是整體玻璃圓面，ringBounds 是進度弧線所在區域。
    const int side = qMin(width(), height());
    const QRectF outer((width() - side) / 2.0 + 10, (height() - side) / 2.0 + 10,
                       side - 20, side - 20);
    const QRectF ringBounds = outer.adjusted(22, 22, -22, -22);

    // 中央面板漸層，白天較亮、夜晚較深。
    const QColor panelStart = m_nightMode ? QColor("#26262a") : QColor("#ffffff");
    const QColor panelEnd = m_nightMode ? QColor("#17171a") : QColor("#eef4fb");
    QLinearGradient panelGradient(outer.topLeft(), outer.bottomRight());
    panelGradient.setColorAt(0.0, panelStart);
    panelGradient.setColorAt(1.0, panelEnd);

    painter.setPen(Qt::NoPen);
    painter.setBrush(panelGradient);
    painter.drawEllipse(outer.adjusted(14, 14, -14, -14));

    // 外圈光暈，用 m_glow 控制透明度，保留低 CPU 的靜態渲染。
    QRadialGradient glow(outer.center(), outer.width() / 2.0);
    const QColor glowColor = m_nightMode ? QColor(126, 212, 255) : QColor(0, 122, 255);
    glow.setColorAt(0.55, QColor(glowColor.red(), glowColor.green(), glowColor.blue(),
                                  static_cast<int>(22 + m_glow * 38)));
    glow.setColorAt(1.0, QColor(glowColor.red(), glowColor.green(), glowColor.blue(), 0));
    painter.setBrush(glow);
    painter.drawEllipse(outer);

    // 進度條底軌，永遠畫滿一圈。
    const QColor trackColor = m_nightMode ? QColor("#3a3a3c") : QColor("#dde5ef");
    painter.setPen(QPen(trackColor, 15, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(ringBounds, 0, 360 * 16);

    // 實際進度弧線，從頂部開始逆時針收縮。
    QConicalGradient progressGradient(ringBounds.center(), -90);
    progressGradient.setColorAt(0.00, m_nightMode ? QColor("#7dd3fc") : QColor("#007aff"));
    progressGradient.setColorAt(0.48, QColor("#34c759"));
    progressGradient.setColorAt(1.00, m_nightMode ? QColor("#c4b5fd") : QColor("#af52de"));

    painter.setPen(QPen(QBrush(progressGradient), 15, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(ringBounds, 90 * 16, static_cast<int>(-360 * 16 * m_progress));

    // 中央倒數文字。
    painter.setPen(m_nightMode ? QColor("#f5f5f7") : QColor("#1d1d1f"));
    QFont timeFont = painter.font();
    timeFont.setPixelSize(46);
    timeFont.setWeight(QFont::DemiBold);
    painter.setFont(timeFont);
    painter.drawText(ringBounds, Qt::AlignCenter, m_timeText);

    // 下方狀態文字，例如「專注模式」。
    QRectF subBounds = ringBounds.adjusted(0, ringBounds.height() * 0.58, 0, 0);
    painter.setPen(m_nightMode ? QColor("#a1a1aa") : QColor("#6e6e73"));
    QFont stateFont = painter.font();
    stateFont.setPixelSize(14);
    stateFont.setWeight(QFont::Medium);
    painter.setFont(stateFont);
    painter.drawText(subBounds, Qt::AlignHCenter | Qt::AlignTop, m_stateText);
}
