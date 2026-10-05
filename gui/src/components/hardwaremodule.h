#pragma once

#include <QVector>
#include <QWidget>

#include "app/appstate.h"
#include "components/sparkline.h"

class AnimatedNumber;
class FanStatus;
class QLabel;

// One hardware column (CPU or GPU) inside a ThermalPanel. The temperature is the
// visually dominant element; a contextual band label, a thin band-colored bar and
// a compact 60 s trend line give the number meaning. Missing values show an
// em dash. Draws no background of its own (the panel owns the surface).
class HardwareModule : public QWidget
{
    Q_OBJECT

public:
    explicit HardwareModule(const QString &identity, QWidget *parent = nullptr);

    void setTemperature(double celsius, bool valid, AppState::Band band);
    void setTemperatureHistory(const QVector<SparkPoint> &points);
    void setFan(int rpm, bool valid);
    void setFanCaption(const QString &caption);

private:
    class ThermalBar;

    QLabel *m_identity = nullptr;
    AnimatedNumber *m_temp = nullptr;
    QLabel *m_unit = nullptr;
    QLabel *m_band = nullptr;
    ThermalBar *m_bar = nullptr;
    Sparkline *m_spark = nullptr;
    FanStatus *m_fan = nullptr;
};
