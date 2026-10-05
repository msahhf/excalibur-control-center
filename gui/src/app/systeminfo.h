#pragma once

#include <QString>

// Read-only, unprivileged machine identity. Sources: DMI (/sys/class/dmi/id/*)
// and the kernel via QSysInfo (uname). No writes, no privileges. Any value that
// cannot be read is left empty; the UI renders it as "Not available".
struct SystemInfo {
    QString manufacturer;
    QString productName;
    QString boardName;
    QString biosVersion;
    QString biosDate;
    QString kernelVersion;
    QString kernelType;
    QString architecture;

    // e.g. "EXCALIBUR G870" or "Not available".
    QString model() const;
    QString bios() const; // "CQ141 (06/27/2024)" when both parts exist
};

SystemInfo readSystemInfo();
