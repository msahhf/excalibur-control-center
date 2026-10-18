#include "app/contrast.h"

#include <algorithm>
#include <cmath>

namespace Contrast {
namespace {

// WCAG 2.1 linearisation of one sRGB channel (input and output in [0, 1]).
double linearise(double channel)
{
    return channel <= 0.03928 ? channel / 12.92
                              : std::pow((channel + 0.055) / 1.055, 2.4);
}

} // namespace

double relativeLuminance(const QColor &opaque)
{
    return 0.2126 * linearise(opaque.redF())
         + 0.7152 * linearise(opaque.greenF())
         + 0.0722 * linearise(opaque.blueF());
}

double ratio(const QColor &a, const QColor &b)
{
    const double la = relativeLuminance(a);
    const double lb = relativeLuminance(b);
    const double lighter = std::max(la, lb);
    const double darker = std::min(la, lb);
    return (lighter + 0.05) / (darker + 0.05);
}

QColor compositeOver(const QColor &fg, const QColor &bg)
{
    const int alpha = fg.alpha();
    if (alpha == 255)
        return QColor(fg.red(), fg.green(), fg.blue(), 255);
    if (alpha == 0)
        return QColor(bg.red(), bg.green(), bg.blue(), 255);

    const int inverse = 255 - alpha;
    // Round-half-up 8-bit source-over blend: (fg*a + bg*(255-a) + 127) / 255.
    const auto blend = [alpha, inverse](int foreground, int backdrop) {
        return (foreground * alpha + backdrop * inverse + 127) / 255;
    };
    return QColor(blend(fg.red(), bg.red()),
                  blend(fg.green(), bg.green()),
                  blend(fg.blue(), bg.blue()),
                  255);
}

QColor bestInk(const QColor &bg, const QColor &base, const QColor &darkInk, const QColor &lightInk)
{
    // The surface the user actually sees: the (possibly translucent) fill
    // flattened onto its base, then both inks flattened onto that surface.
    const QColor surface = compositeOver(bg, base);
    const double darkRatio = ratio(compositeOver(darkInk, surface), surface);
    const double lightRatio = ratio(compositeOver(lightInk, surface), surface);
    return darkRatio >= lightRatio ? darkInk : lightInk;
}

double worstRatioForInk(const QVector<QColor> &samples, const QColor &base, const QColor &ink)
{
    if (samples.isEmpty())
        return 1.0; // nothing measured -> claim nothing
    double worst = 21.0; // ratio() never exceeds this
    for (const QColor &sample : samples) {
        const QColor surface = compositeOver(sample, base);
        worst = std::min(worst, ratio(compositeOver(ink, surface), surface));
    }
    return worst;
}

QColor bestInkForSpan(const QVector<QColor> &samples, const QColor &base, const QColor &darkInk,
                      const QColor &lightInk)
{
    const double dark = worstRatioForInk(samples, base, darkInk);
    const double light = worstRatioForInk(samples, base, lightInk);
    return dark >= light ? darkInk : lightInk;
}

} // namespace Contrast
