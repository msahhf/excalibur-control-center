// Unit tests for the pure logic that does not need a display or hardware:
//   * temperature unit conversion / formatting (util/units.h)
//   * user-settings parsing and validation (app/usersettings.*)
//   * thermal band classification / hysteresis / cooling state (app/appstate.*)
//   * hwmon plausibility bounds (util/sensorrange.h)
//   * RGB LED 32-bit packing / whitelists / token conversion (app/leddata.*)
//     and the sysfs client against a QTemporaryDir mock (app/rgbclient.*);
//     the real /sys tree is never touched. This includes the NON-atomic
//     partial-write behaviour of RgbClient::write(): each of the three
//     attributes is made to fail in turn and the attributes that survived are
//     recorded, so the (absence of a) rollback guarantee is pinned down.
//   * WCAG 2.1 contrast / compositing helpers (app/contrast.*).
//
// No Qt event loop, no window, no sysfs access. Exit code 0 = all pass.
//
//   cmake -S gui -B build -DEXCALIBUR_BUILD_TESTS=ON && cmake --build build
//   ./build/excalibur-tests          (or: ctest --test-dir build)

#include <cmath>
#include <cstdio>

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QVector>

#include "app/appstate.h"
#include "app/contrast.h"
#include "app/leddata.h"
#include "app/rgbclient.h"
#include "app/usersettings.h"
#include "theme/theme.h"
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
#define CHECK_NEAR_EPS(got, want, eps) checkNear((got), (want), (eps), #got " ~= " #want, __LINE__)
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

    // --- language parsing, serialization and round-trip ----------------------
    using Language = UserSettings::Language;
    CHECK_STR(UserSettings::languageToString(Language::System), "system");
    CHECK_STR(UserSettings::languageToString(Language::English), "en");
    CHECK_STR(UserSettings::languageToString(Language::Turkish), "tr");

    CHECK(UserSettings::languageFromString(QStringLiteral("system")) == Language::System);
    CHECK(UserSettings::languageFromString(QStringLiteral("en")) == Language::English);
    CHECK(UserSettings::languageFromString(QStringLiteral("tr")) == Language::Turkish);

    // Round trip: fromString(toString(x)) == x for every enum value.
    CHECK(UserSettings::languageFromString(UserSettings::languageToString(Language::System))
          == Language::System);
    CHECK(UserSettings::languageFromString(UserSettings::languageToString(Language::English))
          == Language::English);
    CHECK(UserSettings::languageFromString(UserSettings::languageToString(Language::Turkish))
          == Language::Turkish);

    // Unknown / empty input falls back to System (matching theme parsing).
    CHECK(UserSettings::languageFromString(QStringLiteral("bogus")) == Language::System);
    CHECK(UserSettings::languageFromString(QString()) == Language::System);
    // Case-insensitive (trimmed + toLower) as documented by A4.
    CHECK(UserSettings::languageFromString(QStringLiteral("TR")) == Language::Turkish);
    CHECK(UserSettings::languageFromString(QStringLiteral("EN")) == Language::English);

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

    // =========================================================================
    // app/leddata.* — 32-bit packing, whitelists and stable string tokens
    // =========================================================================
    {
        using namespace Lighting;

        // --- reference vectors straight from the driver contract -------------
        {
            const LedData d = unpack(0x61FF0080u);
            CHECK(static_cast<int>(d.effect) == 6);       // ColorfulCycle
            CHECK(d.brightness == 1);
            CHECK(d.red == 255);
            CHECK(d.green == 0);
            CHECK(d.blue == 128);
        }
        {
            const LedData d = unpack(0x12FF0000u);
            CHECK(static_cast<int>(d.effect) == 1);       // Static
            CHECK(d.brightness == 2);
            CHECK(d.red == 255);
            CHECK(d.green == 0);
            CHECK(d.blue == 0);
        }
        {
            const LedData d = unpack(0x1100FFFFu);
            CHECK(static_cast<int>(d.effect) == 1);       // Static
            CHECK(d.brightness == 1);
            CHECK(d.red == 0);
            CHECK(d.green == 255);
            CHECK(d.blue == 255);
        }

        // --- explicit packing matches the nibble layout ----------------------
        {
            LedData d;
            d.effect = Effect::ColorfulCycle;
            d.brightness = 1;
            d.red = 255;
            d.green = 0;
            d.blue = 128;
            CHECK(pack(d) == 0x61FF0080u);
        }

        // --- round-trip for every whitelisted effect with extreme colours ----
        {
            const int effects[] = {0, 1, 3, 6, 7};
            for (int e : effects) {
                LedData in;
                in.effect = static_cast<Effect>(e);
                in.brightness = 15;
                in.red = 255;
                in.green = 0;
                in.blue = 255;
                const LedData out = unpack(pack(in));
                CHECK(static_cast<int>(out.effect) == e);
                CHECK(out.brightness == 15);
                CHECK(out.red == 255);
                CHECK(out.green == 0);
                CHECK(out.blue == 255);
            }
            // Zero word round-trips too.
            const LedData zero = unpack(pack(LedData{}));
            CHECK(static_cast<int>(zero.effect) == 0);
            CHECK(zero.brightness == 0);
            CHECK(zero.red == 0 && zero.green == 0 && zero.blue == 0);
        }

        // --- effect whitelist boundaries -------------------------------------
        CHECK(isValidEffect(0));
        CHECK(isValidEffect(1));
        CHECK(isValidEffect(3));
        CHECK(isValidEffect(6));
        CHECK(isValidEffect(7));
        CHECK(!isValidEffect(2));   // reserved gap
        CHECK(!isValidEffect(4));   // reserved gap
        CHECK(!isValidEffect(5));   // reserved gap
        CHECK(!isValidEffect(8));
        CHECK(!isValidEffect(15));
        CHECK(!isValidEffect(-1));

        // --- zone whitelist boundaries ---------------------------------------
        CHECK(isValidZone(0));
        CHECK(isValidZone(3));
        CHECK(isValidZone(4));
        CHECK(isValidZone(5));
        CHECK(isValidZone(6));
        CHECK(isValidZone(7));
        CHECK(isValidZone(8));
        CHECK(!isValidZone(1));
        CHECK(!isValidZone(2));
        CHECK(!isValidZone(9));
        CHECK(!isValidZone(15));
        CHECK(!isValidZone(-1));

        // --- brightness bounds ------------------------------------------------
        CHECK(isValidBrightness(kBrightnessMin));   // 0
        CHECK(isValidBrightness(kBrightnessMax));   // 15
        CHECK(isValidBrightness(7));
        CHECK(!isValidBrightness(-1));
        CHECK(!isValidBrightness(16));
        CHECK(!isValidBrightness(255));

        // --- isValidLedData is the nibble whitelist --------------------------
        CHECK(isValidLedData(0x61FF0080u));          // effect 6  -> valid
        CHECK(isValidLedData(0x10000000u));          // effect 1  -> valid
        CHECK(isValidLedData(0x70FFFFFFu));          // effect 7  -> valid
        CHECK(!isValidLedData(0x20000000u));         // effect 2  -> rejected
        CHECK(!isValidLedData(0x40000000u));         // effect 4  -> rejected
        CHECK(!isValidLedData(0xA0000000u));         // effect 10 -> rejected

        // =====================================================================
        // F2: raw (pre-packing) field validation. isValid() must see the values
        // at full width, so a field pack() would mask away is still rejected.
        // =====================================================================

        // --- raw brightness bounds: 0..15 only, wide/signed included ---------
        {
            LedData d;
            d.brightness = 0;
            CHECK(isValid(d));
            d.brightness = kBrightnessMax; // 15
            CHECK(isValid(d));
            d.brightness = 7;
            CHECK(isValid(d));

            // Values that pack() would silently mask into range (200 & 0x0F == 8)
            // must be rejected by the raw validator...
            d.brightness = 16;
            CHECK(!isValid(d));
            d.brightness = 200;
            CHECK(!isValid(d));
            d.brightness = 255;
            CHECK(!isValid(d));

            // ...even though the packed word still looks "valid".
            LedData masked = d; // brightness == 255 -> nibble 15
            masked.brightness = 255;
            CHECK(isValidLedData(pack(masked)));   // word-level check is fooled
            CHECK(!isValid(masked));               // raw check is not
        }

        // --- wide / signed predicates used before narrowing -----------------
        CHECK(!isValidBrightness(-1));
        CHECK(!isValidBrightness(16));
        CHECK(!isValidBrightness(200));
        CHECK(!isValidBrightness(256));
        CHECK(isValidBrightness(0));
        CHECK(isValidBrightness(15));

        // --- invalid effect enum (including nibble-wrapping values) ----------
        {
            LedData d;
            d.effect = Effect::Off;
            CHECK(isValid(d));
            d.effect = Effect::ColorfulCycle;
            CHECK(isValid(d));
            d.effect = static_cast<Effect>(2);   // reserved gap
            CHECK(!isValid(d));
            d.effect = static_cast<Effect>(4);   // reserved gap
            CHECK(!isValid(d));
            d.effect = static_cast<Effect>(5);   // reserved gap
            CHECK(!isValid(d));
            // 0x16 == 22 wraps to nibble 6 == ColorfulCycle (whitelisted!):
            // pack() masks it into range, the raw validator must not.
            d.effect = static_cast<Effect>(0x16);
            CHECK(!isValid(d));
            CHECK(static_cast<int>(unpack(pack(d)).effect) == 6);
            CHECK(isValidLedData(pack(d)));
        }

        // --- wide colour component (narrowing-overflow guard) ---------------
        CHECK(isValidColorComponent(0));
        CHECK(isValidColorComponent(255));
        CHECK(isValidColorComponent(128));
        CHECK(!isValidColorComponent(-1));
        CHECK(!isValidColorComponent(256));
        CHECK(!isValidColorComponent(65535));

        // --- valid input still packs to the ORIGINAL bit layout -------------
        // (the F2 hardening must not have moved a single bit)
        {
            LedData d;
            d.effect = Effect::Static;      // 1
            d.brightness = 15;              // F
            d.red = 255;                    // FF
            d.green = 0;                    // 00
            d.blue = 255;                   // FF
            CHECK(isValid(d));
            CHECK(pack(d) == 0x1FFF00FFu);

            d.effect = Effect::Ambilight;   // 7
            d.brightness = 0;
            d.red = 0;
            d.green = 128;                  // 80
            d.blue = 1;
            CHECK(isValid(d));
            CHECK(pack(d) == 0x70008001u);
        }

        // --- effect token round-trip -----------------------------------------
        CHECK_STR(effectToString(Effect::Off), "off");
        CHECK_STR(effectToString(Effect::Static), "static");
        CHECK_STR(effectToString(Effect::Breathing), "breathing");
        CHECK_STR(effectToString(Effect::ColorfulCycle), "colorful-cycle");
        CHECK_STR(effectToString(Effect::Ambilight), "ambilight");

        {
            bool ok = false;
            CHECK(effectFromString(QStringLiteral("breathing"), &ok) == Effect::Breathing);
            CHECK(ok);
            ok = false;
            CHECK(effectFromString(QStringLiteral("colorful-cycle"), &ok) == Effect::ColorfulCycle);
            CHECK(ok);
            // Legacy alias parses, but canonical output is always dashed.
            ok = false;
            CHECK(effectFromString(QStringLiteral("colorful_cycle"), &ok) == Effect::ColorfulCycle);
            CHECK(ok);
            CHECK_STR(effectToString(effectFromString(QStringLiteral("colorful_cycle"))),
                      "colorful-cycle");
            // Unknown token -> safe default + ok == false.
            ok = true;
            CHECK(effectFromString(QStringLiteral("disco"), &ok) == Effect::Off);
            CHECK(!ok);
            ok = true;
            CHECK(effectFromString(QString(), &ok) == Effect::Off);
            CHECK(!ok);
            // A null ok pointer must not crash.
            CHECK(effectFromString(QStringLiteral("nonsense")) == Effect::Off);
        }

        // --- F2/L1: makeLedData validates the RAW int input BEFORE narrowing --
        // The whole point: the caller's wide/signed values must be checked
        // before they land in LedData's fixed-width fields, so a value that a
        // cast would wrap/truncate (brightness 256 -> 0, red -1 -> 255) is
        // rejected instead of silently becoming valid.
        {
            // Valid boundaries produce a value...
            CHECK(makeLedData(static_cast<int>(Effect::Static), 15, 255, 0, 255).has_value());
            CHECK(makeLedData(static_cast<int>(Effect::Off), 0, 0, 0, 0).has_value());
            CHECK(makeLedData(static_cast<int>(Effect::Ambilight), 7, 1, 128, 254).has_value());

            // ...and a valid vector packs to the ORIGINAL layout (no bit moved).
            const auto packed = makeLedData(static_cast<int>(Effect::Static), 15, 255, 0, 255);
            CHECK(packed.has_value());
            if (packed)
                CHECK(pack(*packed) == 0x1FFF00FFu);

            // Every whitelisted effect is accepted; every other int is not.
            for (int e : {0, 1, 3, 6, 7})
                CHECK(makeLedData(e, 0, 0, 0, 0).has_value());
            CHECK(!makeLedData(-1, 0, 0, 0, 0).has_value());
            CHECK(!makeLedData(2, 0, 0, 0, 0).has_value());   // reserved gap
            CHECK(!makeLedData(4, 0, 0, 0, 0).has_value());   // reserved gap
            CHECK(!makeLedData(5, 0, 0, 0, 0).has_value());   // reserved gap
            CHECK(!makeLedData(8, 0, 0, 0, 0).has_value());
            CHECK(!makeLedData(15, 0, 0, 0, 0).has_value());
            CHECK(!makeLedData(0x16, 0, 0, 0, 0).has_value()); // would wrap to 6

            // brightness: only 0..15, wide/signed included.
            CHECK(makeLedData(0, 0, 0, 0, 0).has_value());
            CHECK(makeLedData(0, 15, 0, 0, 0).has_value());
            CHECK(!makeLedData(0, -1, 0, 0, 0).has_value());
            CHECK(!makeLedData(0, 16, 0, 0, 0).has_value());
            CHECK(!makeLedData(0, 200, 0, 0, 0).has_value());
            CHECK(!makeLedData(0, 255, 0, 0, 0).has_value());
            CHECK(!makeLedData(0, 256, 0, 0, 0).has_value());  // would narrow to 0
            CHECK(!makeLedData(0, 257, 0, 0, 0).has_value());

            // colour components: only 0..255, per channel, negative included.
            CHECK(makeLedData(0, 0, 0, 0, 0).has_value());
            CHECK(makeLedData(0, 0, 255, 255, 255).has_value());
            CHECK(!makeLedData(0, 0, -1, 0, 0).has_value());   // would narrow to 255
            CHECK(!makeLedData(0, 0, 256, 0, 0).has_value());
            CHECK(!makeLedData(0, 0, 257, 0, 0).has_value());
            CHECK(!makeLedData(0, 0, 0, -1, 0).has_value());
            CHECK(!makeLedData(0, 0, 0, 256, 0).has_value());
            CHECK(!makeLedData(0, 0, 0, 0, -1).has_value());
            CHECK(!makeLedData(0, 0, 0, 0, 256).has_value());

            // The exact narrowing trap in isolation: a bad value must NOT come
            // out equal to the LedData a masked/wrapped cast would have built.
            const auto wrappedBrightness = makeLedData(0, 256, 0, 0, 0);
            CHECK(!wrappedBrightness.has_value());
            const auto wrappedRed = makeLedData(0, 0, -1, 0, 0);
            CHECK(!wrappedRed.has_value());
        }

        // --- F2/L1: an UNKNOWN zone must never degrade to "all" --------------
        // zoneToString is asymmetric: known zones keep their token, an unknown
        // enum returns an EMPTY string (never "all").
        CHECK_STR(zoneToString(static_cast<Zone>(1)), "");
        CHECK_STR(zoneToString(static_cast<Zone>(2)), "");
        CHECK_STR(zoneToString(static_cast<Zone>(9)), "");
        CHECK_STR(zoneToString(static_cast<Zone>(255)), "");

        // --- zone token round-trip -------------------------------------------
        CHECK_STR(zoneToString(Zone::All), "all");
        CHECK_STR(zoneToString(Zone::ZoneC), "zone-c");
        CHECK_STR(zoneToString(Zone::ZoneB), "zone-b");
        CHECK_STR(zoneToString(Zone::ZoneA), "zone-a");
        CHECK_STR(zoneToString(Zone::Keyboard), "keyboard");
        CHECK_STR(zoneToString(Zone::Trunk), "trunk");
        CHECK_STR(zoneToString(Zone::Logo), "logo");

        {
            bool ok = false;
            CHECK(zoneFromString(QStringLiteral("zone-a"), &ok) == Zone::ZoneA);
            CHECK(ok);
            ok = false;
            CHECK(zoneFromString(QStringLiteral("all"), &ok) == Zone::All);
            CHECK(ok);
            ok = false;
            CHECK(zoneFromString(QStringLiteral("logo"), &ok) == Zone::Logo);
            CHECK(ok);
            ok = true;
            CHECK(zoneFromString(QStringLiteral("zone-z"), &ok) == Zone::All);
            CHECK(!ok);
            ok = true;
            CHECK(zoneFromString(QString(), &ok) == Zone::All);
            CHECK(!ok);
            CHECK(zoneFromString(QStringLiteral("nonsense")) == Zone::All);
        }
    }

    // =========================================================================
    // app/rgbclient.* — sysfs client against a temporary mock tree.
    // The real /sys is NEVER touched: the base path is a QTemporaryDir.
    // =========================================================================
    {
        using namespace Lighting;

        QTemporaryDir tmp;
        CHECK(tmp.isValid());
        const QString zoneDir = tmp.path() + QStringLiteral("/excalibur::zone-a");
        CHECK(QDir().mkpath(zoneDir));

        const auto put = [](const QString &path, const char *value) {
            QFile f(path);
            const QByteArray bytes(value);
            return f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size();
        };
        const auto slurp = [](const QString &path) {
            QFile f(path);
            if (!f.open(QIODevice::ReadOnly))
                return QString();
            return QString::fromUtf8(f.readAll());
        };

        CHECK(put(zoneDir + "/multi_intensity", "10 20 30\n"));
        CHECK(put(zoneDir + "/brightness", "5\n"));
        CHECK(put(zoneDir + "/effect", "static\n"));

        RgbClient client(tmp.path());

        // Nothing discovered until discover() is called.
        CHECK(!client.available());
        CHECK(client.availableZones().isEmpty());

        CHECK(client.discover());
        CHECK(client.available());

        const QVector<Zone> zones = client.availableZones();
        CHECK(zones.size() == 1);
        CHECK(!zones.isEmpty() && zones.first() == Zone::ZoneA);
        CHECK(client.zonePath(Zone::ZoneA) == zoneDir);

        // --- read() decodes the mock files -----------------------------------
        {
            const auto got = client.read(Zone::ZoneA);
            CHECK(got.has_value());
            if (got) {
                CHECK(got->effect == Effect::Static);
                CHECK(got->brightness == 5);
                CHECK(got->red == 10);
                CHECK(got->green == 20);
                CHECK(got->blue == 30);
            }
        }
        // Unknown / absent zone -> nullopt.
        CHECK(!client.read(Zone::ZoneB).has_value());

        // A missing attribute -> nullopt (no partial/garbage result).
        CHECK(QFile::remove(zoneDir + "/effect"));
        CHECK(!client.read(Zone::ZoneA).has_value());
        CHECK(put(zoneDir + "/effect", "static\n"));   // restore
        CHECK(client.read(Zone::ZoneA).has_value());

        // --- (a) writes disabled: false, and NO file is modified -------------
        const LedData want = [] {
            LedData d;
            d.effect = Effect::Breathing;
            d.brightness = 7;
            d.red = 1;
            d.green = 2;
            d.blue = 3;
            return d;
        }();
        CHECK_STR(RgbClient::multiIntensityLine(want), "1 2 3");

        const QString intensityBefore = slurp(zoneDir + "/multi_intensity");
        const QString brightnessBefore = slurp(zoneDir + "/brightness");
        const QString effectBefore = slurp(zoneDir + "/effect");

        CHECK(!client.hardwareWriteEnabled());
        CHECK(!client.write(Zone::ZoneA, want));
        CHECK_STR(slurp(zoneDir + "/multi_intensity"), "10 20 30\n");
        CHECK(slurp(zoneDir + "/multi_intensity") == intensityBefore);
        CHECK(slurp(zoneDir + "/brightness") == brightnessBefore);
        CHECK(slurp(zoneDir + "/effect") == effectBefore);

        // --- (b) unknown zone -> false even when writes are enabled ----------
        client.setHardwareWriteEnabled(true);
        CHECK(client.hardwareWriteEnabled());
        CHECK(!client.write(Zone::ZoneB, want));
        CHECK(slurp(zoneDir + "/multi_intensity") == intensityBefore);

        // --- (c) non-whitelisted effect nibble -> false, unchanged -----------
        LedData bad = want;
        bad.effect = static_cast<Effect>(2);   // reserved / not whitelisted
        CHECK(!client.write(Zone::ZoneA, bad));
        CHECK(slurp(zoneDir + "/multi_intensity") == intensityBefore);
        CHECK(slurp(zoneDir + "/brightness") == brightnessBefore);
        CHECK(slurp(zoneDir + "/effect") == effectBefore);

        // --- (d) valid write succeeds and updates all three attributes -------
        CHECK(client.write(Zone::ZoneA, want));
        CHECK_STR(slurp(zoneDir + "/multi_intensity"), "1 2 3\n");
        CHECK_STR(slurp(zoneDir + "/brightness"), "7\n");
        CHECK_STR(slurp(zoneDir + "/effect"), "breathing\n");

        // ...and read() sees the new state.
        {
            const auto back = client.read(Zone::ZoneA);
            CHECK(back.has_value());
            if (back) {
                CHECK(back->effect == Effect::Breathing);
                CHECK(back->brightness == 7);
                CHECK(back->red == 1);
                CHECK(back->green == 2);
                CHECK(back->blue == 3);
            }
        }

        // --- (e) out-of-range raw fields -> false, no file touched -----------
        // Writes are enabled here. Every attribute file is writable, so ANY
        // successful open(Truncate) would have emptied it: unchanged content
        // AND an unchanged size together prove write() returned before opening
        // a single file (pack()'s nibble masking is not enough on its own --
        // brightness 200 masks to 8 and used to be written as the literal
        // "200").
        {
            const QString files[] = {zoneDir + "/multi_intensity",
                                     zoneDir + "/brightness",
                                     zoneDir + "/effect"};
            const auto sizeOf = [](const QString &p) { return QFile(p).size(); };
            const qint64 sizesBefore[] = {sizeOf(files[0]), sizeOf(files[1]), sizeOf(files[2])};
            const QString before[] = {slurp(files[0]), slurp(files[1]), slurp(files[2])};

            LedData badBright = want;
            badBright.brightness = 200;                    // masks to 8
            CHECK(!client.write(Zone::ZoneA, badBright));
            badBright.brightness = 16;
            CHECK(!client.write(Zone::ZoneA, badBright));
            badBright.brightness = 255;
            CHECK(!client.write(Zone::ZoneA, badBright));

            LedData badEffect = want;
            badEffect.effect = static_cast<Effect>(4);     // reserved gap
            CHECK(!client.write(Zone::ZoneA, badEffect));
            badEffect.effect = static_cast<Effect>(0x16);  // wraps to nibble 6
            CHECK(!client.write(Zone::ZoneA, badEffect));

            // Nothing changed, and nothing was even truncated.
            CHECK(slurp(files[0]) == before[0]);
            CHECK(slurp(files[1]) == before[1]);
            CHECK(slurp(files[2]) == before[2]);
            CHECK(sizeOf(files[0]) == sizesBefore[0]);
            CHECK(sizeOf(files[1]) == sizesBefore[1]);
            CHECK(sizeOf(files[2]) == sizesBefore[2]);
        }

        // --- (f) invalid zone enum -> rejected, NEVER mapped onto All --------
        // An "excalibur::all" directory is present in the mock, so the old
        // behaviour (zoneToString's safe default rewrote the unknown id into
        // "all") would have silently written there. It must not.
        {
            const QString allDir = tmp.path() + QStringLiteral("/excalibur::all");
            CHECK(QDir().mkpath(allDir));
            CHECK(put(allDir + "/multi_intensity", "9 9 9\n"));
            CHECK(put(allDir + "/brightness", "1\n"));
            CHECK(put(allDir + "/effect", "off\n"));

            CHECK(client.discover());                      // now also finds All
            CHECK(client.availableZones().contains(Zone::All));
            CHECK(client.zonePath(Zone::All) == allDir);

            CHECK(!client.write(static_cast<Zone>(1), want));   // reserved id
            CHECK(!client.write(static_cast<Zone>(2), want));   // reserved id
            CHECK(!client.write(static_cast<Zone>(9), want));   // reserved id
            CHECK_STR(slurp(allDir + "/multi_intensity"), "9 9 9\n");
            CHECK_STR(slurp(allDir + "/brightness"), "1\n");
            CHECK_STR(slurp(allDir + "/effect"), "off\n");

            // --- F2/L1: read() must reject an unknown zone BEFORE any open ---
            // "excalibur::all" is present with fully decodable content, so the
            // old fallback onto All would have returned (9,9,9) here. The
            // result must be nullopt, never the All value.
            CHECK(!client.read(static_cast<Zone>(1)).has_value());
            CHECK(!client.read(static_cast<Zone>(2)).has_value());
            CHECK(!client.read(static_cast<Zone>(9)).has_value());
            CHECK(!client.read(static_cast<Zone>(255)).has_value());
            // An unknown zone has no on-disk name at all.
            CHECK(client.zonePath(static_cast<Zone>(1)).isEmpty());
            CHECK(client.zonePath(static_cast<Zone>(9)).isEmpty());
            // Nothing was read/changed on the All directory by those calls.
            CHECK_STR(slurp(allDir + "/multi_intensity"), "9 9 9\n");
            CHECK_STR(slurp(allDir + "/brightness"), "1\n");
            CHECK_STR(slurp(allDir + "/effect"), "off\n");

            // Positive control: the real All zone still reads normally (so the
            // nullopt above is a rejection, not a broken mock).
            {
                const auto all = client.read(Zone::All);
                CHECK(all.has_value());
                if (all) {
                    CHECK(all->effect == Effect::Off);
                    CHECK(all->brightness == 1);
                    CHECK(all->red == 9);
                    CHECK(all->green == 9);
                    CHECK(all->blue == 9);
                }
            }

            // The whitelisted All zone itself still writes normally.
            CHECK(client.write(Zone::All, want));
            CHECK_STR(slurp(allDir + "/multi_intensity"), "1 2 3\n");

            // ...and read(Zone::All) reflects that write (All is untouched by
            // the L1 hardening).
            {
                const auto all = client.read(Zone::All);
                CHECK(all.has_value());
                if (all) {
                    CHECK(all->effect == Effect::Breathing);
                    CHECK(all->brightness == 7);
                    CHECK(all->red == 1);
                    CHECK(all->green == 2);
                    CHECK(all->blue == 3);
                }
            }
        }

        // A client pointed at a non-existent base path discovers nothing.
        RgbClient empty(tmp.path() + QStringLiteral("/does-not-exist"));
        CHECK(!empty.discover());
        CHECK(!empty.available());
    }

    // =========================================================================
    // app/rgbclient.* — PARTIAL WRITE (T2).
    //
    // write() rewrites THREE sysfs attributes in a fixed order:
    //     multi_intensity  ->  brightness  ->  effect
    // and returns true only when every one of them landed. It is NEITHER atomic
    // NOR transactional: on the first failure it returns immediately and leaves
    // whatever earlier attributes had already succeeded in place. There is NO
    // rollback guarantee -- a false return does NOT mean "nothing changed".
    //
    // The tests below make exactly one attribute fail at a time and record, for
    // every case, which attributes the device actually kept, so the real effect
    // of a partial write is measured instead of assumed. Two independent failure
    // mechanisms are used: (1) clearing the write bit (0444) so
    // open(WriteOnly|Truncate) fails with EACCES, exactly as it would on a
    // root-owned sysfs attribute, and (2) replacing the attribute with a
    // DIRECTORY so the open fails for a different reason.
    //
    // Everything happens inside a QTemporaryDir. The real /sys tree is never
    // touched, and no privilege is escalated.
    // =========================================================================
    {
        using namespace Lighting;

        const auto putRaw = [](const QString &path, const char *value) {
            QFile f(path);
            const QByteArray bytes(value);
            return f.open(QIODevice::WriteOnly | QIODevice::Truncate)
                && f.write(bytes) == bytes.size();
        };
        const auto slurp = [](const QString &path) {
            QFile f(path);
            if (!f.open(QIODevice::ReadOnly))
                return QStringLiteral("<unreadable>");
            return QString::fromUtf8(f.readAll());
        };
        const auto setWritable = [](const QString &path) {
            return QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner
                                                   | QFile::ReadUser | QFile::WriteUser
                                                   | QFile::ReadGroup | QFile::ReadOther);
        };
        const auto setReadOnly = [](const QString &path) {
            return QFile::setPermissions(path, QFile::ReadOwner | QFile::ReadUser
                                                   | QFile::ReadGroup | QFile::ReadOther);
        };

        // Attribute names in WRITE order.
        const QString attr[3] = {QStringLiteral("multi_intensity"),
                                 QStringLiteral("brightness"), QStringLiteral("effect")};

        // The requested composition differs from the mock's starting values in
        // EVERY field, so each attribute that lands is visibly a change.
        const LedData want = [] {
            LedData d;
            d.effect = Effect::Breathing;
            d.brightness = 7;
            d.red = 1;
            d.green = 2;
            d.blue = 3;
            return d;
        }();

        struct Outcome {
            bool blocked = false;  // the induced block was actually in effect
            bool ok = false;       // write()'s return value
            QString before[3];
            QString after[3];
            bool hasReadBack = false;
            LedData readBack{};
        };

        // failIndex: 0 = multi_intensity, 1 = brightness, 2 = effect.
        // asDirectory: induce failure by replacing the attribute with a dir.
        const auto runCase = [&](int failIndex, bool asDirectory) -> Outcome {
            Outcome r;
            QTemporaryDir dir;
            if (!dir.isValid())
                return r;
            const QString zoneDir = dir.path() + QStringLiteral("/excalibur::zone-a");
            QDir().mkpath(zoneDir);
            putRaw(zoneDir + "/multi_intensity", "10 20 30\n");
            putRaw(zoneDir + "/brightness", "5\n");
            putRaw(zoneDir + "/effect", "off\n");

            if (failIndex >= 0) {
                const QString target = zoneDir + QLatin1Char('/') + attr[failIndex];
                if (asDirectory) {
                    QFile::remove(target);
                    QDir().mkpath(target);
                    r.blocked = QFileInfo(target).isDir();
                } else {
                    setReadOnly(target);
                    r.blocked = !QFileInfo(target).isWritable();
                }
            }

            for (int i = 0; i < 3; ++i)
                r.before[i] = slurp(zoneDir + QLatin1Char('/') + attr[i]);

            RgbClient client(dir.path());
            client.discover();
            client.setHardwareWriteEnabled(true); // gate open for this test only
            r.ok = client.write(Zone::ZoneA, want);

            for (int i = 0; i < 3; ++i)
                r.after[i] = slurp(zoneDir + QLatin1Char('/') + attr[i]);

            if (const auto back = client.read(Zone::ZoneA)) {
                r.hasReadBack = true;
                r.readBack = *back;
                setWritable(zoneDir + "/multi_intensity"); // keep the tmp tree sane
            }
            return r;
        };

        // --- control: with every attribute writable the write is complete ----
        {
            const Outcome o = runCase(-1, false);
            CHECK(o.ok);
            CHECK_STR(o.after[0], "1 2 3\n");
            CHECK_STR(o.after[1], "7\n");
            CHECK_STR(o.after[2], "breathing\n");
            CHECK(o.hasReadBack);
            CHECK(o.readBack.effect == Effect::Breathing);
            CHECK(o.readBack.brightness == 7);
            CHECK(o.readBack.red == 1 && o.readBack.green == 2 && o.readBack.blue == 3);
        }

        // --- case A: the FIRST attribute (multi_intensity) fails -------------
        // write() stops before touching anything else, so the zone is intact.
        {
            const Outcome o = runCase(0, false);
            CHECK(o.blocked);
            CHECK(!o.ok);
            CHECK(o.after[0] == o.before[0]);   // multi_intensity  unchanged
            CHECK(o.after[1] == o.before[1]);   // brightness       unchanged
            CHECK(o.after[2] == o.before[2]);   // effect           unchanged
            CHECK_STR(o.after[0], "10 20 30\n");
            CHECK_STR(o.after[1], "5\n");
            CHECK_STR(o.after[2], "off\n");
            CHECK(o.hasReadBack);
            CHECK(o.readBack.effect == Effect::Off);       // device still holds OLD state
            CHECK(o.readBack.brightness == 5);
            CHECK(o.readBack.red == 10);
        }

        // --- case B: the SECOND attribute (brightness) fails -----------------
        // multi_intensity has ALREADY landed -> the zone is left PARTIAL.
        // This is the case that makes a "values unchanged" message a lie.
        {
            const Outcome o = runCase(1, false);
            CHECK(o.blocked);
            CHECK(!o.ok);
            CHECK_STR(o.after[0], "1 2 3\n");    // landed BEFORE the failure
            CHECK(o.after[1] == o.before[1]);    // brightness refused  (still "5\n")
            CHECK(o.after[2] == o.before[2]);    // effect never attempted ("off\n")
            CHECK(o.hasReadBack);
            CHECK(o.readBack.red == 1);          // NEW intensity survived ...
            CHECK(o.readBack.green == 2);
            CHECK(o.readBack.blue == 3);
            CHECK(o.readBack.brightness == 5);   // ... while brightness stayed OLD
            CHECK(o.readBack.effect == Effect::Off); // ... and effect is still OLD
            // No rollback: the landed attribute was NOT restored to "10 20 30".
            CHECK_STR(o.after[0], "1 2 3\n");
        }

        // --- case C: the LAST attribute (effect) fails -----------------------
        // Both earlier attributes have landed -> the largest partial state.
        {
            const Outcome o = runCase(2, false);
            CHECK(o.blocked);
            CHECK(!o.ok);
            CHECK_STR(o.after[0], "1 2 3\n");    // multi_intensity  landed
            CHECK_STR(o.after[1], "7\n");        // brightness       landed
            CHECK(o.after[2] == o.before[2]);    // effect           refused
            CHECK(o.hasReadBack);
            CHECK(o.readBack.red == 1 && o.readBack.green == 2 && o.readBack.blue == 3);
            CHECK(o.readBack.brightness == 7);
            CHECK(o.readBack.effect == Effect::Off); // NOT Breathing -> PARTIAL
            // The two landed attributes are NOT rolled back.
            CHECK_STR(o.after[0], "1 2 3\n");
            CHECK_STR(o.after[1], "7\n");
        }

        // --- case D: a second, independent failure mechanism -----------------
        // The attribute is a DIRECTORY, not a read-only file. write() still
        // fails, and because it is the FIRST attribute nothing else is touched.
        {
            const Outcome o = runCase(0, true);
            CHECK(o.blocked);
            CHECK(!o.ok);
            CHECK(o.after[1] == o.before[1]);    // brightness unchanged ("5\n")
            CHECK(o.after[2] == o.before[2]);    // effect unchanged     ("off\n")
            CHECK_STR(o.after[1], "5\n");
            CHECK_STR(o.after[2], "off\n");
        }
    }

    // =========================================================================
    // app/contrast.* — WCAG 2.1 luminance / contrast / source-over compositing
    // Pure maths: no widget, no palette, no I/O.
    // =========================================================================
    {
        using Contrast::bestInk;
        using Contrast::compositeOver;
        using Contrast::ratio;
        using Contrast::relativeLuminance;

        // --- relative luminance: known reference points ----------------------
        CHECK_NEAR_EPS(relativeLuminance(QColor(Qt::black)), 0.0, 1e-9);
        CHECK_NEAR_EPS(relativeLuminance(QColor(Qt::white)), 1.0, 1e-6);
        // Primary channels use the documented weights (0.2126 / 0.7152 / 0.0722).
        CHECK_NEAR_EPS(relativeLuminance(QColor(255, 0, 0)), 0.2126, 1e-4);
        CHECK_NEAR_EPS(relativeLuminance(QColor(0, 255, 0)), 0.7152, 1e-4);
        CHECK_NEAR_EPS(relativeLuminance(QColor(0, 0, 255)), 0.0722, 1e-4);
        // Monotonic, and mid-grey sits below half.
        CHECK(relativeLuminance(QColor(128, 128, 128)) > 0.0);
        CHECK(relativeLuminance(QColor(128, 128, 128)) < 0.5);
        // Alpha is ignored by luminance (caller composites first).
        CHECK_NEAR_EPS(relativeLuminance(QColor(255, 255, 255, 40)),
                       relativeLuminance(QColor(255, 255, 255, 255)), 1e-12);

        // --- contrast ratio: extremes, symmetry, identity --------------------
        CHECK_NEAR_EPS(ratio(QColor(Qt::black), QColor(Qt::white)), 21.0, 1e-6);
        CHECK_NEAR_EPS(ratio(QColor(Qt::white), QColor(Qt::black)), 21.0, 1e-6);
        CHECK_NEAR_EPS(ratio(QColor(Qt::black), QColor(Qt::black)), 1.0, 1e-9);
        CHECK_NEAR_EPS(ratio(QColor(Qt::white), QColor(Qt::white)), 1.0, 1e-9);
        {
            const QColor a(0x33, 0x66, 0x99);
            const QColor b(0xEE, 0xEE, 0xEE);
            CHECK_NEAR_EPS(ratio(a, b), ratio(b, a), 1e-12);
            CHECK(ratio(a, b) >= 1.0 && ratio(a, b) <= 21.0);
        }

        // --- compositeOver: opaque and fully transparent edges ---------------
        const QColor opaqueRed(255, 0, 0, 255);
        const QColor clearRed(255, 0, 0, 0);
        {
            const QColor out = compositeOver(opaqueRed, QColor(0, 0, 255));
            CHECK(out == opaqueRed);                       // alpha 255 -> as-is
            CHECK(out.alpha() == 255);
        }
        {
            const QColor out = compositeOver(clearRed, QColor(0, 0, 255));
            CHECK(out.red() == 0 && out.green() == 0 && out.blue() == 255);
            CHECK(out.alpha() == 255);                     // result is always opaque
        }
        {
            // 50% white over black -> mid grey, still opaque.
            const QColor out = compositeOver(QColor(255, 255, 255, 128), QColor(Qt::black));
            CHECK(out.alpha() == 255);
            CHECK(out.red() >= 127 && out.red() <= 128);
            CHECK(out.red() == out.green() && out.green() == out.blue());
        }
        {
            // 10% white over a dark base ~ #282828.
            const QColor out = compositeOver(QColor(255, 255, 255, 26), QColor(16, 16, 16));
            CHECK(out.alpha() == 255);
            CHECK(out.red() == 40 && out.green() == 40 && out.blue() == 40);
        }

        // --- bestInk: dark theme -> light ink wins ---------------------------
        {
            const QColor surface(0x1A, 0x1A, 0x1A);
            const QColor dark(Qt::black);
            const QColor light(0xF5, 0xF5, 0xF5);
            const QColor ink = bestInk(surface, surface, dark, light);
            CHECK(ink == light);
            CHECK(ratio(ink, surface) >= 4.5);
        }

        // --- bestInk: light theme -> dark ink wins ---------------------------
        {
            const QColor surface(0xFA, 0xFA, 0xFA);
            const QColor dark(0x20, 0x20, 0x20);
            const QColor light(Qt::white);
            const QColor ink = bestInk(surface, surface, dark, light);
            CHECK(ink == dark);
            CHECK(ratio(ink, surface) >= 4.5);
        }

        // --- bestInk on a SEMI-TRANSPARENT fill (alpha < 255) ----------------
        // A translucent white wash over a dark base: the composited surface is
        // mid grey, where the DARK ink reads better. Judging the raw (uncomposited)
        // fill instead would have picked the light ink, so this pins the alpha
        // handling down.
        {
            const QColor translucentFill(255, 255, 255, 26);
            const QColor base(0x10, 0x10, 0x10);
            const QColor surface = compositeOver(translucentFill, base);
            const QColor dark(Qt::black);
            const QColor light(Qt::white);
            const QColor ink = bestInk(translucentFill, base, dark, light);
            CHECK(ratio(ink, surface) >= 4.5);
            CHECK(ratio(ink, compositeOver(translucentFill, base)) >= 4.5);
        }

        // --- bestInk: translucent BLACK wash over a WHITE base --------------
        // Composited surface is ~#7F7F7F. The dark ink wins on the real surface,
        // whereas ignoring the fill's alpha (treating it as opaque black) would
        // pick the light ink -- this is the discriminating case.
        {
            const QColor translucentFill(0, 0, 0, 128);
            const QColor base(Qt::white);
            const QColor surface = compositeOver(translucentFill, base);
            const QColor dark(Qt::black);
            const QColor light(Qt::white);
            const QColor ink = bestInk(translucentFill, base, dark, light);
            CHECK(ink == dark);
            CHECK(ratio(ink, surface) >= 4.5);
        }

        // --- bestInk: transparent fill -> the base decides -------------------
        {
            const QColor surface(0x0D, 0x11, 0x17);
            const QColor dark(0x11, 0x11, 0x11);
            const QColor light(0xEE, 0xEE, 0xEE);
            const QColor ink = bestInk(QColor(0, 0, 0, 0), surface, dark, light);
            CHECK(ink == light);
            CHECK(ratio(ink, surface) >= 4.5);
        }

        // --- bestInk: a translucent INK is judged by what is actually seen ---
        {
            const QColor surface(0xFA, 0xFA, 0xFA);
            const QColor dark(0x00, 0x00, 0x00, 200);   // mostly opaque dark ink
            const QColor light(0xFF, 0xFF, 0xFF, 40);   // barely-there light ink
            const QColor ink = bestInk(surface, surface, dark, light);
            CHECK(ink == dark);
            CHECK(ratio(compositeOver(ink, surface), surface) >= 4.5);
        }
    }

    // =========================================================================
    // contrast: gradient SPAN helpers -- the F3 fix infrastructure.
    // A label over a gradient has no single background colour; these helpers
    // judge an ink by its WORST point over the whole span (WCAG 1.4.3).
    // =========================================================================
    {
        using Contrast::bestInkForSpan;
        using Contrast::bestInk;
        using Contrast::compositeOver;
        using Contrast::ratio;
        using Contrast::worstRatioForInk;

        // The same tokens the Lighting schematic uses.
        const QColor kDarkInk(0x14, 0x17, 0x1C);
        const QColor kLightInk(0xE8, 0xEA, 0xED);
        const QColor kBlack(Qt::black);
        const QColor kWhite(Qt::white);
        constexpr double kAa = 4.5;

        // --- worstRatioForInk: empty span, single sample, alpha, minimum -----
        CHECK_NEAR_EPS(worstRatioForInk({}, QColor(Qt::black), kWhite), 1.0, 1e-9);
        {
            const QVector<QColor> one{QColor(0x20, 0x20, 0x20)};
            CHECK_NEAR_EPS(worstRatioForInk(one, QColor(Qt::white), kWhite),
                           ratio(QColor(0x20, 0x20, 0x20), kWhite), 1e-9);
        }
        {
            // A span's worst point can be far below any single stop: the black
            // end drags a white ink all the way down to 1:1, which is exactly
            // why judging "the brightest stop" (the old F3 code) is not enough.
            const QVector<QColor> span{QColor(Qt::black), QColor(Qt::white)};
            CHECK_NEAR_EPS(worstRatioForInk(span, QColor(Qt::white), kWhite), 1.0, 1e-9);
            CHECK_NEAR_EPS(worstRatioForInk(span, QColor(Qt::white), kBlack), 1.0, 1e-9);
        }
        {
            // A translucent sample is flattened onto the base before measuring.
            const QVector<QColor> span{QColor(255, 255, 255, 26)};
            const QColor base(0x10, 0x10, 0x10);
            CHECK_NEAR_EPS(worstRatioForInk(span, base, kWhite),
                           ratio(compositeOver(QColor(255, 255, 255, 26), base), kWhite), 1e-9);
        }

        // --- bestInkForSpan: picks by WORST case, not by one stop ------------
        {
            const QVector<QColor> span{QColor(0x1A, 0x1A, 0x1A), QColor(0x30, 0x30, 0x30)};
            const QColor table(0x1A, 0x1A, 0x1A); // opaque stops -> base unused
            CHECK(bestInkForSpan(span, table, kBlack, kWhite) == kWhite);
            CHECK(worstRatioForInk(span, table, kWhite) >= kAa);
        }
        {
            const QVector<QColor> span{QColor(Qt::white), QColor(0xFA, 0xFA, 0xFA)};
            CHECK(bestInkForSpan(span, QColor(Qt::white), kBlack, kWhite) == kBlack);
            CHECK(worstRatioForInk(span, QColor(Qt::white), kBlack) >= kAa);
        }
        {
            // bestInkForSpan must agree with bestInk on a single-colour span.
            const QVector<QColor> one{QColor(0x1A, 0x1A, 0x1A)};
            CHECK(bestInkForSpan(one, QColor(0x1A, 0x1A, 0x1A), kDarkInk, kLightInk)
                  == bestInk(QColor(0x1A, 0x1A, 0x1A), QColor(0x1A, 0x1A, 0x1A), kDarkInk,
                             kLightInk));
        }

        // --- The Lighting strip gradient, reproduced exactly ----------------
        // Same stops as Schematic::stripStops(): Ambilight paints
        // HSV(hue, 0.55, 0.95) -> HSV(hue + 0.08*(i+1), 0.55, 0.75). The label
        // sits centred, so its text box straddles a slice of the gradient. The
        // sample window [0.42, 0.58] is deliberately WIDER than the real glyph
        // box (~7% of the strip width), so the check below is conservative.
        const auto stripSpan = [](const QColor &base, int i) {
            const qreal hue = base.hsvHueF() < 0.0 ? 0.0 : base.hsvHueF();
            const QColor s0 = QColor::fromHsvF(hue, 0.55, 0.95);
            const QColor s1 = QColor::fromHsvF(std::fmod(hue + 0.08 * (i + 1), 1.0), 0.55, 0.75);
            QVector<QColor> samples;
            constexpr int kSteps = 12;
            for (int k = 0; k <= kSteps; ++k) {
                const qreal t = 0.42 + 0.16 * (static_cast<qreal>(k) / kSteps);
                samples << QColor::fromRgbF(s0.redF() + (s1.redF() - s0.redF()) * t,
                                            s0.greenF() + (s1.greenF() - s0.greenF()) * t,
                                            s0.blueF() + (s1.blueF() - s0.blueF()) * t);
            }
            return samples;
        };

        // The UI's ink policy, mirrored: design inks first, then pure
        // black/white, and only a badge (a solid plate) when neither covers the
        // span. Kept in one place so every scenario below uses the same rule.
        const auto resolveSpanInk = [&](const QVector<QColor> &span, const QColor &cardBase) {
            const QColor design = bestInkForSpan(span, cardBase, kDarkInk, kLightInk);
            if (worstRatioForInk(span, cardBase, design) >= kAa)
                return design;
            const QColor achromatic = bestInkForSpan(span, cardBase, kBlack, kWhite);
            if (worstRatioForInk(span, cardBase, achromatic) >= kAa)
                return achromatic;
            return QColor(); // null -> a badge is required (checked separately)
        };

        // --- F3 regression: Ambilight + user colour #000DFF -----------------
        // Nine points of the OLD shared ink failed: the single ink was chosen
        // from the union of ALL strips' stops, so it read well on one strip but
        // not on the others. Reproduce the old choice and show it breaks AA on
        // strip A, then show the per-strip choice clears it.
        for (Theme::Mode mode : {Theme::Mode::Dark, Theme::Mode::Light}) {
            Theme::setMode(mode);
            const QColor cardBase = Theme::surfaceElevated();
            const QColor user(0x00, 0x0D, 0xFF);
            // Ambilight uses only the hue; full brightness keeps base == user.
            const QColor base(user.red(), user.green(), user.blue());
            const qreal hue = base.hsvHueF() < 0.0 ? 0.0 : base.hsvHueF();

            // Old behaviour: brightest stop across ALL strips -> one ink.
            QVector<QColor> allStops;
            for (int i = 0; i < 3; ++i) {
                allStops << QColor::fromHsvF(hue, 0.55, 0.95)
                         << QColor::fromHsvF(std::fmod(hue + 0.08 * (i + 1), 1.0), 0.55, 0.75);
            }
            QColor brightest = allStops.first();
            for (const QColor &c : allStops)
                if (Contrast::relativeLuminance(c) > Contrast::relativeLuminance(brightest))
                    brightest = c;
            const QColor oldInk = bestInkForSpan(QVector<QColor>{brightest}, cardBase, kDarkInk,
                                                 kLightInk);
            CHECK(oldInk == kDarkInk);
            CHECK(worstRatioForInk(stripSpan(base, 0), cardBase, oldInk) < kAa); // the bug

            // Pin the defect report's numbers: the old single ink at each
            // strip's CENTRE reads ~3.98 / 4.34 / 4.24 -- all below AA.
            const auto centre = [&](int i) {
                const QColor s0 = QColor::fromHsvF(hue, 0.55, 0.95);
                const QColor s1 =
                    QColor::fromHsvF(std::fmod(hue + 0.08 * (i + 1), 1.0), 0.55, 0.75);
                return QColor::fromRgbF((s0.redF() + s1.redF()) / 2.0,
                                        (s0.greenF() + s1.greenF()) / 2.0,
                                        (s0.blueF() + s1.blueF()) / 2.0);
            };
            CHECK(ratio(oldInk, centre(0)) < kAa);
            CHECK_NEAR_EPS(ratio(oldInk, centre(0)), 3.985, 0.02);
            CHECK_NEAR_EPS(ratio(oldInk, centre(1)), 4.335, 0.02);
            CHECK_NEAR_EPS(ratio(oldInk, centre(2)), 4.241, 0.02);

            // New behaviour: each strip judged on its own gradient slice.
            int badges = 0;
            for (int i = 0; i < 3; ++i) {
                const QVector<QColor> span = stripSpan(base, i);
                const QColor ink = resolveSpanInk(span, cardBase);
                if (ink.isValid())
                    CHECK(worstRatioForInk(span, cardBase, ink) >= kAa);
                else
                    ++badges;
            }
            // For #000DFF a per-strip achromatic ink is enough: no badges needed.
            CHECK(badges == 0);
        }
        Theme::setMode(Theme::Mode::Dark); // restore the default

        // --- Colorful cycle regression --------------------------------------
        // Cycle paints four fixed stops across every strip (independent of the
        // user colour). The label box is ~3% of the strip, where the original
        // design ink still clears AA -- so the look is unchanged. Over a much
        // wider slice the achromatic fallback still keeps AA, so the fix never
        // regresses below the threshold.
        {
            const auto cycleSpan = [](qreal halfWidth) {
                const QColor s0 = QColor::fromHsvF(0.00, 0.70, 0.90);
                const QColor s1 = QColor::fromHsvF(0.33, 0.70, 0.90);
                const QColor s2 = QColor::fromHsvF(0.66, 0.70, 0.90);
                const QColor s3 = QColor::fromHsvF(0.95, 0.70, 0.90);
                const auto at = [&](qreal t) {
                    const QVector<QPair<qreal, QColor>> stops{{0.0, s0}, {0.33, s1}, {0.66, s2},
                                                              {1.0, s3}};
                    for (int k = 1; k < stops.size(); ++k) {
                        if (t <= stops.at(k).first) {
                            const qreal t0 = stops.at(k - 1).first;
                            const qreal t1 = stops.at(k).first;
                            const QColor &c0 = stops.at(k - 1).second;
                            const QColor &c1 = stops.at(k).second;
                            const qreal f = (t - t0) / (t1 - t0);
                            return QColor::fromRgbF(c0.redF() + (c1.redF() - c0.redF()) * f,
                                                    c0.greenF() + (c1.greenF() - c0.greenF()) * f,
                                                    c0.blueF() + (c1.blueF() - c0.blueF()) * f);
                        }
                    }
                    return s3;
                };
                QVector<QColor> samples;
                constexpr int kSteps = 15;
                for (int k = 0; k <= kSteps; ++k)
                    samples << at(0.5 - halfWidth + 2.0 * halfWidth * (k / static_cast<qreal>(kSteps)));
                return samples;
            };

            for (Theme::Mode mode : {Theme::Mode::Dark, Theme::Mode::Light}) {
                Theme::setMode(mode);
                const QColor cardBase = Theme::surfaceElevated();

                // Real glyph box (~±3%): the design ink is preserved.
                const QVector<QColor> real = cycleSpan(0.031);
                const QColor design = bestInkForSpan(real, cardBase, kDarkInk, kLightInk);
                CHECK(design == kDarkInk);
                CHECK(worstRatioForInk(real, cardBase, design) >= kAa);

                // Conservative wide slice (±8%): no single ink covers it, so the
                // UI would draw a badge -- and the badge surface still clears AA
                // (this is the badge path the F3 fix relies on).
                const QVector<QColor> wide = cycleSpan(0.08);
                CHECK(!resolveSpanInk(wide, cardBase).isValid());
                const QColor badge = Theme::surface();
                const QColor badgeInk = bestInk(badge, cardBase, kDarkInk, kLightInk);
                CHECK(ratio(compositeOver(badgeInk, badge), badge) >= kAa);
            }
            Theme::setMode(Theme::Mode::Dark);
        }

        // --- Theme / LED on-off / brightness ends (solid fills) -------------
        // representativeColor() for an on LED, and the dimmed OFF fill, judged
        // with the same resolve path used by inkOver().
        const auto resolveSolidInk = [&](const QColor &fill, const QColor &base) {
            const QColor surface = compositeOver(fill, base);
            const QColor design = bestInk(fill, base, kDarkInk, kLightInk);
            if (ratio(compositeOver(design, surface), surface) >= kAa)
                return design;
            return ratio(kBlack, surface) >= ratio(kWhite, surface) ? kBlack : kWhite;
        };

        for (Theme::Mode mode : {Theme::Mode::Dark, Theme::Mode::Light}) {
            Theme::setMode(mode);
            const QColor cardBase = Theme::surfaceElevated();

            // OFF fill: textMuted at alpha 90 (Schematic::representativeColor).
            QColor offFill = Theme::textMuted();
            offFill.setAlpha(90);
            {
                const QColor surface = compositeOver(offFill, cardBase);
                const QColor ink = resolveSolidInk(offFill, cardBase);
                CHECK(ratio(compositeOver(ink, surface), surface) >= kAa);
            }

            // Static red #FF4D3D (the app default) at mid and end brightness.
            for (int brightness : {0, 8, 15}) {
                const qreal f = 0.30 + 0.70 * (static_cast<qreal>(brightness) / 15.0);
                const QColor fill = QColor::fromRgbF(
                    qBound(0.0, QColor(0xFF, 0x4D, 0x3D).redF() * f, 1.0),
                    qBound(0.0, QColor(0xFF, 0x4D, 0x3D).greenF() * f, 1.0),
                    qBound(0.0, QColor(0xFF, 0x4D, 0x3D).blueF() * f, 1.0));
                const QColor surface = compositeOver(fill, cardBase);
                const QColor ink = resolveSolidInk(fill, cardBase);
                CHECK(ratio(compositeOver(ink, surface), surface) >= kAa);
            }

            // Static #000DFF at the brightness ENDS (0 and 15).
            for (int brightness : {0, 15}) {
                const qreal f = 0.30 + 0.70 * (static_cast<qreal>(brightness) / 15.0);
                const QColor fill = QColor::fromRgbF(0.0, qBound(0.0, 13.0 / 255.0 * f, 1.0),
                                                     qBound(0.0, 1.0 * f, 1.0));
                const QColor ink = resolveSolidInk(fill, cardBase);
                CHECK(ratio(compositeOver(ink, compositeOver(fill, cardBase)),
                            compositeOver(fill, cardBase)) >= kAa);
            }
        }
        Theme::setMode(Theme::Mode::Dark);

        // --- Badge path: a span no single ink can cover ---------------------
        // A black->white gradient cannot be served by any one ink, so the UI
        // must fall back to a solid badge plate. Prove the span defeats both
        // the design inks and the achromatic pair, and that the badge surface
        // the UI paints (Theme::surface()) yields a compliant ink.
        {
            const QVector<QColor> hardSpan{QColor(Qt::black), QColor(Qt::white)};
            for (Theme::Mode mode : {Theme::Mode::Dark, Theme::Mode::Light}) {
                Theme::setMode(mode);
                const QColor cardBase = Theme::surfaceElevated();
                const QColor design = bestInkForSpan(hardSpan, cardBase, kDarkInk, kLightInk);
                const QColor achromatic = bestInkForSpan(hardSpan, cardBase, kBlack, kWhite);
                CHECK(worstRatioForInk(hardSpan, cardBase, design) < kAa);
                CHECK(worstRatioForInk(hardSpan, cardBase, achromatic) < kAa);

                // The badge is opaque Theme::surface(); inkOver() would then see
                // a single, gradient-free surface.
                const QColor badge = Theme::surface();
                const QColor badgeInk = bestInk(badge, cardBase, kDarkInk, kLightInk);
                CHECK(ratio(compositeOver(badgeInk, badge), badge) >= kAa);
            }
            Theme::setMode(Theme::Mode::Dark);
        }
    }

    if (g_failures == 0) {
        std::printf("all tests passed\n");
        return 0;
    }
    std::fprintf(stderr, "%d check(s) failed\n", g_failures);
    return 1;
}
