#pragma once

#include <QFrame>

class QLabel;
class ThermalIndicator;

// A hardware "module" panel: module kind, device name, hero temperature with a
// thermal indicator, and prominent fan telemetry. Paints its own theme-aware
// surface/border (no stylesheet), so it adapts to dark/light palettes.
class HardwareCard : public QFrame
{
    Q_OBJECT

public:
    HardwareCard(const QString &moduleLabel, const QString &deviceLabel,
                 const QString &tempCaption, const QString &fanCaption,
                 QWidget *parent = nullptr);

    void setTemperature(double celsius, bool valid);
    void setFan(long rpm, bool valid);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QLabel *m_device = nullptr;
    QLabel *m_temp = nullptr;
    QLabel *m_tempCaption = nullptr;
    ThermalIndicator *m_indicator = nullptr;
    QLabel *m_fanCaption = nullptr;
    QLabel *m_fanValue = nullptr;
    QLabel *m_fanDot = nullptr;
};
