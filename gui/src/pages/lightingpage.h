#pragma once

#include "app/leddata.h"
#include "app/rgbclient.h"

#include <QColor>
#include <QVector>
#include <QWidget>

class ActionButton;
class EmptyState;
class QCheckBox;
class QHBoxLayout;
class QLabel;
class QSlider;
class QSpinBox;
class QStackedWidget;
class StatusPill;

// Hand-painted, product-styled widgets that belong exclusively to the Lighting
// page. They are defined (and moc'd) inside lightingpage.cpp; only forward
// declarations are needed here.
namespace LightingUi {
class Schematic;
class ZoneChip;
class EffectCard;
class ToggleSwitch;
class ColorSwatch;
class GlowPreview;
} // namespace LightingUi

// Lighting page: a prototype editor for the EXCALIBUR RGB LED controller.
//
// Nothing here writes to the hardware by default. The page:
//   * shows only the zones the kernel actually exposes (RgbClient::availableZones),
//   * lets the user compose effect / colour / brightness and previews the exact
//     values that WOULD be sent (dry-run, tokens shown verbatim),
//   * never calls RgbClient::write() unless BOTH an application gate (a checkbox,
//     OFF by default, never persisted) AND the read-only kernel gate
//     (/sys/module/excalibur_wmi/parameters/enable_write) are open.
//
// The kernel gate file is only ever opened ReadOnly; the page never escalates
// privileges and never copies the wide Windows/WMI permission model.
class LightingPage : public QWidget
{
    Q_OBJECT

public:
    explicit LightingPage(QWidget *parent = nullptr);

public slots:
    // Re-scan the LED base path for `excalibur::<zone>` entries (read-only).
    void rescan();

private:
    // Parsed kernel write gate. `exists` is false when the parameter file is
    // absent; `open` mirrors the driver's boolean spelling.
    struct KernelGate {
        bool exists = false;
        bool open = false;
        QString value;
    };

    void buildUi();
    QWidget *buildHeader();
    QWidget *buildContent();
    QWidget *buildControls();
    QWidget *buildPreview();
    QWidget *buildGate();
    QWidget *buildProtocolNote();

    void rebuildZoneSelectors();
    void selectZone(Lighting::Zone zone);
    void hydrateZone(Lighting::Zone zone);
    void applyStateToControls();
    void refreshPreview();
    void refreshGate();
    void updateAvailabilityPill();
    void setControlsEnabled(bool enabled);

    // Slots for the interactive controls.
    void onToggle(bool on);
    void onEffectPicked(Lighting::Effect effect);
    void onColorPicked(const QColor &color);
    void onRgbEdited();
    void onBrightnessChanged(int value);
    void onChooseColor();
    void onApply();

    // Outcome of the most recent "Apply to hardware" attempt. Partial is kept
    // distinct from Failed: the three sysfs attributes are written one by one, so
    // a failed write may still have changed part of the zone.
    enum class WriteOutcome { Applied, Partial, Failed, Blocked };
    void showWriteStatus(WriteOutcome outcome, const QString &detail);

    // Helpers.
    Lighting::LedData currentData() const { return m_data; }
    bool writeGateEffective() const;
    KernelGate readKernelGate() const;
    QString kernelGatePath() const;

    // --- Model / client ------------------------------------------------------
    RgbClient m_client;
    QVector<Lighting::Zone> m_availableZones;
    Lighting::Zone m_zone = Lighting::Zone::All;
    Lighting::LedData m_data;
    // Last live (non-Off) selection, restored when the user flips the switch back
    // on. Kept in memory only; never persisted.
    Lighting::LedData m_lastOn;
    bool m_uiGate = false;   // application gate; OFF by default, not persisted
    bool m_updating = false; // guards programmatic control updates
    // Last observed kernel gate (read-only). Kept so writeGateEffective() needs
    // no I/O; refreshed by refreshGate().
    bool m_kernelGateOpen = false;
    bool m_kernelGateExists = false;

    // --- Header --------------------------------------------------------------
    ActionButton *m_rescan = nullptr;
    StatusPill *m_availabilityPill = nullptr;

    // --- Zone selection ------------------------------------------------------
    QWidget *m_chipHost = nullptr;
    QHBoxLayout *m_chipLayout = nullptr;
    QLabel *m_zoneSummary = nullptr;
    QVector<LightingUi::ZoneChip *> m_chips;

    // --- Schematic -----------------------------------------------------------
    LightingUi::Schematic *m_schematic = nullptr;

    // --- Controls ------------------------------------------------------------
    QWidget *m_controlsPanel = nullptr;
    LightingUi::ToggleSwitch *m_toggle = nullptr;
    LightingUi::ColorSwatch *m_mainSwatch = nullptr;
    QVector<LightingUi::ColorSwatch *> m_palette;
    ActionButton *m_chooseColor = nullptr;
    QSpinBox *m_red = nullptr;
    QSpinBox *m_green = nullptr;
    QSpinBox *m_blue = nullptr;
    QSlider *m_brightness = nullptr;
    QLabel *m_brightnessValue = nullptr;
    QVector<LightingUi::EffectCard *> m_effectCards;

    // --- Preview -------------------------------------------------------------
    LightingUi::GlowPreview *m_glow = nullptr;
    QLabel *m_prevZone = nullptr;
    QLabel *m_prevEffect = nullptr;
    QLabel *m_prevBrightness = nullptr;
    QLabel *m_prevIntensity = nullptr;
    QLabel *m_prevPacked = nullptr;
    QLabel *m_previewNote = nullptr;

    // --- Hardware write gate -------------------------------------------------
    QWidget *m_gatePanel = nullptr;
    QCheckBox *m_uiGateBox = nullptr;
    StatusPill *m_uiGatePill = nullptr;
    StatusPill *m_kernelGatePill = nullptr;
    QLabel *m_gateWarning = nullptr;
    ActionButton *m_apply = nullptr;
    StatusPill *m_writePill = nullptr;
    QLabel *m_writeStatus = nullptr;

    // --- Availability --------------------------------------------------------
    QStackedWidget *m_stack = nullptr;
    QWidget *m_content = nullptr;
    EmptyState *m_empty = nullptr;
};
