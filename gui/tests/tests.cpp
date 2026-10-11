// Unit tests for the pure logic that does not need a display or hardware:
//   * temperature unit conversion / formatting (util/units.h)
//   * user-settings parsing and validation (app/usersettings.*)
//
// No Qt event loop, no window, no sysfs access. Exit code 0 = all pass.
//
//   cmake -S gui -B build -DEXCALIBUR_BUILD_TESTS=ON && cmake --build build
//   ./build/excalibur-tests          (or: ctest --test-dir build)

#include <cmath>
#include <cstdio>

#include "app/usersettings.h"
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

} // namespace

#define CHECK(cond) check((cond), #cond, __LINE__)
#define CHECK_NEAR(got, want) checkNear((got), (want), 1e-9, #got " == " #want, __LINE__)
#define CHECK_STR(got, want) checkStr((got), (want), __LINE__)

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

    if (g_failures == 0) {
        std::printf("all tests passed\n");
        return 0;
    }
    std::fprintf(stderr, "%d check(s) failed\n", g_failures);
    return 1;
}
