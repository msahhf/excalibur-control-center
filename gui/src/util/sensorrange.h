#pragma once

// Plausibility bounds for read-only hwmon telemetry.
//
// The hwmon files under /sys/class/hwmon are untrusted input: a driver, a
// firmware bug or a mis-mapped register can emit a nonsense sentinel (e.g. the
// classic 16-bit 0xFFFF, a sign-extended negative, or a wildly out-of-range
// glitch). Such readings are NOT physically meaningful and must not be shown as
// if they were real hardware measurements.
//
// IMPORTANT: out-of-range is treated as UNAVAILABLE, never clamped or smoothed.
// The value is simply reported as missing, so the field stays invalid and the
// device status degrades honestly. Nothing is modified or adjusted.
namespace SensorRange {

// Temperature in degrees Celsius.
//
// Upper bound 150.0 C: comfortably above any real CPU/GPU junction limit
// (~105-115 C) while rejecting obvious garbage sentinels such as 0xFFFF raw
// millidegrees and wildly out-of-range driver/EC glitches.
//
// Lower bound 0.0 C: a powered CPU/GPU die cannot plausibly report a sub-zero
// temperature; negative hwmon values are sign-extended "negative noise" rather
// than real measurements. This deliberately rejects the -5 C sample from the
// audit (B7). (Note: a lower bound of -40.0 C would *admit* -5 C, so it cannot
// be used together with the requirement that -5 C be reported as unavailable.)
constexpr double kTempMinC = 0.0;
constexpr double kTempMaxC = 150.0;

// Fan speed in RPM.
//
// 0 is a valid reading: a fan may legitimately be stopped. The ceiling is
// deliberately generous (30000 RPM) -- far above any plausible laptop fan, but
// finite, so absurd values (e.g. 999999999) are rejected as unavailable rather
// than displayed.
constexpr long kFanMinRpm = 0;
constexpr long kFanMaxRpm = 30000;

// True when a Celsius temperature is within the plausible bounds above.
inline bool temperatureCelsiusValid(double c)
{
    return c >= kTempMinC && c <= kTempMaxC;
}

// True when an RPM reading is within the plausible bounds above.
inline bool fanRpmValid(long rpm)
{
    return rpm >= kFanMinRpm && rpm <= kFanMaxRpm;
}

} // namespace SensorRange
