#pragma once

#include <QLabel>

class QPropertyAnimation;

// A numeric label that interpolates smoothly toward a new target instead of
// jumping every second (Section 12). A value that is unavailable is shown as an
// em dash, never as 0.
class AnimatedNumber : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(double animatedValue READ animatedValue WRITE setAnimatedValue)

public:
    explicit AnimatedNumber(QWidget *parent = nullptr);

    void setDecimals(int decimals);
    void setGrouping(bool enabled); // thousands separators
    void setInvalidText(const QString &text);
    void setDuration(int ms);

    // Update the displayed value. `valid == false` shows the invalid text.
    void setNumeric(double value, bool valid);

    double animatedValue() const { return m_displayValue; }
    void setAnimatedValue(double value);

private:
    void refresh();

    QPropertyAnimation *m_anim = nullptr;
    double m_displayValue = 0.0;
    double m_target = 0.0;
    bool m_hadValue = false;
    bool m_valid = false;
    int m_decimals = 0;
    bool m_grouping = false;
    int m_duration = 450;
    QString m_invalid = QStringLiteral("\u2014"); // em dash
};
