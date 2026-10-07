#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QIcon>
#include <QLockFile>
#include <QPalette>
#include <QPixmap>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QThread>
#include <QTimer>
#include <QtGlobal>

#include <cstdio>

#include "app/identity.h"
#include "app/diagnostics.h"
#include "app/telemetrymodel.h"
#include "app/usersettings.h"
#include "app/version.h"
#include "hwmonclient.h"
#include "mainwindow.h"
#include "navigation/pagecontainer.h"
#include "theme/theme.h"
#include "util/units.h"

namespace {

// Build the multi-resolution window/tray icon from the embedded asset set so the
// application never depends on an installed theme or an absolute path.
QIcon loadAppIcon()
{
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128, 256}) {
        icon.addFile(QStringLiteral(":/icons/excalibur-control-center-%1.png").arg(size),
                     QSize(size, size));
    }
    return icon;
}

#ifdef EXCALIBUR_DEV_HOOKS
// Default data source; overridable only for testing (no hardware impact).
QString hwmonRoot()
{
    return qEnvironmentVariable("EXCALIBUR_HWMON_ROOT", QStringLiteral("/sys/class/hwmon"));
}

// Headless one-shot probe: prints the status + values and exits.
int runOnce()
{
    HwmonClient client(hwmonRoot());
    client.discover();
    const ExcTelemetry t = client.read();
    const ExcStatus st = client.status(t);

    const char *state = (st == ExcStatus::Connected)    ? "CONNECTED"
                        : (st == ExcStatus::Degraded)   ? "DEGRADED"
                                                        : "DISCONNECTED";

    std::printf("state=%s\n", state);
    std::printf("hwmon=%s\n", client.devicePath().toUtf8().constData());
    if (t.cpuTempValid)
        std::printf("cpu_temp_c=%s\n", QString::number(t.cpuTempC, 'f', 1).toUtf8().constData());
    if (t.gpuTempValid)
        std::printf("gpu_temp_c=%s\n", QString::number(t.gpuTempC, 'f', 1).toUtf8().constData());
    if (t.cpuFanValid)
        std::printf("cpu_fan_rpm=%ld\n", t.cpuFanRpm);
    if (t.gpuFanValid)
        std::printf("gpu_fan_rpm=%ld\n", t.gpuFanRpm);
    return 0;
}
#endif // EXCALIBUR_DEV_HOOKS

bool hasFlag(const QStringList &args, const QString &flag)
{
    return args.contains(flag);
}

#ifdef EXCALIBUR_DEV_HOOKS
QString flagValue(const QStringList &args, const QString &flag)
{
    const int i = args.indexOf(flag);
    return (i >= 0 && i + 1 < args.size()) ? args.at(i + 1) : QString();
}

// Dev-only: render a window to PNG. Uses the offscreen platform when no display
// is present, so it never requires a session and never touches hardware.
int runScreenshot(const QStringList &args, const QString &path)
{
    qputenv("EXCALIBUR_NO_ANIM", "1");

    MainWindow window;
    const QString size = flagValue(args, QStringLiteral("--size"));
    if (!size.isEmpty()) {
        const QStringList wh = size.split(QLatin1Char('x'));
        if (wh.size() == 2)
            window.resize(wh.at(0).toInt(), wh.at(1).toInt());
    }
    const QString page = flagValue(args, QStringLiteral("--page"));
    if (!page.isEmpty())
        window.showPage(page.toInt());

    window.show();
    QApplication::processEvents();

    // Optional: let the telemetry model run for N seconds first so the history
    // charts have real samples. Dev/validation only; never touches hardware.
    const QString delay = flagValue(args, QStringLiteral("--shot-delay"));
    if (!delay.isEmpty()) {
        const int seconds = delay.toInt();
        QElapsedTimer elapsed;
        elapsed.start();
        while (elapsed.elapsed() < seconds * 1000) {
            QApplication::processEvents(QEventLoop::AllEvents, 100);
            QThread::msleep(20);
        }
    }

    QApplication::processEvents();

    const QPixmap pm = window.grab();
    if (!pm.save(path)) {
        std::fprintf(stderr, "screenshot save failed: %s\n", path.toUtf8().constData());
        return 1;
    }
    std::printf("screenshot=%s (%dx%d)\n", path.toUtf8().constData(), pm.width(), pm.height());
    return 0;
}
#endif // EXCALIBUR_DEV_HOOKS

