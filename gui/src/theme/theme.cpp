#include "theme.h"

namespace Theme {
namespace {

Mode g_mode = Mode::Dark;

inline QColor col(const char *hex) { return QColor(QString::fromLatin1(hex)); }

// Dark token set (Section 14 defaults).
const QColor kDarkBackground = col("#0E1014");
const QColor kDarkSidebar = col("#12151A");
const QColor kDarkSurface = col("#161A20");
const QColor kDarkSurfaceElevated = col("#1D222A");
const QColor kDarkBorder = col("#272D37");
const QColor kDarkTextPrimary = col("#E8EAED");
const QColor kDarkTextSecondary = col("#8B93A1");
const QColor kDarkTextMuted = col("#5B6472");

// Light token set mirrors the same logic.
const QColor kLightBackground = col("#F3F4F7");
const QColor kLightSidebar = col("#FFFFFF");
const QColor kLightSurface = col("#FFFFFF");
const QColor kLightSurfaceElevated = col("#F7F8FA");
const QColor kLightBorder = col("#E1E4EA");
const QColor kLightTextPrimary = col("#14171C");
const QColor kLightTextSecondary = col("#58616F");
const QColor kLightTextMuted = col("#8C95A3");

// Accent + semantics (accent is a distinct red-orange so it is never mistaken
// for the critical/error color).
const QColor kAccentDark = col("#FF4D3D");
const QColor kAccentLight = col("#E23A2E");
const QColor kSuccessDark = col("#35C46B");
const QColor kSuccessLight = col("#1E9E52");
const QColor kWarningDark = col("#E0A33E");
const QColor kWarningLight = col("#B0751A");
const QColor kCriticalDark = col("#E5484D");
const QColor kCriticalLight = col("#D33A3A");
const QColor kCoolDark = col("#4FB0E6");
const QColor kCoolLight = col("#2A87C4");

} // namespace

void setMode(Mode mode) { g_mode = mode; }
Mode mode() { return g_mode; }
bool isDark() { return g_mode == Mode::Dark; }

QColor background() { return isDark() ? kDarkBackground : kLightBackground; }
QColor sidebar() { return isDark() ? kDarkSidebar : kLightSidebar; }
QColor surface() { return isDark() ? kDarkSurface : kLightSurface; }
QColor surfaceElevated() { return isDark() ? kDarkSurfaceElevated : kLightSurfaceElevated; }
QColor surfaceHover()
{
    QColor c = surfaceElevated();
    return isDark() ? c.lighter(112) : c.darker(102);
}
QColor border() { return isDark() ? kDarkBorder : kLightBorder; }

QColor textPrimary() { return isDark() ? kDarkTextPrimary : kLightTextPrimary; }
QColor textSecondary() { return isDark() ? kDarkTextSecondary : kLightTextSecondary; }
QColor textMuted() { return isDark() ? kDarkTextMuted : kLightTextMuted; }

QColor accent() { return isDark() ? kAccentDark : kAccentLight; }
QColor success() { return isDark() ? kSuccessDark : kSuccessLight; }
QColor warning() { return isDark() ? kWarningDark : kWarningLight; }
QColor critical() { return isDark() ? kCriticalDark : kCriticalLight; }
QColor cool() { return isDark() ? kCoolDark : kCoolLight; }

QFont font(int pointSize, QFont::Weight weight, int letterSpacing)
{
    QFont f;
    f.setPointSize(pointSize);
    f.setWeight(weight);
    if (letterSpacing != 100)
        f.setLetterSpacing(QFont::PercentageSpacing, letterSpacing);
    return f;
}

QFont labelFont(int pointSize, QFont::Weight weight, int letterSpacing)
{
    return font(pointSize, weight, letterSpacing);
}

} // namespace Theme
