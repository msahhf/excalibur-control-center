#pragma once

#include <QWidget>
#include <QColor>
#include <QString>

// A small status pill: colored dot + label on a subtle surface. Used for the
// hero cooling line and connection state. Tone maps to a semantic color.
class StatusPill : public QWidget
{
    Q_OBJECT

public:
    enum class Tone { Neutral, Success, Warning, Critical, Info };

    explicit StatusPill(QWidget *parent = nullptr);

    void setText(const QString &text);
    void setTone(Tone tone);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QColor toneColor() const;

    QString m_text;
    Tone m_tone = Tone::Neutral;
};
