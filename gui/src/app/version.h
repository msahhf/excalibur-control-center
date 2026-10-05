#pragma once

// Product version comes from a single place: the CMake project() VERSION, passed
// in as EXCALIBUR_VERSION. The fallback only matters for editors/IDE parsing.
#ifndef EXCALIBUR_VERSION
#define EXCALIBUR_VERSION "0.1.0"
#endif

namespace AppVersion {
inline const char *string() { return EXCALIBUR_VERSION; }
} // namespace AppVersion
