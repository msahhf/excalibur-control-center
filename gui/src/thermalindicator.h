#pragma once

#include <QWidget>

// A restrained horizontal thermal scale: low-contrast track, subtle tick marks
// and a small current-position indicator. Color shifts to a semantic accent only
// when the temperature is genuinely high. Theme-aware (palette based).
class ThermalIndicator : public QWidget
{
    Q_OBJECT

public:
    explicit ThermalIndicator(QWidget *parent = nullptr);

    void setValue(double celsius, bool valid);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *) override;

private:
    double m_celsius = 0.0;
    bool m_valid = false;
    int m_min = 0;   // scale minimum (°C)
    int m_max = 100; // scale maximum (°C)
};
