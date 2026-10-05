#include "appstate.h"

#include "theme/theme.h"

namespace AppState {

Thresholds cpuThresholds() { return Thresholds{50.0, 74.0, 89.0}; }
Thresholds gpuThresholds() { return Thresholds{45.0, 69.0, 84.0}; }

Band classify(double celsius, const Thresholds &th)
{
    if (celsius < th.coolMax)
        return Band::Cool;
    if (celsius <= th.normalMax)
        return Band::Normal;
    if (celsius <= th.warmMax)
        return Band::Warm;
    return Band::High;
}

Band classifyHysteresis(double celsius, const Thresholds &th, Band previous)
{
    const Band raw = classify(celsius, th);

    // Rising (or same): accept immediately.
    if (raw >= previous)
        return raw;

    // Cooling down: require clearing the boundary that starts `previous` by the
    // hysteresis margin before we report a cooler band.
    double boundary = th.coolMax;
    switch (previous) {
    case Band::Normal: boundary = th.coolMax; break;
    case Band::Warm: boundary = th.normalMax; break;
    case Band::High: boundary = th.warmMax; break;
    case Band::Cool: return Band::Cool;
    }

    if (celsius <= boundary - kHysteresis)
        return raw;
    return previous;
}

Band worst(Band a, Band b)
{
    return static_cast<int>(a) >= static_cast<int>(b) ? a : b;
}

QString bandLabel(Band band)
{
    switch (band) {
    case Band::Cool: return QStringLiteral("Cool");
    case Band::Normal: return QStringLiteral("Normal");
    case Band::Warm: return QStringLiteral("Warm");
    case Band::High: return QStringLiteral("High");
    }
    return QString();
}

QColor bandColor(Band band)
{
    switch (band) {
    case Band::Cool: return Theme::cool();
    case Band::Normal: return Theme::success();
    case Band::Warm: return Theme::warning();
    case Band::High: return Theme::critical();
    }
    return Theme::textSecondary();
}

} // namespace AppState
