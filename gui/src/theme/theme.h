#pragma once

#include <QColor>
#include <QFont>
#include <QString>
#include <QtGlobal>

// Central design tokens for the EXCALIBUR Control Center.
//
// The UI is NOT palette-derived: the product owns an explicit token set so the
// look is identical across KDE/Breeze, GNOME, Wayland and X11. Two modes are
// supported (Dark default, Light). All widget code asks this namespace for
// colors, spacing, radii and fonts, so there is exactly one place to tune the
// product's look.
namespace Theme {

enum class Mode { Dark, Light };

// Setonce, very early in main() before any widget is constructed.
void setMode(Mode mode);
Mode mode();
bool isDark();

// --- Surfaces (layered by tone; borders stay subtle) ---------------------
QColor background();       // page background
QColor sidebar();          // navigation surface
QColor surface();          // standard card surface
QColor surfaceElevated();  // raised sub-surface / hovered card
QColor surfaceHover();     // interaction hover tint
QColor border();           // low-contrast divider/outline

// --- Text ----------------------------------------------------------------
QColor textPrimary();
QColor textSecondary();
QColor textMuted();

// --- Brand + semantics (accent is NOT an error color) --------------------
QColor accent();     // EXCALIBUR red-orange, used sparingly
QColor success();    // healthy
QColor warning();    // caution
QColor critical();   // dangerous / unavailable
QColor cool();       // cool thermal band (informational blue)

// --- Spacing scale (4/8/12/16/24/32/40) ----------------------------------
namespace Space {
constexpr int XS = 4;
constexpr int S = 8;
constexpr int M = 12;
constexpr int L = 16;
constexpr int XL = 24;
constexpr int XXL = 32;
constexpr int XXXL = 40;
} // namespace Space

constexpr int RadiusSmall = 8;
constexpr int Radius = 14;
constexpr int CardPadding = 22;

// --- Typography ----------------------------------------------------------
// Base point sizes relative to the platform default. Supplying an explicit
// family-independent point size (rather than delta) keeps hierarchy stable.
QFont font(int pointSize, QFont::Weight weight = QFont::Normal, int letterSpacing = 100);
// Small uppercase-ish caption/label style.
QFont labelFont(int pointSize = 10, QFont::Weight weight = QFont::DemiBold, int letterSpacing = 135);

// Tabular (fixed-width) figures, so animated numbers keep a stable width.
// No-op on Qt < 6.7 where font features are unavailable.
inline void enableTabularFigures(QFont &f)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    f.setFeature("tnum", 1);
#else
    Q_UNUSED(f);
#endif
}

} // namespace Theme
