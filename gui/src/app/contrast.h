#pragma once

#include <QColor>
#include <QVector>

// Pure, dependency-free WCAG 2.1 contrast helpers (F3 infrastructure).
//
// Small, fully testable logic shared by the UI: it lets a widget pick an ink
// colour that stays readable on a surface that may be *semi-transparent* (a
// tinted fill drawn over a base). Everything here is a pure function of its
// arguments: no widgets, no palette, no global state, no I/O.
//
// Reference: W3C "Web Content Accessibility Guidelines (WCAG) 2.1",
// §1.4.3 (contrast minimum) and the relative-luminance definition in
// §Relative luminance. AA thresholds are 4.5:1 for normal text, 3:1 for large
// text; this module only computes the numbers, callers apply the threshold.
//
// Colour-space note: channel values are treated as sRGB (QColor::redF() etc.)
// and linearised with the WCAG piecewise transfer function. Alpha is NOT a
// colour: every function that needs a colour first flattens it onto an opaque
// backdrop with compositeOver(), exactly like the compositor does.

namespace Contrast {

// WCAG 2.1 relative luminance of an OPAQUE colour, in [0.0, 1.0].
// The alpha channel is ignored: pass an already-composited colour (use
// compositeOver() first) when the source has transparency.
//   0.2126*R + 0.7152*G + 0.0722*B  with each channel linearised:
//   c <= 0.03928 ? c/12.92 : ((c + 0.055)/1.055)^2.4
double relativeLuminance(const QColor &opaque);

// WCAG contrast ratio between two opaque colours, in [1.0, 21.0]:
//   (L_lighter + 0.05) / (L_darker + 0.05)
// Symmetric in its arguments; identical colours yield exactly 1.0.
double ratio(const QColor &a, const QColor &b);

// Source-over ("normal") composite of `fg` onto `bg`, returning an OPAQUE
// colour. `bg` is treated as an opaque backdrop (its own alpha is ignored:
// callers that need a transparent backdrop flatten it first, e.g.
// compositeOver(tint, base)). Edge cases:
//   * fg.alpha() == 255 -> fg's channels, unchanged.
//   * fg.alpha() == 0   -> bg's channels.
//   * otherwise         -> per-channel linear blend, rounded, alpha forced 255.
QColor compositeOver(const QColor &fg, const QColor &bg);

// Pick whichever ink reads better on a surface formed by compositing `bg`
// (which may be semi-transparent) over `base`. Both candidate inks are also
// composited over that surface before their ratios are compared, so a
// semi-transparent ink is judged by what the user actually sees — alpha is
// never ignored. Returns the ORIGINAL candidate (not the composited copy);
// on an exact ratio tie `darkInk` wins.
//
// Typical use: dark "ink on a dark card" and light "ink on a light card" are
// both offered, and this returns the one with the higher WCAG ratio.
QColor bestInk(const QColor &bg, const QColor &base, const QColor &darkInk, const QColor &lightInk);

// --- Span (gradient) helpers ----------------------------------------------
// A gradient-filled region has no single background colour: a label placed on
// it sits over a RANGE of colours. bestInk() only judges one surface, so using
// it on the gradient's "brightest" stop is not enough -- the label's text box
// can straddle several stops whose worst point is darker or lighter than that
// stop. These two helpers let a caller pick one ink for a WHOLE span and know
// how it does at the span's worst point (the WCAG 2.1 §1.4.3 "contrast
// minimum" is a worst-case rule).

// Worst (minimum) WCAG ratio `ink` achieves against ANY of the `samples`, with
// every sample and the ink first flattened onto the opaque `base` (so a
// translucent sample or ink is judged by what is actually drawn). An empty
// sample list returns 1.0: a caller with nothing to measure cannot claim a win.
double worstRatioForInk(const QVector<QColor> &samples, const QColor &base, const QColor &ink);

// Pick whichever candidate ink maximises the WORST-case ratio over the whole
// span, so the returned ink clears as many of the span's colours as possible.
// On an exact tie `darkInk` wins, mirroring bestInk(). Callers compare the
// result with worstRatioForInk() to decide whether the span is safe or needs a
// solid backing surface (a badge) behind the text.
QColor bestInkForSpan(const QVector<QColor> &samples, const QColor &base, const QColor &darkInk,
                      const QColor &lightInk);

} // namespace Contrast
