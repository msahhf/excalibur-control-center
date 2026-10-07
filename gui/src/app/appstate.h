#pragma once

#include <QColor>
#include <QString>

// Interpretation layer. Turns raw telemetry into user-facing meaning. This sits
// ABOVE the hardware layer: widgets never read sysfs and never classify values
// themselves.
//
// Thresholds are generic, documented defaults (Section 6.1) and are NOT claimed
// to be Casper's official limits. They live here, in one place, so they are easy
// to change.
namespace AppState {

// Internal connection state (kept separate from user-facing language, Section 11).
enum class Status { Connected, Degraded, Disconnected };

// Thermal bands, ordered cool -> high.
enum class Band { Cool = 0, Normal = 1, Warm = 2, High = 3 };

struct Thresholds {
    double coolMax;   // below this -> Cool
    double normalMax; // <= this (and >= coolMax) -> Normal
    double warmMax;   // <= this -> Warm; above -> High
};

// Suggested defaults (Section 6.1):
//   CPU  Cool <50      Normal 50-74   Warm 75-89    High >=90
//   GPU  Cool <45      Normal 45-69   Warm 70-84    High >=85
Thresholds cpuThresholds();
Thresholds gpuThresholds();

// Hysteresis (Section 6.1), in degrees Celsius. Rising across a boundary changes
// the band immediately; falling must clear the boundary by this margin so the
// label does not flicker around a threshold.
constexpr double kHysteresis = 2.0;

// Plain classification (no hysteresis).
Band classify(double celsius, const Thresholds &th);

// Hysteresis-aware classification. `previous` is the last reported band for the
// same sensor. Use classify() for the first reading, then this afterwards.
Band classifyHysteresis(double celsius, const Thresholds &th, Band previous);

// Hottest of two bands (system state = worst of CPU/GPU).
Band worst(Band a, Band b);

QString bandLabel(Band band); // "Cool" / "Normal" / "Warm" / "High"
QColor bandColor(Band band);

// --- Cooling state -------------------------------------------------------
// We do NOT have a programmatic fan/OEM-performance mode. This is an honest,
// conservative read derived only from what the hwmon backend actually reports:
//   - no usable fan reading            -> Unavailable
//   - fans present but not turning     -> Idle
//   - fans turning while load is warm+ -> Elevated
//   - fans turning, load normal/cool   -> Active
// It deliberately never claims a specific OEM mode ("Turbo"/"Boost"/"Gaming").
enum class CoolingState { Unavailable, Idle, Active, Elevated };

// fanDataValid: at least one fan RPM reading was readable.
// fanMoving:    at least one valid fan RPM is above zero.
// band:         the current system thermal band (worst of CPU/GPU).
CoolingState coolingState(bool fanDataValid, bool fanMoving, Band band);

QString coolingStateLabel(CoolingState state); // "Cooling Active", etc.

// Sanitises a backend hwmon label for UI use: trims whitespace, falls back when
// empty, and caps the length so an unexpected driver label cannot break a compact
// layout. The semantic fallback ("CPU"/"GPU"/"CPU Fan"/"GPU Fan") is what the
// product shows when the backend provides nothing.
QString displaySensorLabel(const QString &raw, const QString &fallback, int maxChars = 14);

// --- BandTracker ---------------------------------------------------------
// Hysteresis-aware per-sensor band tracking across polls. Both Dashboard and
// Cooling need the same CPU/GPU classification, so the logic lives in one place
// instead of being duplicated per page. Behaviour is identical to calling
// classify() once and classifyHysteresis() thereafter.
struct BandTracker {
    bool have = false;
    Band cpu = Band::Normal;
    Band gpu = Band::Normal;

    void reset();
    void update(bool cpuValid, double cpuCelsius, bool gpuValid, double gpuCelsius);
    Band systemBand() const { return worst(cpu, gpu); }
};

} // namespace AppState
