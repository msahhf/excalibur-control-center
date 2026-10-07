#pragma once

#include <QString>

// Presentation-layer unit conversion. The telemetry snapshot ALWAYS stores the
// real backend value (Celsius); widgets convert only for display through these
// helpers, so there is exactly one place that knows the conversion.
namespace Units {

enum class TemperatureUnit { Celsius, Fahrenheit };

inline double fromCelsius(double celsius, TemperatureUnit unit)
{
    return unit == TemperatureUnit::Fahrenheit ? celsius * 9.0 / 5.0 + 32.0 : celsius;
}

inline bool isFahrenheit(TemperatureUnit unit)
{
    return unit == TemperatureUnit::Fahrenheit;
}

// Fractions used by the "don't exaggerate a flat line" sparkline span.
inline double spanFactor(TemperatureUnit unit)
{
    return isFahrenheit(unit) ? 9.0 / 5.0 : 1.0;
}

inline int decimals(TemperatureUnit unit)
{
    return isFahrenheit(unit) ? 1 : 0;
}

inline QString symbol(TemperatureUnit unit)
{
    return isFahrenheit(unit) ? QStringLiteral("\u00B0F") : QStringLiteral("\u00B0C");
}

// "51 °C" / "123.8 °F"
inline QString format(double celsius, TemperatureUnit unit)
{
    return QStringLiteral("%1 %2")
        .arg(QString::number(fromCelsius(celsius, unit), 'f', decimals(unit)), symbol(unit));
}

} // namespace Units