#ifdef EXCALIBUR_DEV_HOOKS
// Dev-only: run the telemetry model headlessly and print one line per poll so
// runtime state transitions can be observed. Uses the same read-only hwmon path
// the GUI uses.
int runTrace(const QStringList &args)
{
    const QString secs = flagValue(args, QStringLiteral("--trace-seconds"));
    const int seconds = secs.isEmpty() ? 20 : secs.toInt();

    TelemetryModel model;
    QObject::connect(
        &model, &TelemetryModel::updated, &model,
        [](const TelemetrySnapshot &s, const QVector<HistorySample> &) {
            const char *st = (s.status == AppState::Status::Connected)  ? "CONNECTED"
                             : (s.status == AppState::Status::Degraded) ? "DEGRADED"
                                                                        : "DISCONNECTED";
            const QByteArray fan1 = s.cpuFanValid ? QByteArray::number(s.cpuFanRpm) : QByteArray("na");
            const QByteArray fan2 = s.gpuFanValid ? QByteArray::number(s.gpuFanRpm) : QByteArray("na");

            // Raw hwmon source, printed next to the parsed value, so the chain
            // raw file -> HwmonClient -> snapshot -> UI is traceable per sample.
            const auto raw = [&s](const char *file) -> QByteArray {
                if (s.hwmonPath.isEmpty())
                    return QByteArray("na");
                QFile f(s.hwmonPath + QLatin1Char('/') + QLatin1String(file));
                if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
                    return QByteArray("na");
                return f.readAll().trimmed();
            };
            std::printf("state=%-12s hwmon=%s raw_t1=%s cpu=%.1f%s raw_t2=%s gpu=%.1f%s fan1=%s fan2=%s\n",
                        st, s.hwmonPath.toUtf8().constData(),
                        raw("temp1_input").constData(),
                        s.cpuTempC, s.cpuTempValid ? "" : "(na)",
                        raw("temp2_input").constData(),
                        s.gpuTempC, s.gpuTempValid ? "" : "(na)",
                        fan1.constData(), fan2.constData());
            std::fflush(stdout);
        });
    model.start();
    QTimer::singleShot(seconds * 1000, qApp, &QCoreApplication::quit);
    return qApp->exec();
}
#endif // EXCALIBUR_DEV_HOOKS

#ifdef EXCALIBUR_DEV_HOOKS
// Dev-only: exercise the runtime settings paths (unit, refresh, theme) through the
// real signals/slots, then grab a frame. Proves no-restart application works.
int runRuntimeExercise(const QStringList &args)
{
    qputenv("EXCALIBUR_NO_ANIM", "1");

    MainWindow window;
    const QString size = flagValue(args, QStringLiteral("--size"));
    if (!size.isEmpty()) {
        const QStringList wh = size.split(QLatin1Char('x'));
        if (wh.size() == 2)
            window.resize(wh.at(0).toInt(), wh.at(1).toInt());
    }
    window.show();
    QApplication::processEvents();

    UserSettings &us = UserSettings::instance();
    QTimer::singleShot(300, &window, [&us]() {
        us.setTempUnit(Units::TemperatureUnit::Fahrenheit);
        us.setRefreshMs(5000);
        us.setTheme(UserSettings::AppTheme::Light);
    });

    const QString out = flagValue(args, QStringLiteral("--screenshot"));
    QTimer::singleShot(1400, &window, [&window, out]() {
        if (!out.isEmpty()) {
            if (window.grab().save(out))
                std::printf("runtime-exercise screenshot=%s\n", out.toUtf8().constData());
        }
        std::printf("runtime-exercise ok unit=%d theme=%d refresh=%d\n",
                    int(UserSettings::instance().tempUnit()),
                    int(UserSettings::instance().theme()),
                    UserSettings::instance().refreshMs());
        std::fflush(stdout);
        qApp->quit();
    });
    return qApp->exec();
}
#endif // EXCALIBUR_DEV_HOOKS

#ifdef EXCALIBUR_DEV_HOOKS
// Dev-only: build a diagnostics snapshot from one poll and write the JSON, so the
// export path can be validated headlessly (no file dialog, no display).
int runExportDiagnostics(const QStringList &args)
{
    const QString path = flagValue(args, QStringLiteral("--export-diagnostics"));
    if (path.isEmpty())
        return 2;

    TelemetryModel model;
    const int delay = flagValue(args, QStringLiteral("--export-delay")).toInt();
    if (delay > 0) {
        model.start(500);
        QElapsedTimer t;
        t.start();
        while (t.elapsed() < delay * 1000) {
            QApplication::processEvents(QEventLoop::AllEvents, 50);
            QThread::msleep(10);
        }
    } else {
        model.pollNow();
    }

    DiagnosticsContext ctx;
    const DiagnosticsSnapshot d =
        buildDiagnostics(model.snapshot(), model.history(), readSystemInfo(), ctx);

    QString error;
    if (!saveDiagnostics(d, path, &error)) {
        std::fprintf(stderr, "export failed: %s\n", error.toUtf8().constData());
        return 1;
    }
    std::printf("diagnostics=%s state=%s samples=%d\n",
                path.toUtf8().constData(), d.state.toUtf8().constData(),
                int(d.history.size()));
    std::fflush(stdout);
    return 0;
}
#endif // EXCALIBUR_DEV_HOOKS

