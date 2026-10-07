#pragma once

#include <QWidget>

#include "app/appstate.h"
#include "components/surfacepanel.h"
#include "util/units.h"

class QLabel;
class QWidget;
class QPainter;
class StatusPill;
class AnimatedNumber;
class StateDot;
class QGraphicsOpacityEffect;
class QPropertyAnimation;

// Hero system-state band. This is the first thing the user reads on the
// Dashboard: a plain-language state derived from real telemetry, the honest
// cooling state, and the two headline temperatures at a glance. The full
// per-sensor detail (fan RPM, 60 s trend) lives in the CPU/GPU cards below, so the
// hero never repeats the whole vitals table. Raw technical terms (DEGRADED, hwmon
// paths) never appear here.
class HeroStatus : public SurfacePanel
{
    Q_OBJECT

public:
    explicit HeroStatus(QWidget *parent = nullptr);

    // Overall connection + thermal state (drives title, subtitle and accent bar).
    void setSystemState(AppState::Status status, AppState::Band band);

    // Honest cooling state derived from real telemetry (drives the status pill).
    void setCoolingState(AppState::CoolingState cooling);

    // Sensor labels come from the hwmon backend (see HwmonClient), already
    // sanitised with CPU/GPU fallbacks. Used for the headline captions.
    void setSensorLabels(const QString &cpu, const QString &gpu);

    // Presentation unit only; the snapshot always stays Celsius.
    void setTemperatureUnit(Units::TemperatureUnit unit);

    // Headline temperatures. Each carries its own validity flag so a missing
    // reading shows an em dash instead of a fake zero.
    void setHeadlineTemps(bool cpuValid, double cpuC, AppState::Band cpuBand,
                          bool gpuValid, double gpuC, AppState::Band gpuBand);

protected:
    void paintOverlay(QPainter &p, const QRectF &surface) override;

private:
    QLabel *m_label = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_subtitle = nullptr;
    StatusPill *m_pill = nullptr;
    QWidget *m_content = nullptr;

    // Headline temperatures (inline, right of the state title).
    StateDot *m_cpuDot = nullptr;
    StateDot *m_gpuDot = nullptr;
    QLabel *m_cpuCaption = nullptr;
    QLabel *m_gpuCaption = nullptr;
    AnimatedNumber *m_cpuTemp = nullptr;
    AnimatedNumber *m_gpuTemp = nullptr;
    QLabel *m_cpuUnit = nullptr;
    QLabel *m_gpuUnit = nullptr;
    Units::TemperatureUnit m_tempUnit = Units::TemperatureUnit::Celsius;

    QGraphicsOpacityEffect *m_effect = nullptr;
    QPropertyAnimation *m_fade = nullptr;
    AppState::Band m_band = AppState::Band::Normal;
    AppState::Status m_status = AppState::Status::Connected;
    bool m_hasState = false;
};
