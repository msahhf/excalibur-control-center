#include "dashboardpage.h"

#include "app/usersettings.h"
#include "components/emptystate.h"
#include "components/hardwaremodule.h"
#include "components/herostatus.h"
#include "components/thermalpanel.h"
#include "pageutils.h"
#include "theme/theme.h"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace {

QVector<SparkPoint> tempPoints(const QVector<HistorySample> &history, bool cpu)
{
    QVector<SparkPoint> pts;
    pts.reserve(history.size());
    for (const HistorySample &s : history) {
        SparkPoint p;
        p.value = cpu ? s.cpuTempC : s.gpuTempC;
        p.valid = cpu ? s.cpuTempValid : s.gpuTempValid;
        p.timestampMs = s.timestampMs;
        pts.append(p);
    }
    return pts;
}

// Real 60 s range of a temperature channel, computed from the history window.
// `ok` is false unless there are at least two valid samples, so we never show a
// range that is not backed by data.
void tempRange(const QVector<HistorySample> &history, bool cpu, bool &ok, double &mn, double &mx)
{
    ok = false;
    int n = 0;
    for (const HistorySample &s : history) {
        const bool v = cpu ? s.cpuTempValid : s.gpuTempValid;
        if (!v)
            continue;
        const double t = cpu ? s.cpuTempC : s.gpuTempC;
        if (!ok) {
            mn = mx = t;
            ok = true;
        } else {
            mn = std::min(mn, t);
            mx = std::max(mx, t);
        }
        ++n;
    }
    if (n < 2)
        ok = false;
}

// Fade a widget in after `delayMs`. Opacity only: the hero and the thermal panel
// are managed by layouts, and moving a layout-managed widget with move() fights
// the layout. When the layout re-activates (resize, show, or a state change that
// resizes the hero) it re-applies its own geometry on top of the stale captured
// position, which left the hero area blank and let the panel below overlap it.
// Fading keeps the restrained "settle in" feel without touching geometry.
void fadeIn(QWidget *w, int delayMs)
{
    if (!w)
        return;
    QTimer::singleShot(delayMs, w, [w]() {
        auto *effect = new QGraphicsOpacityEffect(w);
        effect->setOpacity(0.0);
        w->setGraphicsEffect(effect);

        auto *fade = new QPropertyAnimation(effect, "opacity", w);
        fade->setDuration(220);
        fade->setStartValue(0.0);
        fade->setEndValue(1.0);
        fade->setEasingCurve(QEasingCurve::OutCubic);
        QObject::connect(fade, &QPropertyAnimation::finished, w, [w]() {
            w->setGraphicsEffect(nullptr);
        });
        fade->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

} // namespace

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = PageUtils::makeCenteredColumn(this);

    root->addWidget(PageUtils::pageTitle(QStringLiteral("Dashboard")));
    root->addWidget(PageUtils::pageSubtitle(
        QStringLiteral("System state and hardware telemetry at a glance.")));

    m_body = new QVBoxLayout;
    m_body->setContentsMargins(0, Theme::Space::M, 0, 0);
    m_body->setSpacing(Theme::Space::L);
    root->addLayout(m_body, 1);

    // Hero state first (always visible).
    m_hero = new HeroStatus;
    m_body->addWidget(m_hero);

    m_stack = new QStackedWidget;

    // Content page: one wide thermal panel (CPU | GPU) instead of two boxes.
    m_content = new QWidget;
    auto *contentLayout = new QVBoxLayout(m_content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    m_panel = new ThermalPanel;
    m_cpu = m_panel->cpu();
    m_gpu = m_panel->gpu();
    // Let the panel take most of the space under the hero (bounded by its max
    // height). The trailing stretch keeps it top-aligned, so taller windows grow
    // the panel instead of leaving a blank area, without the panel floating in
    // the middle of the page.
    contentLayout->addWidget(m_panel, 10);
    contentLayout->addStretch(1);

    m_empty = new EmptyState;
    m_empty->setMessage(
        QStringLiteral("Waiting for hardware"),
        QStringLiteral("Load the excalibur_wmi driver to enable hardware telemetry. "
                       "This screen updates automatically when the device appears."));

    m_stack->addWidget(m_content);
    m_stack->addWidget(m_empty);
    m_body->addWidget(m_stack, 1);
}

void DashboardPage::playIntro()
{
    if (!qEnvironmentVariableIsEmpty("EXCALIBUR_NO_ANIM"))
        return;
    fadeIn(m_hero, 40);
    fadeIn(m_content, 90);
}

void DashboardPage::setSnapshot(const TelemetrySnapshot &s, const QVector<HistorySample> &history)
{
    const Units::TemperatureUnit unit = UserSettings::instance().tempUnit();
    m_hero->setTemperatureUnit(unit);
    m_cpu->setTemperatureUnit(unit);
    m_gpu->setTemperatureUnit(unit);

    if (s.status == AppState::Status::Disconnected) {
        m_bands.reset(); // fresh classification when it comes back
        m_hero->setSystemState(AppState::Status::Disconnected, AppState::Band::Normal);
        m_hero->setCoolingState(AppState::CoolingState::Unavailable);
        m_hero->setSensorLabels(s.cpuLabel, s.gpuLabel);
        m_hero->setHeadlineTemps(false, 0.0, AppState::Band::Normal,
                                 false, 0.0, AppState::Band::Normal);
        m_stack->setCurrentWidget(m_empty);
        return;
    }

    m_stack->setCurrentWidget(m_content);

    // Classify with hysteresis (shared across Dashboard/Cooling).
    m_bands.update(s.cpuTempValid, s.cpuTempC, s.gpuTempValid, s.gpuTempC);

    const AppState::Band sysBand = m_bands.systemBand();
    const bool fanValid = s.cpuFanValid || s.gpuFanValid;
    const bool fanMoving = (s.cpuFanValid && s.cpuFanRpm > 0) || (s.gpuFanValid && s.gpuFanRpm > 0);
    const AppState::CoolingState cooling = AppState::coolingState(fanValid, fanMoving, sysBand);

    m_hero->setSystemState(s.status, sysBand);
    m_hero->setCoolingState(cooling);
    m_hero->setSensorLabels(s.cpuLabel, s.gpuLabel);
    m_hero->setHeadlineTemps(s.cpuTempValid, s.cpuTempC, m_bands.cpu,
                             s.gpuTempValid, s.gpuTempC, m_bands.gpu);

    m_cpu->setIdentity(s.cpuLabel);
    m_gpu->setIdentity(s.gpuLabel);
    m_cpu->setTemperature(s.cpuTempC, s.cpuTempValid, m_bands.cpu);
    m_cpu->setTemperatureHistory(tempPoints(history, true));
    m_cpu->setFan(s.cpuFanRpm, s.cpuFanValid);
    {
        bool ok = false;
        double mn = 0.0;
        double mx = 0.0;
        tempRange(history, true, ok, mn, mx);
        m_cpu->setTemperatureRange(ok, mn, mx);
    }

    m_gpu->setTemperature(s.gpuTempC, s.gpuTempValid, m_bands.gpu);
    m_gpu->setTemperatureHistory(tempPoints(history, false));
    m_gpu->setFan(s.gpuFanRpm, s.gpuFanValid);
    {
        bool ok = false;
        double mn = 0.0;
        double mx = 0.0;
        tempRange(history, false, ok, mn, mx);
        m_gpu->setTemperatureRange(ok, mn, mx);
    }
}