#ifdef EXCALIBUR_DEV_HOOKS
// Dev-only: stress hide/show + navigation many times and report whether any
// duplicate telemetry owner / timer / tray / page was created.
int runTrayStress(const QStringList &args)
{
    qputenv("EXCALIBUR_NO_ANIM", "1");

    MainWindow window;
    window.resize(1100, 700);
    window.show();
    QApplication::processEvents();

    TelemetryModel *model = window.findChild<TelemetryModel *>();
    int cycles = 20;
    if (const QString c = flagValue(args, QStringLiteral("--cycles")); !c.isEmpty())
        cycles = c.toInt();

    for (int i = 0; i < cycles; ++i) {
        window.showPage(i % 6); // navigate across all pages
        window.hide();          // hide (instant)
        QApplication::processEvents();
        window.show();          // show (instant)
        QApplication::processEvents();
    }

    // Let a couple of telemetry ticks happen while looping.
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < 1200) {
        QApplication::processEvents(QEventLoop::AllEvents, 50);
        QThread::msleep(10);
    }

    const int models = window.findChildren<TelemetryModel *>().size();
    const int timers = model ? model->findChildren<QTimer *>().size() : -1;
    const int trays = window.findChildren<QSystemTrayIcon *>().size();
    PageContainer *pc = window.findChild<PageContainer *>();
    const int pages = pc ? pc->count() : -1;
    std::printf("tray-stress cycles=%d models=%d timers=%d trays=%d pages=%d visible=%d page=%d\n",
                cycles, models, timers, trays, pages, window.isVisible() ? 1 : 0,
                pc ? pc->currentIndex() : -1);
    std::fflush(stdout);
    return 0;
}
#endif // EXCALIBUR_DEV_HOOKS

} // namespace

int main(int argc, char **argv)
{
    QElapsedTimer startup;
    startup.start();

    QApplication app(argc, argv);
    app.setApplicationName(AppIdentity::desktopId());
    app.setApplicationDisplayName(AppIdentity::displayName());
    app.setOrganizationName(AppIdentity::organization());
    app.setApplicationVersion(QString::fromLatin1(AppVersion::string()));
    // Lets the compositor/Wayland and KDE associate the window with its .desktop
    // entry, so the launcher icon and task grouping are correct.
    app.setDesktopFileName(AppIdentity::desktopId());
    app.setWindowIcon(loadAppIcon());

    const QStringList args = app.arguments();

    // Session theme overrides for convenience/dev (not persisted); otherwise the
    // stored preference (including "System") drives the theme.
    if (hasFlag(args, QStringLiteral("--light")))
        UserSettings::instance().setTheme(UserSettings::AppTheme::Light, false);
    else if (hasFlag(args, QStringLiteral("--dark")))
        UserSettings::instance().setTheme(UserSettings::AppTheme::Dark, false);

#ifdef EXCALIBUR_DEV_HOOKS
    if (hasFlag(args, QStringLiteral("--tray-stress")))
        return runTrayStress(args);

    if (!flagValue(args, QStringLiteral("--export-diagnostics")).isEmpty())
        return runExportDiagnostics(args);

    if (hasFlag(args, QStringLiteral("--exercise-runtime")))
        return runRuntimeExercise(args);

    if (hasFlag(args, QStringLiteral("--once")))
        return runOnce();

    if (const QString path = flagValue(args, QStringLiteral("--screenshot")); !path.isEmpty())
        return runScreenshot(args, path);

    if (hasFlag(args, QStringLiteral("--trace")))
        return runTrace(args);

    // Headless settings persistence hooks (dev only).
    if (const QString v = flagValue(args, QStringLiteral("--set-theme")); !v.isEmpty()) {
        UserSettings::instance().setTheme(UserSettings::themeFromString(v));
        return 0;
    }
    if (const QString v = flagValue(args, QStringLiteral("--set-unit")); !v.isEmpty()) {
        UserSettings::instance().setTempUnit(UserSettings::tempUnitFromString(v));
        return 0;
    }
    if (const QString v = flagValue(args, QStringLiteral("--set-refresh")); !v.isEmpty()) {
        UserSettings::instance().setRefreshMs(v.toInt());
        return 0;
    }
    if (hasFlag(args, QStringLiteral("--reset-settings"))) {
        UserSettings::instance().resetToDefaults();
        return 0;
    }
    if (hasFlag(args, QStringLiteral("--dump-settings"))) {
        UserSettings &us = UserSettings::instance();
        std::printf("theme=%s\n", UserSettings::themeToString(us.theme()).toUtf8().constData());
        std::printf("unit=%s\n", UserSettings::tempUnitToString(us.tempUnit()).toUtf8().constData());
        std::printf("refresh_ms=%d\n", us.refreshMs());
        return 0;
    }
#endif

    // Single instance: never run two windows / two telemetry owners / two timers /
    // two tray icons. A QLockFile (PID-aware, self-healing after a crash) in the
    // per-user runtime dir is enough; no IPC/daemon machinery.
    QString lockDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (lockDir.isEmpty())
        lockDir = QDir::tempPath();
    QLockFile instanceLock(lockDir + QStringLiteral("/excalibur-control-center.lock"));
    if (!instanceLock.tryLock(100)) {
        std::fprintf(stderr, "EXCALIBUR Control Center is already running.\n");
        return 0;
    }

    MainWindow window;
    window.show();
    QApplication::processEvents();
#ifdef EXCALIBUR_DEV_HOOKS
    std::fprintf(stderr, "first_frame_ms=%lld\n", static_cast<long long>(startup.elapsed()));
#endif
    window.playIntro();
    return app.exec();
}
