#include <QApplication>
#include <QElapsedTimer>
#include <QGraphicsOpacityEffect>
#include <QPalette>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QThread>
#include <QtGlobal>

#include <cstdio>

#include "hwmonclient.h"
#include "mainwindow.h"
#include "theme/theme.h"

namespace {

// Build an application palette from the active theme tokens so native controls,
// tooltips, selection and menus match the product surface.
void applyThemePalette(QApplication &app)
{
    QPalette p;
    p.setColor(QPalette::Window, Theme::background());
    p.setColor(QPalette::WindowText, Theme::textPrimary());
    p.setColor(QPalette::Base, Theme::surface());
    p.setColor(QPalette::AlternateBase, Theme::surfaceElevated());
    p.setColor(QPalette::Text, Theme::textPrimary());
    p.setColor(QPalette::Button, Theme::surfaceElevated());
    p.setColor(QPalette::ButtonText, Theme::textPrimary());
    p.setColor(QPalette::ToolTipBase, Theme::surfaceElevated());
    p.setColor(QPalette::ToolTipText, Theme::textPrimary());
    p.setColor(QPalette::Mid, Theme::border());
    p.setColor(QPalette::Highlight, Theme::accent());
    p.setColor(QPalette::HighlightedText, Theme::textPrimary());
    p.setColor(QPalette::Disabled, QPalette::WindowText, Theme::textMuted());
    p.setColor(QPalette::Disabled, QPalette::Text, Theme::textMuted());
    app.setPalette(p);
}

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

bool hasFlag(const QStringList &args, const QString &flag)
{
    return args.contains(flag);
}

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

// Restrained app-start settle: a short content fade-in. Usability is never
// delayed (the window is interactive immediately).
void playStartupFade(MainWindow &window)
{
    if (!qEnvironmentVariableIsEmpty("EXCALIBUR_NO_ANIM"))
        return;
    auto *effect = new QGraphicsOpacityEffect(window.centralWidget());
    window.centralWidget()->setGraphicsEffect(effect);
    auto *anim = new QPropertyAnimation(effect, "opacity", &window);
    anim->setDuration(220);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("excalibur-control-center"));
    app.setApplicationDisplayName(QStringLiteral("EXCALIBUR Control Center"));

    const QStringList args = app.arguments();

    Theme::setMode(hasFlag(args, QStringLiteral("--light")) ? Theme::Mode::Light : Theme::Mode::Dark);
    applyThemePalette(app);

    if (hasFlag(args, QStringLiteral("--once")))
        return runOnce();

    if (const QString path = flagValue(args, QStringLiteral("--screenshot")); !path.isEmpty())
        return runScreenshot(args, path);

    MainWindow window;
    window.show();
    playStartupFade(window);
    return app.exec();
}
