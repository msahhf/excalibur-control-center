#include "theme.h"

#include <QApplication>
#include <QPalette>
#include <QtGlobal>

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
// Muted captions (9-10 px labels: group headers, "RPM", version). WCAG 1.4.3
// AA small text = 4.5:1. Measured against every dark surface muted text can sit
// on: background 5.39, sidebar 5.18, surface 4.94, surfaceElevated 4.52.
// (The previous #5B6472 was 2.67-3.18:1 and failed everywhere.)
const QColor kDarkTextMuted = col("#7F899A");

// Light token set mirrors the same logic.
const QColor kLightBackground = col("#F3F4F7");
const QColor kLightSidebar = col("#FFFFFF");
const QColor kLightSurface = col("#FFFFFF");
const QColor kLightSurfaceElevated = col("#F7F8FA");
const QColor kLightBorder = col("#D4D8DF");
const QColor kLightTextPrimary = col("#14171C");
const QColor kLightTextSecondary = col("#58616F");
// AA small text on the lightest-on-dim surfaces: background (#F3F4F7) 4.74,
// sidebar/surface (#FFFFFF) 5.22, surfaceElevated 4.91, hover 4.70.
// (The previous #8C95A3 was 2.72-3.02:1 and failed everywhere.)
const QColor kLightTextMuted = col("#646D7D");

// Accent + semantics (accent is a distinct red-orange so it is never mistaken
// for the critical/error color).
const QColor kAccentDark = col("#FF4D3D");
// Light accent is also used for small text and focus rings; #C42D22 keeps
// 5.61:1 on white / 5.10:1 on the page background (was 4.30 / 3.91).
const QColor kAccentLight = col("#C42D22");
const QColor kSuccessDark = col("#35C46B");
// Light semantics are read as small text ("Connected", band labels) on cards
// and page background: 5.08 / 4.62 (was 3.46 / 3.15, AA fail).
const QColor kSuccessLight = col("#157F3D");
const QColor kWarningDark = col("#E0A33E");
// 5.92 / 5.38 (was 3.88 / 3.53, AA fail).
const QColor kWarningLight = col("#8A5A0B");
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

void applyApplicationPalette()
{
    if (!qApp)
        return;
    QPalette p;
    p.setColor(QPalette::Window, background());
    p.setColor(QPalette::WindowText, textPrimary());
    p.setColor(QPalette::Base, surface());
    p.setColor(QPalette::AlternateBase, surfaceElevated());
    p.setColor(QPalette::Text, textPrimary());
    p.setColor(QPalette::Button, surfaceElevated());
    p.setColor(QPalette::ButtonText, textPrimary());
    p.setColor(QPalette::ToolTipBase, surfaceElevated());
    p.setColor(QPalette::ToolTipText, textPrimary());
    p.setColor(QPalette::Mid, border());
    p.setColor(QPalette::Highlight, accent());
    // Selection/hover fill colour is the accent. In the light theme the accent
    // was darkened for AA small text (#C42D22), which would drag the previous
    // dark highlight text down to 3.20:1 — white on it is 5.61:1. The dark
    // accent is unchanged, so it keeps the primary text pairing.
    p.setColor(QPalette::HighlightedText, isDark() ? textPrimary() : QColor(Qt::white));
    p.setColor(QPalette::Disabled, QPalette::WindowText, textMuted());
    p.setColor(QPalette::Disabled, QPalette::Text, textMuted());
    qApp->setPalette(p);
}

QFont font(int pointSize, QFont::Weight weight, int letterSpacing)
{
    QFont f;
    f.setPointSize(pointSize);
    f.setWeight(weight);
    if (letterSpacing != 100)
        f.setLetterSpacing(QFont::PercentageSpacing, letterSpacing);
    enableTabularFigures(f);
    return f;
}

QFont labelFont(int pointSize, QFont::Weight weight, int letterSpacing)
{
    return font(pointSize, weight, letterSpacing);
}

} // namespace Theme
