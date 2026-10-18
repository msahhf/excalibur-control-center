#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

#include "app/leddata.h"

// Thin, mock-able client for the kernel LED class exposed under
// /sys/class/leds/excalibur::<zone>/ with three attributes:
//
//   multi_intensity : "R G B"      (space separated, 0..255 each)
//   brightness      : "0".."15"
//   effect          : one of the stable tokens from Lighting::effectToString()
//
// The class never hard-codes /sys: the base path is injected so tests can point
// it at a QTemporaryDir. Writes are additionally gated behind an explicit
// hardware-write switch that is OFF by default, so constructing a client can
// never touch the real hardware by accident.
class RgbClient
{
public:
    static constexpr const char *kPrefix = "excalibur::";

    explicit RgbClient(const QString &ledBasePath = QStringLiteral("/sys/class/leds"));

    // Scan the base path for `excalibur::<zone>` entries. Returns true when at
    // least one known zone was found. Safe to call repeatedly.
    bool discover();

    // True when discover() has found at least one known zone.
    bool available() const;

    // Known zones that are actually present on this machine, in whitelist order.
    QVector<Lighting::Zone> availableZones() const;

    // The hardware-write switch. OFF by default: while OFF, write() is a no-op
    // that returns false WITHOUT opening any file.
    bool hardwareWriteEnabled() const { return m_writeEnabled; }
    void setHardwareWriteEnabled(bool enabled) { m_writeEnabled = enabled; }

    // Read the current state of a zone. Returns std::nullopt when:
    //   * the zone is not on the whitelist (an unknown enum is rejected BEFORE
    //     any path is built, so it is NEVER mapped onto Zone::All),
    //   * the zone is not present on this machine,
    //   * a file is missing, or a value cannot be parsed/validated.
    // In every rejected case no file is opened.
    std::optional<Lighting::LedData> read(Lighting::Zone zone) const;

    // Write the state of a zone. Returns false (and changes nothing, opening no
    // file) when:
    //   (a) hardware writes are disabled,
    //   (b) the zone is not on the whitelist (an unknown enum is rejected, it is
    //       NEVER mapped onto Zone::All),
    //   (c) the zone is not present on this machine,
    //   (d) any raw field is out of range -- zone/effect/brightness/colour are
    //       validated at full width BEFORE packing and BEFORE any file access,
    //       so pack()'s nibble masking can never legitimise a bad value.
    // Otherwise writes multi_intensity, then brightness, then effect, in that
    // order, and returns true only if every write succeeded.
    bool write(Lighting::Zone zone, const Lighting::LedData &data);

    // The exact string written to the multi_intensity attribute, e.g.
    // "255 0 128" (no trailing newline).
    static QString multiIntensityLine(const Lighting::LedData &data);

    // Absolute path of a zone's LED directory, e.g.
    // "<base>/excalibur::zone-a". Returns an EMPTY QString for a zone that is
    // not on the whitelist (it has no on-disk name).
    QString zonePath(Lighting::Zone zone) const;

private:
    QString m_basePath;
    QStringList m_zones; // discovered LED directory names, e.g. "excalibur::zone-a"
    bool m_writeEnabled = false;
};
