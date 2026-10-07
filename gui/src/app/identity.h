#pragma once

#include <QString>

// Single source of truth for user-facing product identity. Keeps the executable
// / desktop / package identifier (excalibur-control-center, a machine name)
// separate from the display name (EXCALIBUR Control Center) that users read.
// The version is NOT here: it lives only in the CMake project() VERSION and is
// surfaced through app/version.h.
namespace AppIdentity {

inline QString displayName() { return QStringLiteral("EXCALIBUR Control Center"); }
inline QString desktopId() { return QStringLiteral("excalibur-control-center"); }
inline QString organization() { return QStringLiteral("EXCALIBUR"); }

// Icon theme name (installed under hicolor/…/apps/ and embedded via resources.qrc).
inline QString iconName() { return QStringLiteral("excalibur-control-center"); }

} // namespace AppIdentity
