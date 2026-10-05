#include "fanstatus.h"

#include "animatednumber.h"
#include "theme/theme.h"

#include <QHBoxLayout>
#include <QLabel>

FanStatus::FanStatus(QWidget *parent)
    : QWidget(parent)
{
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(Theme::Space::S);

    m_caption = new QLabel(QStringLiteral("Fan"));
    m_caption->setFont(Theme::font(11, QFont::Medium));
    {
        QPalette p = m_caption->palette();
        p.setColor(QPalette::WindowText, Theme::textSecondary());
        m_caption->setPalette(p);
    }

    m_rpm = new AnimatedNumber;
    m_rpm->setFont(Theme::font(15, QFont::DemiBold));
    m_rpm->setGrouping(true);
    m_rpm->setInvalidText(QStringLiteral("\u2014"));
    {
        QPalette p = m_rpm->palette();
        p.setColor(QPalette::WindowText, Theme::textPrimary());
        m_rpm->setPalette(p);
    }

    auto *unit = new QLabel(QStringLiteral("RPM"));
    unit->setFont(Theme::labelFont(9, QFont::Medium, 130));
    {
        QPalette p = unit->palette();
        p.setColor(QPalette::WindowText, Theme::textMuted());
        unit->setPalette(p);
    }
    m_unit = unit;

    row->addWidget(m_caption, 0, Qt::AlignVCenter);
    row->addSpacing(Theme::Space::S);
    row->addWidget(m_rpm, 0, Qt::AlignVCenter);
    row->addWidget(unit, 0, Qt::AlignVCenter);
    row->addStretch(1);
}

void FanStatus::setCaption(const QString &caption)
{
    m_caption->setText(caption);
}

void FanStatus::setFan(int rpm, bool valid)
{
    m_rpm->setNumeric(rpm, valid);
    m_unit->setVisible(valid);
}
