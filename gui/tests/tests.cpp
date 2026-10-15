// Unit tests for the pure logic that does not need a display or hardware:
//   * temperature unit conversion / formatting (util/units.h)
//   * user-settings parsing and validation (app/usersettings.*)
//   * thermal band classification / hysteresis / cooling state (app/appstate.*)
//   * hwmon plausibility bounds (util/sensorrange.h)
//
// No Qt event loop, no window, no sysfs access. Exit code 0 = all pass.
//
//   cmake -S gui -B build -DEXCALIBUR_BUILD_TESTS=ON && cmake --build build
//   ./build/excalibur-tests          (or: ctest --test-dir build)

#include <cmath>
#include <cstdio>

#include "app/appstate.h"
#include "app/usersettings.h"
#include "util/sensorrange.h"
#include "util/units.h"

namespace {

int g_failures = 0;

void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL (line %d): %s\n", line, expr);
        ++g_failures;
    }
}

void checkNear(double got, double want, double eps, const char *expr, int line)
{
    if (std::fabs(got - want) > eps) {
        std::fprintf(stderr, "FAIL (line %d): %s -> got %.6f want %.6f\n", line, expr, got, want);
        ++g_failures;
    }
}

void checkStr(const QString &got, const char *want, int line)
{
    if (got != QString::fromUtf8(want)) {
        std::fprintf(stderr, "FAIL (line %d): got \"%s\" want \"%s\"\n",
                     line, got.toUtf8().constData(), want);
        ++g_failures;
    }
}

void checkBand(AppState::Band got, AppState::Band want, const char *expr, int line)
{
    if (got != want) {
        std::fprintf(stderr, "FAIL (line %d): %s -> got %d want %d\n",
                     line, expr, static_cast<int>(got), static_cast<int>(want));
        ++g_failures;
    }
}

} // namespace

#define CHECK(cond) check((cond), #cond, __LINE__)
#define CHECK_NEAR(got, want) checkNear((got), (want), 1e-9, #got " == " #want, __LINE__)
#define CHECK_STR(got, want) checkStr((got), (want), __LINE__)
#define CHECK_BAND(got, want) checkBand((got), (want), #got " == " #want, __LINE__)

