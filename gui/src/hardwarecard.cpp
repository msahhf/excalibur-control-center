#include "hardwarecard.h"

#include "theme.h"
#include "thermalindicator.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>

namespace {

QLabel *makeLabel(const QString &text, int deltaPt, bool bold,
                  const QColor &color = QColor(), double tracking = 100.0)
{
    auto *l = new QLabel(text);
    QFont f = l->font();
    if (deltaPt != 0)
        f.setPointSize(f.pointSize() + deltaPt);
    f.setBold(bold);
    if (tracking != 100.0)
        f.setLetterSpacing(QFont::PercentageSpacing, tracking);
    l->setFont(f);
    if (color.isValid()) {
        QPalette p = l->palette();
        p.setColor(QPalette::WindowText, color);
        l->setPalette(p);
    }
    return l;
}

QString formatRpm(long rpm)
{
    // Locale-independent thousands grouping with a comma: 3581 -> "3,581".
    const QString digits = QString::number(rpm);
    QString grouped;
    int count = 0;
    for (int i = digits.size() - 1; i >= 0; --i) {
        grouped.prepend(digits.at(i));
        if (++count % 3 == 0 && i > 0)
            grouped.prepend(QLatin1Char(','));
    }
    return grouped + QStringLiteral(" RPM");
}

} // namespace

HardwareCard::HardwareCard(const QString &moduleLabel, const QString &deviceLabel,
                           const QString &tempCaption, const QString &fanCaption,
                           QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(26, 22, 26, 24);
    v->setSpacing(0);

    // Module kind: PROCESSOR / GRAPHICS
    auto *module = makeLabel(moduleLabel, -1, true, Theme::dim(this), 135.0);
    v->addWidget(module);
    v->addSpacing(6);

    // Device name: CPU / NVIDIA GPU
    m_device = makeLabel(deviceLabel, 4, true);
    v->addWidget(m_device);
    v->addSpacing(10);

    // Hero temperature + caption
    m_temp = makeLabel(QStringLiteral("—"), 30, true);
    m_tempCaption = makeLabel(tempCaption, -1, false, Theme::dim(this), 120.0);
    {
        auto *row = new QHBoxLayout;
        row->setSpacing(12);
        row->addWidget(m_temp, 0, Qt::AlignBottom);
        row->addWidget(m_tempCaption, 0, Qt::AlignBottom);
        row->addStretch(1);
        v->addLayout(row);
    }
    v->addSpacing(14);

    // Thermal scale
    m_indicator = new ThermalIndicator(this);
    v->addWidget(m_indicator);
    v->addSpacing(20);

    // Fan telemetry (prominent)
    m_fanCaption = makeLabel(fanCaption, -1, false, Theme::dim(this), 120.0);
    v->addWidget(m_fanCaption);
    v->addSpacing(4);

    m_fanDot = new QLabel;
    m_fanDot->setFixedSize(9, 9);
    m_fanValue = makeLabel(QStringLiteral("—"), 8, true);
    {
        auto *row = new QHBoxLayout;
        row->setSpacing(9);
        row->addWidget(m_fanDot, 0, Qt::AlignVCenter);
        row->addWidget(m_fanValue, 0, Qt::AlignVCenter);
        row->addStretch(1);
        v->addLayout(row);
    }

    setFan(0, false);
    setTemperature(0.0, false);
}

void HardwareCard::setTemperature(double celsius, bool valid)
{
    m_temp->setText(valid ? QStringLiteral("%1°").arg(qRound(celsius))
                          : QStringLiteral("—"));
    m_indicator->setValue(celsius, valid);
}

void HardwareCard::setFan(long rpm, bool valid)
{
    m_fanValue->setText(valid ? formatRpm(rpm) : QStringLiteral("—"));

    const QColor dot = valid ? QColor(Theme::Accent) : Theme::border(this);
    m_fanDot->setStyleSheet(QStringLiteral("background-color:%1; border-radius:4px;")
                                .arg(dot.name()));
}

void HardwareCard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    QColor bg = Theme::surface(this);
    QColor border = Theme::border(this);
    border.setAlpha(170);

    p.setPen(QPen(border, 1));
    p.setBrush(bg);
    p.drawRoundedRect(r, 12, 12);
}
