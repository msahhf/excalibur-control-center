#include <QApplication>
#include <QPixmap>
#include <QtGlobal>

#include <cstdio>

#include "hwmonclient.h"
#include "mainwindow.h"

namespace {

// Optional dark test palette (dev-only, via --dark). The UI itself is fully
// palette-driven; this only lets us verify the dark rendering headlessly.
void applyDarkPalette(QApplication &app)
{
    QPalette p;
    const QColor window("#1b1e22");
    const QColor text("#e6e6e6");
    const QColor dim("#8a9099");
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, QColor("#141619"));
    p.setColor(QPalette::AlternateBase, QColor("#20242a"));
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, QColor("#20242a"));
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::Mid, QColor("#3a4048"));
    p.setColor(QPalette::Highlight, QColor("#c8102e"));
    p.setColor(QPalette::Disabled, QPalette::WindowText, dim);
    app.setPalette(p);
}

// Default data source; overridable only for testing (no hardware impact).
QString hwmonRoot()
{
    return qEnvironmentVariable("EXCALIBUR_HWMON_ROOT", QStringLiteral("/sys/class/hwmon"));
}

// Headless one-shot probe: prints the status + values and exits.
// Useful for automated validation without a display.
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
    // QString::number is locale-independent (always '.'), unlike printf("%f").
    if (t.cpuTempValid)
        std::printf("cpu_temp_c=%s\n",
                    QString::number(t.cpuTempC, 'f', 1).toUtf8().constData());
    if (t.gpuTempValid)
        std::printf("gpu_temp_c=%s\n",
                    QString::number(t.gpuTempC, 'f', 1).toUtf8().constData());
    if (t.cpuFanValid)
        std::printf("cpu_fan_rpm=%ld\n", t.cpuFanRpm);
    if (t.gpuFanValid)
        std::printf("gpu_fan_rpm=%ld\n", t.gpuFanRpm);
    return 0;
}

// Dev-only: render the window headlessly to a PNG (offscreen platform).
// Does not touch hardware; used to visually verify the layout in CI/automation.
int runScreenshot(const QString &path)
{
    MainWindow window;
    window.resize(1100, 700);
    window.show();
    QApplication::processEvents();
    QApplication::processEvents();

    const QPixmap pm = window.grab();
    if (!pm.save(path)) {
        std::fprintf(stderr, "screenshot save failed: %s\n", path.toUtf8().constData());
        return 1;
    }
    std::printf("screenshot=%s (%dx%d)\n", path.toUtf8().constData(), pm.width(), pm.height());
    return 0;
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("excalibur-control-center"));
    app.setApplicationDisplayName(QStringLiteral("EXCALIBUR Control Center"));

    const QStringList args = app.arguments();

    if (args.contains(QStringLiteral("--dark")))
        applyDarkPalette(app);

    if (args.contains(QStringLiteral("--once")))
        return runOnce();

    const int shotIdx = args.indexOf(QStringLiteral("--screenshot"));
    if (shotIdx >= 0 && shotIdx + 1 < args.size())
        return runScreenshot(args.at(shotIdx + 1));

    MainWindow window;
    window.show();
    return app.exec();
}