int main()
{
    using Units::TemperatureUnit;

    // --- temperature conversion: known reference points -----------------------
    CHECK_NEAR(Units::fromCelsius(0.0, TemperatureUnit::Celsius), 0.0);
    CHECK_NEAR(Units::fromCelsius(100.0, TemperatureUnit::Celsius), 100.0);
    CHECK_NEAR(Units::fromCelsius(0.0, TemperatureUnit::Fahrenheit), 32.0);
    CHECK_NEAR(Units::fromCelsius(100.0, TemperatureUnit::Fahrenheit), 212.0);
    CHECK_NEAR(Units::fromCelsius(37.0, TemperatureUnit::Fahrenheit), 98.6);
    CHECK_NEAR(Units::fromCelsius(-40.0, TemperatureUnit::Fahrenheit), -40.0);
    CHECK_NEAR(Units::fromCelsius(60.0, TemperatureUnit::Fahrenheit), 140.0);

    // --- formatting -----------------------------------------------------------
    CHECK_STR(Units::format(51.0, TemperatureUnit::Celsius), "51 \u00B0C");
    CHECK_STR(Units::format(51.4, TemperatureUnit::Celsius), "51 \u00B0C"); // 0 decimals
    CHECK_STR(Units::format(20.0, TemperatureUnit::Fahrenheit), "68.0 \u00B0F"); // 1 decimal
    CHECK_STR(Units::symbol(TemperatureUnit::Celsius), "\u00B0C");
    CHECK_STR(Units::symbol(TemperatureUnit::Fahrenheit), "\u00B0F");

    // --- helper predicates ----------------------------------------------------
    CHECK(Units::decimals(TemperatureUnit::Celsius) == 0);
    CHECK(Units::decimals(TemperatureUnit::Fahrenheit) == 1);
    CHECK_NEAR(Units::spanFactor(TemperatureUnit::Celsius), 1.0);
    CHECK_NEAR(Units::spanFactor(TemperatureUnit::Fahrenheit), 9.0 / 5.0);
    CHECK(Units::isFahrenheit(TemperatureUnit::Fahrenheit));
    CHECK(!Units::isFahrenheit(TemperatureUnit::Celsius));

    // --- theme / unit parsing and validation ----------------------------------
    CHECK(UserSettings::themeFromString(QStringLiteral("light")) == UserSettings::AppTheme::Light);
    CHECK(UserSettings::themeFromString(QStringLiteral("DARK")) == UserSettings::AppTheme::Dark);
    CHECK(UserSettings::themeFromString(QStringLiteral("system")) == UserSettings::AppTheme::System);
    CHECK(UserSettings::themeFromString(QString()) == UserSettings::AppTheme::System);
    CHECK(UserSettings::themeFromString(QStringLiteral("bogus")) == UserSettings::AppTheme::System);

    CHECK(UserSettings::tempUnitFromString(QStringLiteral("fahrenheit")) == TemperatureUnit::Fahrenheit);
    CHECK(UserSettings::tempUnitFromString(QStringLiteral("Fahrenheit")) == TemperatureUnit::Fahrenheit);
    CHECK(UserSettings::tempUnitFromString(QStringLiteral("celsius")) == TemperatureUnit::Celsius);
    CHECK(UserSettings::tempUnitFromString(QString()) == TemperatureUnit::Celsius);
    CHECK(UserSettings::tempUnitFromString(QStringLiteral("kelvin")) == TemperatureUnit::Celsius);

    CHECK_STR(UserSettings::themeToString(UserSettings::AppTheme::Light), "light");
    CHECK_STR(UserSettings::themeToString(UserSettings::AppTheme::Dark), "dark");
    CHECK_STR(UserSettings::themeToString(UserSettings::AppTheme::System), "system");
    CHECK_STR(UserSettings::tempUnitToString(TemperatureUnit::Celsius), "celsius");
    CHECK_STR(UserSettings::tempUnitToString(TemperatureUnit::Fahrenheit), "fahrenheit");

    // --- refresh interval validation ------------------------------------------
    CHECK(UserSettings::refreshOptions() == QVector<int>({1000, 2000, 5000, 10000}));
    CHECK(UserSettings::isRefreshValid(1000));
    CHECK(UserSettings::isRefreshValid(2000));
    CHECK(UserSettings::isRefreshValid(5000));
    CHECK(UserSettings::isRefreshValid(10000));
    CHECK(!UserSettings::isRefreshValid(0));
    CHECK(!UserSettings::isRefreshValid(-1));
    CHECK(!UserSettings::isRefreshValid(1500));
    CHECK(!UserSettings::isRefreshValid(999999));

    // =========================================================================
    // app/appstate.h — pure classification logic
    // =========================================================================
    using AppState::Band;
    using AppState::CoolingState;

    const AppState::Thresholds cpuTh = AppState::cpuThresholds();
    const AppState::Thresholds gpuTh = AppState::gpuThresholds();

    // Documented defaults: CPU 50/74/89, GPU 45/69/84 (header, Section 6.1).
    CHECK_NEAR(cpuTh.coolMax, 50.0);
    CHECK_NEAR(cpuTh.normalMax, 74.0);
    CHECK_NEAR(cpuTh.warmMax, 89.0);
    CHECK_NEAR(gpuTh.coolMax, 45.0);
    CHECK_NEAR(gpuTh.normalMax, 69.0);
    CHECK_NEAR(gpuTh.warmMax, 84.0);
    CHECK_NEAR(AppState::kHysteresis, 2.0);

    // --- classify: exact boundary semantics (below coolMax -> Cool;
    //     <= normalMax -> Normal; <= warmMax -> Warm; above -> High). ---------
    CHECK_BAND(AppState::classify(-10.0, cpuTh), Band::Cool);
    CHECK_BAND(AppState::classify(49.999, cpuTh), Band::Cool);
    CHECK_BAND(AppState::classify(50.0, cpuTh), Band::Normal);  // ON coolMax
    CHECK_BAND(AppState::classify(60.0, cpuTh), Band::Normal);
    CHECK_BAND(AppState::classify(74.0, cpuTh), Band::Normal);  // ON normalMax
    CHECK_BAND(AppState::classify(74.001, cpuTh), Band::Warm);  // just above
    CHECK_BAND(AppState::classify(80.0, cpuTh), Band::Warm);
    CHECK_BAND(AppState::classify(89.0, cpuTh), Band::Warm);    // ON warmMax
    CHECK_BAND(AppState::classify(89.001, cpuTh), Band::High);  // just above

    CHECK_BAND(AppState::classify(44.999, gpuTh), Band::Cool);
    CHECK_BAND(AppState::classify(45.0, gpuTh), Band::Normal);  // ON coolMax
    CHECK_BAND(AppState::classify(69.0, gpuTh), Band::Normal);  // ON normalMax
    CHECK_BAND(AppState::classify(69.001, gpuTh), Band::Warm);
    CHECK_BAND(AppState::classify(84.0, gpuTh), Band::Warm);    // ON warmMax
    CHECK_BAND(AppState::classify(84.001, gpuTh), Band::High);

    // --- classifyHysteresis: rising crosses immediately -----------------------
    CHECK_BAND(AppState::classifyHysteresis(50.0, cpuTh, Band::Cool), Band::Normal);
    CHECK_BAND(AppState::classifyHysteresis(80.0, cpuTh, Band::Normal), Band::Warm);
    CHECK_BAND(AppState::classifyHysteresis(95.0, cpuTh, Band::Warm), Band::High);
    CHECK_BAND(AppState::classifyHysteresis(100.0, cpuTh, Band::Cool), Band::High);

    // --- classifyHysteresis: falling must clear the boundary by kHysteresis ---
    // From High (cpu warmMax=89): must reach <= 87 to drop.
    CHECK_BAND(AppState::classifyHysteresis(88.0, cpuTh, Band::High), Band::High);  // inside margin
    CHECK_BAND(AppState::classifyHysteresis(87.5, cpuTh, Band::High), Band::High);  // inside margin
    CHECK_BAND(AppState::classifyHysteresis(87.0, cpuTh, Band::High), Band::Warm);  // exactly cleared
    CHECK_BAND(AppState::classifyHysteresis(80.0, cpuTh, Band::High), Band::Warm);
    // From Warm (cpu normalMax=74): must reach <= 72 to drop.
    CHECK_BAND(AppState::classifyHysteresis(73.0, cpuTh, Band::Warm), Band::Warm);
    CHECK_BAND(AppState::classifyHysteresis(72.0, cpuTh, Band::Warm), Band::Normal);
    // From Normal (cpu coolMax=50): must reach <= 48 to drop.
    CHECK_BAND(AppState::classifyHysteresis(49.0, cpuTh, Band::Normal), Band::Normal);
    CHECK_BAND(AppState::classifyHysteresis(48.0, cpuTh, Band::Normal), Band::Cool);
    // Same band stays the same.
    CHECK_BAND(AppState::classifyHysteresis(60.0, cpuTh, Band::Normal), Band::Normal);

    // --- worst: hottest of two -------------------------------------------------
    CHECK_BAND(AppState::worst(Band::Cool, Band::High), Band::High);
    CHECK_BAND(AppState::worst(Band::High, Band::Cool), Band::High);
    CHECK_BAND(AppState::worst(Band::Warm, Band::Normal), Band::Warm);
    CHECK_BAND(AppState::worst(Band::Normal, Band::Warm), Band::Warm);
    CHECK_BAND(AppState::worst(Band::Cool, Band::Cool), Band::Cool);
    CHECK_BAND(AppState::worst(Band::Normal, Band::Normal), Band::Normal);
    CHECK_BAND(AppState::worst(Band::High, Band::High), Band::High);

    // --- bandLabel -------------------------------------------------------------
    CHECK_STR(AppState::bandLabel(Band::Cool), "Cool");
    CHECK_STR(AppState::bandLabel(Band::Normal), "Normal");
    CHECK_STR(AppState::bandLabel(Band::Warm), "Warm");
    CHECK_STR(AppState::bandLabel(Band::High), "High");

    // --- coolingState: each documented case -----------------------------------
    CHECK(AppState::coolingState(false, false, Band::Cool) == CoolingState::Unavailable);
    CHECK(AppState::coolingState(false, true, Band::High) == CoolingState::Unavailable);
    CHECK(AppState::coolingState(true, false, Band::Cool) == CoolingState::Idle);
    CHECK(AppState::coolingState(true, false, Band::High) == CoolingState::Idle);
    CHECK(AppState::coolingState(true, true, Band::Cool) == CoolingState::Active);
    CHECK(AppState::coolingState(true, true, Band::Normal) == CoolingState::Active);
    CHECK(AppState::coolingState(true, true, Band::Warm) == CoolingState::Elevated);
    CHECK(AppState::coolingState(true, true, Band::High) == CoolingState::Elevated);

    CHECK_STR(AppState::coolingStateLabel(CoolingState::Unavailable), "Cooling unavailable");
    CHECK_STR(AppState::coolingStateLabel(CoolingState::Idle), "Cooling idle");
    CHECK_STR(AppState::coolingStateLabel(CoolingState::Active), "Cooling active");
    CHECK_STR(AppState::coolingStateLabel(CoolingState::Elevated), "Cooling elevated");

    // --- displaySensorLabel: trim, fallback, cap (default maxChars=14) --------
    CHECK_STR(AppState::displaySensorLabel(QStringLiteral("  CPU  "), QStringLiteral("fb")), "CPU");
    CHECK_STR(AppState::displaySensorLabel(QStringLiteral("   "), QStringLiteral("CPU")), "CPU");
    CHECK_STR(AppState::displaySensorLabel(QString(), QStringLiteral("GPU Fan")), "GPU Fan");
    // Exactly maxChars is not truncated.
    CHECK_STR(AppState::displaySensorLabel(QStringLiteral("abcdefghijklmn"), QStringLiteral("fb")),
              "abcdefghijklmn");
    // Longer than maxChars -> left(maxChars-1) + ellipsis (14 chars total).
    CHECK_STR(AppState::displaySensorLabel(QStringLiteral("abcdefghijklmnop"), QStringLiteral("fb")),
              "abcdefghijklm\u2026");
    // Custom maxChars.
    CHECK_STR(AppState::displaySensorLabel(QStringLiteral("abcdefghij"), QStringLiteral("fb"), 5),
              "abcd\u2026");
    // maxChars <= 0 disables the cap.
    CHECK_STR(AppState::displaySensorLabel(QStringLiteral("abcdefghijklmnop"), QStringLiteral("fb"), 0),
              "abcdefghijklmnop");

    // --- BandTracker -----------------------------------------------------------
    {
        AppState::BandTracker t;
        CHECK(!t.have);
        CHECK_BAND(t.cpu, Band::Normal);
        CHECK_BAND(t.gpu, Band::Normal);
        CHECK_BAND(t.systemBand(), Band::Normal);

        // First reading uses classify() directly (cpu 95 -> High, gpu 30 -> Cool).
        t.update(true, 95.0, true, 30.0);
        CHECK(t.have);
        CHECK_BAND(t.cpu, Band::High);
        CHECK_BAND(t.gpu, Band::Cool);
        CHECK_BAND(t.systemBand(), Band::High);

        // Subsequent readings use hysteresis: 88 is inside the margin -> stays High.
        t.update(true, 88.0, true, 30.0);
        CHECK_BAND(t.cpu, Band::High);
        // Clearing the margin (<= 87) drops to Warm.
        t.update(true, 87.0, true, 30.0);
        CHECK_BAND(t.cpu, Band::Warm);
        CHECK_BAND(t.systemBand(), Band::Warm);

        // Invalid readings leave the tracked band untouched.
        t.update(false, 0.0, false, 0.0);
        CHECK_BAND(t.cpu, Band::Warm);
        CHECK_BAND(t.gpu, Band::Cool);

        // GPU uses gpuThresholds (84 warmMax): 90 -> High.
        t.update(false, 0.0, true, 90.0);
        CHECK_BAND(t.gpu, Band::High);
        CHECK_BAND(t.systemBand(), Band::High);

        // reset() clears have and returns both bands to Normal.
        t.reset();
        CHECK(!t.have);
        CHECK_BAND(t.cpu, Band::Normal);
        CHECK_BAND(t.gpu, Band::Normal);
        CHECK_BAND(t.systemBand(), Band::Normal);

        // After reset the next update classifies plainly again (no hysteresis).
        t.update(true, 95.0, false, 0.0);
        CHECK_BAND(t.cpu, Band::High);
    }

    // =========================================================================
    // util/sensorrange.h — inclusive plausibility bounds
    // =========================================================================
    // Use the ACTUAL constants, not assumed values.
    CHECK(SensorRange::temperatureCelsiusValid(SensorRange::kTempMinC));   // 0.0
    CHECK(SensorRange::temperatureCelsiusValid(SensorRange::kTempMaxC));   // 150.0
    CHECK(SensorRange::temperatureCelsiusValid(36.5));
    CHECK(SensorRange::temperatureCelsiusValid(95.0));
    CHECK(!SensorRange::temperatureCelsiusValid(SensorRange::kTempMinC - 0.1));
    CHECK(!SensorRange::temperatureCelsiusValid(SensorRange::kTempMaxC + 0.1));
    CHECK(!SensorRange::temperatureCelsiusValid(-5.0));   // audit B7: sub-zero rejected
    CHECK(!SensorRange::temperatureCelsiusValid(999999.0));

    CHECK(SensorRange::fanRpmValid(SensorRange::kFanMinRpm));              // 0 is valid
    CHECK(SensorRange::fanRpmValid(SensorRange::kFanMaxRpm));              // 30000
    CHECK(SensorRange::fanRpmValid(1500));
    CHECK(!SensorRange::fanRpmValid(SensorRange::kFanMinRpm - 1));
    CHECK(!SensorRange::fanRpmValid(SensorRange::kFanMaxRpm + 1));
    CHECK(!SensorRange::fanRpmValid(999999));

    if (g_failures == 0) {
        std::printf("all tests passed\n");
        return 0;
    }
    std::fprintf(stderr, "%d check(s) failed\n", g_failures);
    return 1;
}
