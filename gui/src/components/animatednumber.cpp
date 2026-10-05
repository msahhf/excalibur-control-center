#include "animatednumber.h"

#include <QPropertyAnimation>

namespace {

QString formatNumber(double value, int decimals, bool grouping)
{
    const bool negative = value < 0.0;
    const double abs = negative ? -value : value;
    QString s = QString::number(abs, 'f', decimals);

    if (grouping) {
        const int dot = s.indexOf(QLatin1Char('.'));
        const int end = (dot >= 0) ? dot : s.size();
        int count = 0;
        for (int i = end - 1; i > 0; --i) {
            if (++count % 3 == 0) {
                s.insert(i, QLatin1Char(','));
                if (dot >= 0)
                    ++count; // keep alignment simple for the integer group only
            }
        }
    }
    return (negative ? QStringLiteral("-") : QString()) + s;
}

} // namespace

AnimatedNumber::AnimatedNumber(QWidget *parent)
    : QLabel(parent)
{
    m_anim = new QPropertyAnimation(this, "animatedValue", this);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    m_anim->setDuration(m_duration);
    setText(m_invalid);
}

void AnimatedNumber::setDecimals(int decimals)
{
    m_decimals = decimals;
    refresh();
}

void AnimatedNumber::setGrouping(bool enabled)
{
    m_grouping = enabled;
    refresh();
}

void AnimatedNumber::setInvalidText(const QString &text)
{
    m_invalid = text;
    if (!m_valid)
        setText(m_invalid);
}

void AnimatedNumber::setDuration(int ms)
{
    m_duration = ms;
    m_anim->setDuration(ms);
}

void AnimatedNumber::setNumeric(double value, bool valid)
{
    if (!valid) {
        m_anim->stop();
        m_valid = false;
        m_hadValue = false;
        setText(m_invalid);
        return;
    }

    m_valid = true;

    if (!m_hadValue) {
        // First real value: no animation, show it immediately.
        m_hadValue = true;
        m_displayValue = value;
        m_target = value;
        refresh();
        return;
    }

    if (qFuzzyCompare(m_target, value))
        return; // nothing to do; avoid restarting the animation every second

    m_target = value;
    m_anim->stop();
    m_anim->setStartValue(m_displayValue);
    m_anim->setEndValue(value);
    m_anim->setDuration(m_duration);
    m_anim->start();
}

void AnimatedNumber::setAnimatedValue(double value)
{
    m_displayValue = value;
    refresh();
}

void AnimatedNumber::refresh()
{
    if (!m_valid) {
        setText(m_invalid);
        return;
    }
    setText(formatNumber(m_displayValue, m_decimals, m_grouping));
}
