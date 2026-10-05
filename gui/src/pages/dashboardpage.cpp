#include "dashboardpage.h"

#include "components/emptystate.h"
#include "components/hardwaremodule.h"
#include "components/herostatus.h"
#include "components/thermalpanel.h"
#include "pageutils.h"
#include "theme/theme.h"

#include <QHBoxLayout>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

QVector<SparkPoint> tempPoints(const QVector<HistorySample> &history, bool cpu)
{
    QVector<SparkPoint> pts;
    pts.reserve(history.size());
    for (const HistorySample &s : history) {
        SparkPoint p;
        p.value = cpu ? s.cpuTempC : s.gpuTempC;
        p.valid = cpu ? s.cpuTempValid : s.gpuTempValid;
        pts.append(p);
    }
    return pts;
}

} // namespace

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::Space::XXL, Theme::Space::XL, Theme::Space::XXL, Theme::Space::XL);
    root->setSpacing(Theme::Space::L);

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
    auto *contentLayout = new QHBoxLayout(m_content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    m_panel = new ThermalPanel;
    m_cpu = m_panel->cpu();
    m_gpu = m_panel->gpu();
    contentLayout->addWidget(m_panel, 1);

    m_empty = new EmptyState;
    m_empty->setMessage(
        QStringLiteral("Waiting for hardware"),
        QStringLiteral("Load the excalibur_wmi driver to enable hardware telemetry. "
                       "This screen updates automatically when the device appears."));

    m_stack->addWidget(m_content);
    m_stack->addWidget(m_empty);
    m_body->addWidget(m_stack, 1);
}

void DashboardPage::setSnapshot(const TelemetrySnapshot &s, const QVector<HistorySample> &history)
{
    if (s.status == AppState::Status::Disconnected) {
        m_haveBands = false; // fresh classification when it comes back
        m_hero->setSystemState(AppState::Status::Disconnected, AppState::Band::Normal);
        m_stack->setCurrentWidget(m_empty);
        return;
    }

    m_stack->setCurrentWidget(m_content);

    // Classify with hysteresis.
    const auto cpuTh = AppState::cpuThresholds();
    const auto gpuTh = AppState::gpuThresholds();

    if (!m_haveBands) {
        if (s.cpuTempValid)
            m_cpuBand = AppState::classify(s.cpuTempC, cpuTh);
        if (s.gpuTempValid)
            m_gpuBand = AppState::classify(s.gpuTempC, gpuTh);
        m_haveBands = true;
    } else {
        if (s.cpuTempValid)
            m_cpuBand = AppState::classifyHysteresis(s.cpuTempC, cpuTh, m_cpuBand);
        if (s.gpuTempValid)
            m_gpuBand = AppState::classifyHysteresis(s.gpuTempC, gpuTh, m_gpuBand);
    }

    const AppState::Band sysBand = AppState::worst(m_cpuBand, m_gpuBand);
    m_hero->setSystemState(s.status, sysBand);

    m_cpu->setTemperature(s.cpuTempC, s.cpuTempValid, m_cpuBand);
    m_cpu->setTemperatureHistory(tempPoints(history, true));
    m_cpu->setFan(s.cpuFanRpm, s.cpuFanValid);

    m_gpu->setTemperature(s.gpuTempC, s.gpuTempValid, m_gpuBand);
    m_gpu->setTemperatureHistory(tempPoints(history, false));
    m_gpu->setFan(s.gpuFanRpm, s.gpuFanValid);
}
