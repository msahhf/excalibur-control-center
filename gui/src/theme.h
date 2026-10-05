#pragma once

#include <QColor>
#include <QPalette>
#include <QWidget>

// Central design tokens + palette-derived helpers.
// Everything except the small semantic/brand accents comes from the active
// Qt palette, so KDE/Breeze dark and light themes both render correctly.
namespace Theme {

// EXCALIBUR brand accent — used sparingly (branding / active / focus).
inline constexpr const char *Accent = "#c8102e";

// Semantic state colors (small accents only).
inline constexpr const char *Ok    = "#2e9e5b"; // healthy / online
inline constexpr const char *Warn  = "#cf8a1a"; // warning / degraded
inline constexpr const char *Error = "#cf4444"; // critical / offline

// Muted secondary text.
inline QColor dim(const QWidget *w)
{
    return w->palette().color(QPalette::Disabled, QPalette::WindowText);
}

// Subtle border color.
inline QColor border(const QWidget *w)
{
    return w->palette().color(QPalette::Mid);
}

// Slightly elevated surface derived from the window color (theme-aware).
inline QColor surface(const QWidget *w)
{
    QColor c = w->palette().color(QPalette::Window);
    return c.lightness() < 128 ? c.lighter(113) : c.darker(104);
}

} // namespace Theme
