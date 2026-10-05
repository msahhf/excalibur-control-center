#include "dashboardpage.h"

#include "components/emptystate.h"
#include "components/hardwaremodule.h"
#include "components/herostatus.h"
#include "pageutils.h"
#include "theme/theme.h"

#include <QHBoxLayout>
#include <QStackedWidget>
#include <QVBoxLayout>

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

    // Content page: two hardware modules.
    m_content = new QWidget;
    auto *contentLayout = new QHBoxLayout(m_content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(Theme::Space::L);

    m_cpu = new HardwareModule(QStringLiteral("CPU"));
    m_gpu = new HardwareModule(QStringLiteral("GPU"));
    contentLayout->addWidget(m_cpu, 1);
    contentLayout->addWidget(m_gpu, 1);

    m_empty = new EmptyState;
    m_empty->setMessage(
        QStringLiteral("Waiting for hardware"),
        QStringLiteral("Load the excalibur_wmi driver to enable hardware telemetry. "
                       "This screen updates automatically when the device appears."));

    m_stack->addWidget(m_content);
    m_stack->addWidget(m_empty);
    m_body->addWidget(m_stack, 1);
}

void DashboardPage::setSnapshot(const TelemetrySnapshot &s)
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

    AppState::Band sysBand = AppState::worst(m_cpuBand, m_gpuBand);
    m_hero->setSystemState(s.status, sysBand);

    m_cpu->setTemperature(s.cpuTempC, s.cpuTempValid, m_cpuBand);
    m_cpu->setFan(s.cpuFanRpm, s.cpuFanValid);
    m_gpu->setTemperature(s.gpuTempC, s.gpuTempValid, m_gpuBand);
    m_gpu->setFan(s.gpuFanRpm, s.gpuFanValid);
}
