#pragma once

#include <QVector>
#include <QWidget>

#include "app/appstate.h"
#include "components/sparkline.h"
#include "util/units.h"

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

    void setIdentity(const QString &identity);
    // Presentation unit only; values passed to setTemperature/setTemperatureRange
    // are already in this unit. The snapshot always stays Celsius.
    void setTemperatureUnit(Units::TemperatureUnit unit);
    void setTemperature(double value, bool valid, AppState::Band band);
    void setTemperatureHistory(const QVector<SparkPoint> &points);
    // Secondary 60 s range of the shown temperature (from real history, already in
    // the display unit). Hidden when there is not enough valid data.
    void setTemperatureRange(bool valid, double minValue, double maxValue);
    void setFan(int rpm, bool valid);

private:
    class ThermalBar;

    QLabel *m_identity = nullptr;
    AnimatedNumber *m_temp = nullptr;
    QLabel *m_unit = nullptr;
    QLabel *m_band = nullptr;
    ThermalBar *m_bar = nullptr;
    Sparkline *m_spark = nullptr;
    QLabel *m_range = nullptr;
    FanStatus *m_fan = nullptr;

    Units::TemperatureUnit m_tempUnit = Units::TemperatureUnit::Celsius;
    bool m_rangeValid = false;
    double m_rMin = 0.0;
    double m_rMax = 0.0;
};
