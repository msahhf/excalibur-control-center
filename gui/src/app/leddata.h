#pragma once

#include <QString>

#include <cstdint>
#include <optional>

// Pure, dependency-free user-space model for the EXCALIBUR RGB LED controller.
//
// The kernel driver / ACPI WMI interface expects a single little-endian 32-bit
// word that packs every user-visible property:
//
//   bits 28..31 : effect         (4-bit whitelist, see Effect)
//   bits 24..27 : brightness     (4-bit, 0..15)
//   bits 16..23 : red            (8-bit, 0..255)
//   bits  8..15 : green          (8-bit, 0..255)
//   bits  0.. 7 : blue           (8-bit, 0..255)
//
// This header carries no Qt-Widgets / sysfs dependency so it can be reused by
// the UI, the tests and any future daemon. String tokens are stable and
// deliberately OUTSIDE the translation system (i18n): they are wire/on-disk
// identifiers, never shown to the user.

namespace Lighting {

// Effect values are the raw nibble written to the driver. Only these values
// are accepted; anything else is rejected by isValidEffect().
enum class Effect : std::uint8_t {
    Off = 0,
    Static = 1,
    Breathing = 3,
    ColorfulCycle = 6,
    Ambilight = 7,
};

// Zone values are the raw driver ids. They are NOT contiguous; only the
// whitelisted subset below is valid.
enum class Zone : std::uint8_t {
    All = 0,
    ZoneC = 3,
    ZoneB = 4,
    ZoneA = 5,
    Keyboard = 6,
    Trunk = 7,
    Logo = 8,
};

// Decoded user-space representation of the 32-bit word.
struct LedData {
    Effect effect = Effect::Off;
    std::uint8_t brightness = 0; // 0..15
    std::uint8_t red = 0;        // 0..255
    std::uint8_t green = 0;      // 0..255
    std::uint8_t blue = 0;       // 0..255
};

inline constexpr int kBrightnessMin = 0;
inline constexpr int kBrightnessMax = 15;

// pack/unpack are exact inverses for any LedData whose effect is whitelisted.
std::uint32_t pack(const LedData &data);
LedData unpack(std::uint32_t word);

// Whitelist predicates. Each reject the reserved / unknown values. They take
// `int` on purpose: callers routinely hold a value in a wider or signed
// variable (a slider, a parsed token, an enum tag) BEFORE narrowing it into
// LedData's fixed-width fields, and these predicates must be able to see the
// un-narrowed value.
bool isValidEffect(int effect);
bool isValidZone(int zone);
bool isValidBrightness(int brightness);

// Single colour component (red / green / blue), wide input: 0..255 inclusive.
// Rejects the values that a narrowing cast to std::uint8_t would wrap or clamp,
// so an out-of-range int cannot be written verbatim.
bool isValidColorComponent(int component);

// Raw-field validation of a LedData, INDEPENDENT of pack()'s nibble masking.
//
// pack() deliberately masks brightness and effect down to a nibble, so
// pack()-then-unpack() cannot detect an out-of-range input: brightness 200
// masks to 8 and would otherwise be written to sysfs as the literal "200".
// This overload inspects the fields at full width and rejects them. Prefer it
// over isValidLedData(word) on any path that leads to a real write.
//
// NOTE: this can only inspect a LedData that ALREADY has the (narrow) field
// types, so it is structurally unable to see a value that a caller has already
// narrowed on the way in (brightness 256 -> 0, red -1 -> 255). Use
// makeLedData() to validate the raw, wide input BEFORE any narrowing happens.
bool isValid(const LedData &data);

// Narrow-and-validate factory for the raw, wide/signed values as they arrive
// from a UI control, a parsed token or a settings store.
//
// Returns a fully formed LedData ONLY when every field is in range:
//   * effect      : on the effect whitelist (isValidEffect)
//   * brightness  : 0..15 inclusive
//   * red/green/blue: 0..255 inclusive, per component
// Any out-of-range field -- including negative values and values a narrowing
// cast would wrap (brightness 256 -> 0, red -1 -> 255) -- yields std::nullopt.
// No masking or truncation is ever applied, so a bad value can never be turned
// into a valid one on the way in.
//
// The zone is deliberately NOT part of LedData; validate a zone separately with
// isValidZone() before building an RgbClient path from it.
std::optional<LedData> makeLedData(int effect, int brightness, int red, int green, int blue);

// True when the nibble-encoded word uses a whitelisted effect (brightness is
// always a valid 0..15 nibble by construction, so this only proves the WORD is
// well formed -- it can never prove the pre-packing input was in range).
bool isValidLedData(std::uint32_t word);

// Token conversion. Tokens are stable, i18n-free identifiers:
//   effect: "off" | "static" | "breathing" | "colorful-cycle" | "ambilight"
//   zone:   "all" | "zone-c" | "zone-b" | "zone-a" | "keyboard" | "trunk" | "logo"
// Unknown input yields a SAFE default for fromString() (Effect::Off / Zone::All)
// and, when `ok` is non-null, sets *ok = false. Known input sets *ok = true.
//
// zoneToString() is asymmetric on purpose: a KNOWN zone returns its stable
// token (Zone::All still returns "all"), but an UNKNOWN enum value returns an
// EMPTY QString -- never a real token. Returning "all" here would let a stray
// value silently masquerade as the real All zone when a caller builds
// "excalibur::all"; an empty token instead makes the mistake obvious. Callers
// that turn a zone into an on-disk name must reject it with isValidZone() first.
QString effectToString(Effect effect);
Effect effectFromString(const QString &token, bool *ok = nullptr);

QString zoneToString(Zone zone);
Zone zoneFromString(const QString &token, bool *ok = nullptr);

} // namespace Lighting
