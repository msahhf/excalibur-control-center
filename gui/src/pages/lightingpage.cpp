#include "lightingpage.h"

#include "app/contrast.h"
#include "app/leddata.h"
#include "components/actionbutton.h"
#include "components/emptystate.h"
#include "components/statuspill.h"
#include "components/surfacepanel.h"
#include "pageutils.h"
#include "theme/theme.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QColorDialog>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <cmath>
#include <algorithm>

namespace {

// ---------------------------------------------------------------------------
// The kernel write gate. This path is only ever opened ReadOnly (see
// LightingPage::readKernelGate). It is never written by this process.
// ---------------------------------------------------------------------------
const char *kKernelGateDefault = "/sys/module/excalibur_wmi/parameters/enable_write";

// Canonical presentation order for zones. This differs from the raw driver id
// order (0,3,4,5,6,7,8): the UI pins "Select all" first and groups Zone A/B/C
// left-to-right. Only zones present on this machine are ever shown.
const Lighting::Zone kPresentationOrder[] = {
    Lighting::Zone::All,
    Lighting::Zone::ZoneA,
    Lighting::Zone::ZoneB,
    Lighting::Zone::ZoneC,
    Lighting::Zone::Keyboard,
    Lighting::Zone::Trunk,
    Lighting::Zone::Logo,
};

// Effects offered as cards, in OEM order.
const Lighting::Effect kEffectOrder[] = {
    Lighting::Effect::Off,
    Lighting::Effect::Static,
    Lighting::Effect::Breathing,
    Lighting::Effect::ColorfulCycle,
    Lighting::Effect::Ambilight,
};

// Preset palette (brand-adjacent, contrast-tuned for both themes).
const char *kPaletteHex[] = {
    "#FF4D3D", // red
    "#FF9F0A", // orange
    "#FFD60A", // amber
    "#35C46B", // green
    "#32C7C7", // teal
    "#4FB0E6", // blue
    "#8B5CF6", // violet
    "#FFFFFF", // white
};

// OEM region labels. "Select all" and "Keyboard" are ordinary words and are
// translated; Zone A/B/C, Trunk and Logo are product/brand tokens and stay
// verbatim (never passed through tr()).
QString zoneDisplayName(Lighting::Zone zone)
{
    switch (zone) {
    case Lighting::Zone::All:
        return QCoreApplication::translate("LightingPage", "Select all");
    case Lighting::Zone::Keyboard:
        return QCoreApplication::translate("LightingPage", "Keyboard");
    case Lighting::Zone::ZoneA:
        return QStringLiteral("Zone A");
    case Lighting::Zone::ZoneB:
        return QStringLiteral("Zone B");
    case Lighting::Zone::ZoneC:
        return QStringLiteral("Zone C");
    case Lighting::Zone::Trunk:
        return QStringLiteral("Trunk");
    case Lighting::Zone::Logo:
        return QStringLiteral("Logo");
    }
    return QString();
}

// OEM effect names, verified against the vendor XAML. OFF / Static / Breathing
// / Colorful cycle are translated; "Ambilight" is a brand token and stays.
QString effectDisplayName(Lighting::Effect effect)
{
    switch (effect) {
    case Lighting::Effect::Off:
        return QCoreApplication::translate("LightingPage", "OFF");
    case Lighting::Effect::Static:
        return QCoreApplication::translate("LightingPage", "Static");
    case Lighting::Effect::Breathing:
        return QCoreApplication::translate("LightingPage", "Breathing");
    case Lighting::Effect::ColorfulCycle:
        return QCoreApplication::translate("LightingPage", "Colorful cycle");
    case Lighting::Effect::Ambilight:
        return QCoreApplication::translate("LightingPage", "Ambilight");
    }
    return QString();
}

// Small uppercase section caption ("ZONES", "CONTROLS", ...).
QLabel *sectionLabel(const QString &text)
{
    auto *l = new QLabel(text);
    l->setFont(Theme::labelFont(9, QFont::DemiBold, 150));
    QPalette p = l->palette();
    p.setColor(QPalette::WindowText, Theme::textMuted());
    l->setPalette(p);
    return l;
}

// A read-only value line for the PREVIEW panel (selectable so the exact tokens
// can be copied out).
QLabel *previewValue()
{
    auto *l = new QLabel(QStringLiteral("\u2014"));
    l->setFont(Theme::font(11, QFont::Medium));
    l->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QPalette p = l->palette();
    p.setColor(QPalette::WindowText, Theme::textPrimary());
    l->setPalette(p);
    return l;
}

QLabel *previewCaption(const QString &text)
{
    auto *l = new QLabel(text);
    l->setFont(Theme::font(11, QFont::Normal));
    l->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QPalette p = l->palette();
    p.setColor(QPalette::WindowText, Theme::textSecondary());
    l->setPalette(p);
    return l;
}

// Representative colour used to paint a zone region / glyph. Off → dim neutral.
QColor representativeColor(const Lighting::LedData &data)
{
    if (data.effect == Lighting::Effect::Off) {
        QColor c = Theme::textMuted();
        c.setAlpha(90);
        return c;
    }
    QColor base(data.red, data.green, data.blue);
    if (!base.isValid())
        base = Theme::accent();
    const qreal f = 0.30 + 0.70 * (static_cast<qreal>(data.brightness) / Lighting::kBrightnessMax);
    return QColor::fromRgbF(qBound(0.0, base.redF() * f, 1.0),
                            qBound(0.0, base.greenF() * f, 1.0),
                            qBound(0.0, base.blueF() * f, 1.0));
}

// The two label-ink candidates kept from the original design. Contrast::bestInk
// returns whichever of the two actually reads better on a given fill.
const QColor kDarkInk(0x14, 0x17, 0x1C);
const QColor kLightInk(0xE8, 0xEA, 0xED);

// Last-resort achromatic inks. Some saturated full-brightness fills cannot
// reach WCAG AA with either design ink: the "violet" preset (#8B5CF6) at full
// brightness tops out at ~4.24:1 (dark ink), just under the 4.5:1 minimum.
// Pure black reads ~4.96:1 there. Because max(min(black-white)) over the
// achromatic pair bottoms out at ~4.58:1 on ANY opaque surface (worst case is a
// mid-grey), mixing pure black and white is always enough to clear AA. These are
// only used when the design inks fall short, so the design language is
// untouched everywhere else.
const QColor kFallbackBlack(0x00, 0x00, 0x00);
const QColor kFallbackWhite(0xFF, 0xFF, 0xFF);

// WCAG 2.1 AA contrast minimum for normal-size text.
constexpr double kAaNormalTextRatio = 4.5;

// Ink colour for a label drawn on top of a filled region. `bg` is the region
// fill (which may be semi-transparent, e.g. the dimmed OFF fill); `base` is the
// real surface the fill sits on (the schematic card). Contrast::bestInk flattens
// both the fill and each candidate ink onto that surface before comparing WCAG
// ratios, so the fill's alpha is never ignored -- the previous hand-rolled
// luminance test did ignore it and picked a dark ink that measured only ~1.9:1
// over the dimmed OFF fill on the dark theme (F3 root cause).
//
// Design-first: keep the intended design ink whenever it already clears AA
// (>= 4.5:1). Only when it does not (e.g. full-brightness violet) do we fall
// back to pure black/white, whichever reads better -- so the design language is
// preserved for every fill that was already accessible.
QColor inkOver(const QColor &bg, const QColor &base)
{
    const QColor surface = Contrast::compositeOver(bg, base);
    const QColor designInk = Contrast::bestInk(bg, base, kDarkInk, kLightInk);
    if (Contrast::ratio(Contrast::compositeOver(designInk, surface), surface)
        >= kAaNormalTextRatio)
        return designInk;

    const double blackRatio = Contrast::ratio(kFallbackBlack, surface);
    const double whiteRatio = Contrast::ratio(kFallbackWhite, surface);
    return blackRatio >= whiteRatio ? kFallbackBlack : kFallbackWhite;
}

// Colour of a stop-interpolated linear gradient at parameter `t` in [0, 1].
// QLinearGradient has no public "colour at t", and the label ink has to be
// judged at every point of the label's text box (not just at a stop), so the
// gradient is re-sampled here from its stops with the same per-channel linear
// interpolation the painter uses.
QColor gradientColorAt(const QGradientStops &stops, qreal t)
{
    if (stops.isEmpty())
        return QColor();
    if (t <= stops.first().first)
        return stops.first().second;
    if (t >= stops.last().first)
        return stops.last().second;
    for (int k = 1; k < stops.size(); ++k) {
        if (t <= stops.at(k).first) {
            const qreal t0 = stops.at(k - 1).first;
            const qreal t1 = stops.at(k).first;
            const QColor &c0 = stops.at(k - 1).second;
            const QColor &c1 = stops.at(k).second;
            const qreal f = t1 > t0 ? (t - t0) / (t1 - t0) : 0.0;
            return QColor::fromRgbF(c0.redF() + (c1.redF() - c0.redF()) * f,
                                    c0.greenF() + (c1.greenF() - c0.greenF()) * f,
                                    c0.blueF() + (c1.blueF() - c0.blueF()) * f);
        }
    }
    return stops.last().second;
}

// How a strip label should be drawn so it stays readable over its gradient.
struct SpanInk {
    QColor ink;
    bool badge = false; // true -> draw a solid plate behind the text
};

// Choose one ink for a label that spans a gradient. `samples` are the actual
// colours under the label's text box (see samplesForTextBox), all opaque; every
// sample and the ink are flattened onto `base` (the card the strip sits on)
// before its WCAG ratio is measured, so alpha is never ignored.
//
// Order of preference, mirroring inkOver()'s "design-first" policy:
//   1. whichever of the two DESIGN inks has the better worst-case ratio, when
//      that ratio already clears AA across the whole span;
//   2. otherwise pure black / white, when the better of the two clears AA -- so
//      a saturated fill that no design ink can serve keeps an achromatic look;
//   3. otherwise request a badge: a solid `badgeSurface` plate whose own
//      contrast (via inkOver) guarantees AA regardless of the gradient. This is
//      the only case where the design's gradient peek-through is sacrificed.
SpanInk inkForSpan(const QVector<QColor> &samples, const QColor &base,
                   const QColor &badgeSurface)
{
    const QColor designInk = Contrast::bestInkForSpan(samples, base, kDarkInk, kLightInk);
    if (Contrast::worstRatioForInk(samples, base, designInk) >= kAaNormalTextRatio)
        return {designInk, false};

    const double black = Contrast::worstRatioForInk(samples, base, kFallbackBlack);
    const double white = Contrast::worstRatioForInk(samples, base, kFallbackWhite);
    if (std::max(black, white) >= kAaNormalTextRatio)
        return {black >= white ? kFallbackBlack : kFallbackWhite, false};

    // No single ink can cover the span: back the text with a badge plate.
    return {inkOver(badgeSurface, base), true};
}

// Field-by-field equality of two compositions. Used to decide whether a write
// that reported an error actually landed (read-back verification).
bool sameLedState(const Lighting::LedData &a, const Lighting::LedData &b)
{
    return a.effect == b.effect && a.brightness == b.brightness && a.red == b.red
        && a.green == b.green && a.blue == b.blue;
}

} // namespace

// ===========================================================================
// Hand-painted product widgets
// ===========================================================================
namespace LightingUi {

// --- On/off switch ---------------------------------------------------------
class ToggleSwitch : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(qreal knob READ knob WRITE setKnob)

public:
    explicit ToggleSwitch(QWidget *parent = nullptr)
        : QAbstractButton(parent)
    {
        setCheckable(true);
        setFixedSize(48, 28);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_Hover, true);
        setAccessibleName(tr("LED on/off"));
        setAccessibleDescription(
            tr("Turns the selected zone on or off. Off maps to the OFF effect."));
        m_anim = new QPropertyAnimation(this, "knob", this);
        m_anim->setDuration(140);
        m_anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(this, &QAbstractButton::toggled, this,
                [this](bool on) { animateTo(on ? 1.0 : 0.0); });
    }

    qreal knob() const { return m_knob; }
    void setKnob(qreal value)
    {
        m_knob = value;
        update();
    }

    // Programmatic state update: no animation, no toggled() emission.
    void setCheckedSilently(bool on)
    {
        m_anim->stop();
        QAbstractButton::setChecked(on);
        m_knob = on ? 1.0 : 0.0;
        update();
    }

signals:
    void toggled(bool on);

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

        const QColor track = isChecked() ? Theme::accent() : Theme::surfaceElevated();
        QColor border = Theme::border();
        border.setAlpha(190);
        p.setPen(QPen(isChecked() ? Theme::accent() : border, 1));
        p.setBrush(track);
        p.drawRoundedRect(r, r.height() / 2.0, r.height() / 2.0);

        const qreal d = r.height() - 6.0;
        const qreal x0 = r.left() + 3.0;
        const qreal x1 = r.right() - d - 3.0;
        const qreal cx = x0 + (x1 - x0) * m_knob + d / 2.0;
        p.setPen(Qt::NoPen);
        p.setBrush(isChecked() ? QColor(Qt::white) : Theme::textMuted());
        p.drawEllipse(QPointF(cx, r.center().y()), d / 2.0, d / 2.0);

        if (hasFocus()) {
            p.setPen(QPen(Theme::accent(), 2));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(r.adjusted(1.0, 1.0, -1.0, -1.0), r.height() / 2.0,
                              r.height() / 2.0);
        }
    }

    void keyPressEvent(QKeyEvent *e) override
    {
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            click();
            e->accept();
            return;
        }
        QAbstractButton::keyPressEvent(e);
    }

    void enterEvent(QEnterEvent *) override { update(); }
    void leaveEvent(QEvent *) override { update(); }

private:
    void animateTo(qreal target)
    {
        m_anim->stop();
        m_anim->setStartValue(m_knob);
        m_anim->setEndValue(target);
        m_anim->start();
    }

    QPropertyAnimation *m_anim = nullptr;
    qreal m_knob = 0.0;
};

// --- Clickable colour square ----------------------------------------------
class ColorSwatch : public QAbstractButton
{
    Q_OBJECT

public:
    ColorSwatch(const QColor &color, bool large, QWidget *parent = nullptr)
        : QAbstractButton(parent)
        , m_color(color)
        , m_large(large)
    {
        setCheckable(true); // so setSelected() can render the accent ring
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_Hover, true);
        setAccessibleName(large ? tr("Selected colour") : tr("Colour preset"));
        setAccessibleDescription(tr("Sets the LED colour."));
        setToolTip(color.name(QColor::HexRgb));
    }

    QColor color() const { return m_color; }
    void setColor(const QColor &c)
    {
        if (c == m_color)
            return;
        m_color = c;
        setToolTip(c.name(QColor::HexRgb));
        update();
    }
    void setSelected(bool selected)
    {
        if (selected == isChecked())
            return;
        setChecked(selected);
        update();
    }

    QSize sizeHint() const override { return m_large ? QSize(46, 46) : QSize(30, 30); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

        p.setPen(Qt::NoPen);
        p.setBrush(m_color);
        p.drawRoundedRect(r, Theme::RadiusSmall, Theme::RadiusSmall);

        QColor border = Theme::border();
        QPen pen(border, 1);
        if (isChecked())
            pen = QPen(Theme::accent(), 2);
        else if (underMouse())
            pen = QPen(Theme::textMuted(), 1);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r, Theme::RadiusSmall, Theme::RadiusSmall);

        if (hasFocus()) {
            p.setPen(QPen(Theme::accent(), 2, Qt::DotLine));
            p.drawRoundedRect(r.adjusted(2.5, 2.5, -2.5, -2.5), Theme::RadiusSmall - 2,
                              Theme::RadiusSmall - 2);
        }
    }

    void enterEvent(QEnterEvent *) override { update(); }
    void leaveEvent(QEvent *) override { update(); }

private:
    QColor m_color;
    bool m_large;
};

// --- Zone chip -------------------------------------------------------------
class ZoneChip : public QAbstractButton
{
    Q_OBJECT

public:
    ZoneChip(Lighting::Zone zone, const QString &label, QWidget *parent = nullptr)
        : QAbstractButton(parent)
        , m_label(label)
        , m_zone(zone)
    {
        setCheckable(true);
        setText(label);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_Hover, true);
        setFixedHeight(36);
        setFont(Theme::font(11, QFont::Medium));
        setAccessibleDescription(tr("Selects this LED zone."));
    }

    Lighting::Zone zone() const { return m_zone; }

    void setSelected(bool selected)
    {
        if (selected == isChecked())
            return;
        setChecked(selected);
        update();
    }

    QSize sizeHint() const override
    {
        const QFontMetrics fm(font());
        return QSize(fm.horizontalAdvance(m_label) + 42, 36);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

        p.setPen(Qt::NoPen);
        p.setBrush(Theme::surfaceElevated());
        p.drawRoundedRect(r, Theme::RadiusSmall, Theme::RadiusSmall);

        QColor border = Theme::border();
        border.setAlpha(190);
        QPen pen(border, 1);
        if (isChecked())
            pen = QPen(Theme::accent(), 2);
        else if (underMouse())
            pen = QPen(Theme::textMuted(), 1);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r, Theme::RadiusSmall, Theme::RadiusSmall);

        // Accent dot marks the active chip at a glance.
        p.setPen(Qt::NoPen);
        p.setBrush(isChecked() ? Theme::accent() : Theme::textMuted());
        p.drawEllipse(QPointF(r.left() + 15.0, r.center().y()), 3.0, 3.0);

        p.setPen(isChecked() ? Theme::textPrimary() : Theme::textSecondary());
        p.setFont(font());
        p.drawText(r.adjusted(27.0, 0.0, -12.0, 0.0), Qt::AlignLeft | Qt::AlignVCenter, m_label);

        if (hasFocus()) {
            p.setPen(QPen(Theme::accent(), 2));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(r.adjusted(1.0, 1.0, -1.0, -1.0), Theme::RadiusSmall,
                              Theme::RadiusSmall);
        }
    }

    void enterEvent(QEnterEvent *) override { update(); }
    void leaveEvent(QEvent *) override { update(); }

private:
    QString m_label;
    Lighting::Zone m_zone;
};

// --- Effect card -----------------------------------------------------------
class EffectCard : public QAbstractButton
{
    Q_OBJECT

public:
    EffectCard(Lighting::Effect effect, const QString &label, QWidget *parent = nullptr)
        : QAbstractButton(parent)
        , m_effect(effect)
        , m_label(label)
    {
        setCheckable(true);
        setText(label);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_Hover, true);
        setFixedSize(112, 108);
        setFont(Theme::font(11, QFont::Medium));
        setAccessibleDescription(tr("Applies this lighting effect."));
    }

    Lighting::Effect effect() const { return m_effect; }
    void setSelected(bool selected)
    {
        if (selected == isChecked())
            return;
        setChecked(selected);
        update();
    }

    // Colours the little preview glyph so each card hints at the current colour
    // and dims as brightness drops.
    void setHint(const Lighting::LedData &data)
    {
        m_hintColor = representativeColor(data);
        m_effectActive = data.effect != Lighting::Effect::Off;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

        p.setPen(Qt::NoPen);
        p.setBrush(Theme::surfaceElevated());
        p.drawRoundedRect(r, Theme::Radius, Theme::Radius);

        QColor border = Theme::border();
        border.setAlpha(190);
        QPen pen(border, 1);
        if (isChecked())
            pen = QPen(Theme::accent(), 2);
        else if (underMouse())
            pen = QPen(Theme::textMuted(), 1);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r, Theme::Radius, Theme::Radius);

        drawHint(p, QRectF(r.left() + 16.0, r.top() + 14.0, r.width() - 32.0, 46.0));

        p.setPen(isChecked() ? Theme::textPrimary() : Theme::textSecondary());
        p.setFont(font());
        p.drawText(QRectF(r.left() + 6.0, r.top() + 68.0, r.width() - 12.0, 30.0),
                   Qt::AlignHCenter | Qt::AlignVCenter, m_label);

        if (hasFocus()) {
            p.setPen(QPen(Theme::accent(), 2));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(r.adjusted(1.0, 1.0, -1.0, -1.0), Theme::Radius, Theme::Radius);
        }
    }

    void enterEvent(QEnterEvent *) override { update(); }
    void leaveEvent(QEvent *) override { update(); }

private:
    void drawHint(QPainter &p, const QRectF &box)
    {
        const QPointF c = box.center();
        const qreal s = qMin(box.width(), box.height());
        const QColor hint = m_effectActive ? m_hintColor : Theme::textMuted();

        switch (m_effect) {
        case Lighting::Effect::Off: {
            p.setPen(QPen(hint, 2.0));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(c, s * 0.30, s * 0.30);
            p.drawLine(c + QPointF(-s * 0.22, s * 0.22), c + QPointF(s * 0.22, -s * 0.22));
            break;
        }
        case Lighting::Effect::Static: {
            p.setPen(Qt::NoPen);
            p.setBrush(hint);
            p.drawEllipse(c, s * 0.30, s * 0.30);
            break;
        }
        case Lighting::Effect::Breathing: {
            p.setBrush(Qt::NoBrush);
            for (int i = 0; i < 3; ++i) {
                QColor ring = hint;
                ring.setAlpha(220 - i * 60);
                p.setPen(QPen(ring, 2.0));
                p.drawEllipse(c, s * (0.18 + i * 0.11), s * (0.18 + i * 0.11));
            }
            break;
        }
        case Lighting::Effect::ColorfulCycle: {
            p.setPen(Qt::NoPen);
            for (int i = 0; i < 6; ++i) {
                p.setBrush(QColor::fromHsvF(static_cast<qreal>(i) / 6.0, 0.75, 0.95));
                const qreal a = 2.0 * M_PI * i / 6.0;
                p.drawEllipse(c + QPointF(std::cos(a), std::sin(a)) * (s * 0.17), s * 0.11,
                              s * 0.11);
            }
            break;
        }
        case Lighting::Effect::Ambilight: {
            p.setPen(Qt::NoPen);
            const qreal barW = s * 0.10;
            const qreal gap = s * 0.06;
            qreal x = c.x() - (4.0 * barW + 3.0 * gap) / 2.0;
            for (int i = 0; i < 4; ++i) {
                const qreal h = s * (0.30 + 0.16 * i);
                QColor bar = hint;
                bar.setAlpha(180 - i * 30);
                p.setBrush(bar);
                p.drawRoundedRect(QRectF(x, c.y() - h / 2.0, barW, h), barW / 2.0, barW / 2.0);
                x += barW + gap;
            }
            break;
        }
        }
    }

    Lighting::Effect m_effect;
    QString m_label;
    QColor m_hintColor = Theme::accent();
    bool m_effectActive = false;
};

// --- Glow preview plate ----------------------------------------------------
class GlowPreview : public QWidget
{
    Q_OBJECT

public:
    explicit GlowPreview(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(84);
        setAccessibleName(tr("Colour preview"));
        setAccessibleDescription(tr("Approximate appearance of the selected lighting."));
    }

    void setState(const Lighting::LedData &data)
    {
        m_data = data;
        update();
    }

    QSize sizeHint() const override { return QSize(140, 90); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

        p.setPen(Qt::NoPen);
        p.setBrush(Theme::background());
        p.drawRoundedRect(r, Theme::RadiusSmall, Theme::RadiusSmall);

        const bool on = m_data.effect != Lighting::Effect::Off;
        const qreal b = static_cast<qreal>(m_data.brightness) / Lighting::kBrightnessMax;

        QColor glow(Theme::textMuted());
        if (on) {
            glow = QColor(m_data.red, m_data.green, m_data.blue);
            if (m_data.effect == Lighting::Effect::ColorfulCycle)
                glow = QColor::fromHsvF(0.85, 0.70, 1.0);
        }

        QRadialGradient grad(r.center(), r.width() * 0.55);
        QColor inner = glow;
        inner.setAlphaF(on ? qBound(0.05, 0.25 + 0.75 * b, 0.95) : 0.05);
        QColor outer = glow;
        outer.setAlpha(0);
        grad.setColorAt(0.0, inner);
        grad.setColorAt(1.0, outer);
        p.setBrush(grad);
        p.drawRoundedRect(r, Theme::RadiusSmall, Theme::RadiusSmall);

        // A quiet centre bar, so "off" still reads as a plate.
        QColor bar = glow;
        bar.setAlphaF(on ? qBound(0.30, 0.40 + 0.60 * b, 1.0) : 0.18);
        p.setBrush(bar);
        p.drawRoundedRect(QRectF(r.center().x() - r.width() * 0.16, r.center().y() - 5.0,
                                 r.width() * 0.32, 10.0),
                          5.0, 5.0);
    }

private:
    Lighting::LedData m_data;
};

// --- Representative zone schematic ----------------------------------------
class Schematic : public QWidget
{
    Q_OBJECT

public:
    explicit Schematic(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(210);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setFocusPolicy(Qt::StrongFocus);
        setAccessibleName(tr("Representative zone schematic"));
        setAccessibleDescription(tr(
            "This is not a live hardware view. Every region is filled with the currently "
            "selected zone's edit and preview values only, so other regions do not show their "
            "own real state, and it is not a physical key map. Click a region to select its "
            "zone; the A/B/C positions are illustrative and the physical key layout is not "
            "verified. Use the arrow keys to cycle zones."));
    }

    void setAvailableZones(const QVector<Lighting::Zone> &zones)
    {
        m_zones = zones;
        m_hover = -1;
        update();
    }

    void setData(const Lighting::LedData &data)
    {
        m_data = data;
        update();
    }

    void setSelected(Lighting::Zone zone)
    {
        m_selected = zone;
        update();
    }

    Lighting::Zone selected() const { return m_selected; }

signals:
    void zoneActivated(Lighting::Zone zone);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void leaveEvent(QEvent *) override;
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *) override { update(); }
    void focusOutEvent(QFocusEvent *) override { update(); }

private:
    struct Regions {
        QRectF logo;
        QRectF keyboard;
        QRectF stripA;
        QRectF stripB;
        QRectF stripC;
        QRectF trunk;
    };

    Regions layoutRegions() const;
    bool has(Lighting::Zone zone) const { return m_zones.contains(zone); }
    Lighting::Zone zoneAt(const QPointF &p) const;
    void drawRegion(QPainter &p, const QRectF &r, Lighting::Zone zone, const QString &label,
                    const QBrush &fill, const QColor &ink, bool badge);

    QVector<Lighting::Zone> m_zones;
    Lighting::LedData m_data;
    Lighting::Zone m_selected = Lighting::Zone::All;
    Lighting::Zone m_hoverZone = Lighting::Zone::All;
    int m_hover = -1;
};

Schematic::Regions Schematic::layoutRegions() const
{
    const QRectF a = QRectF(rect()).adjusted(16.0, 16.0, -16.0, -16.0);
    const qreal topH = a.height() * 0.24;
    const qreal logoW = qMin(a.width() * 0.24, 130.0);

    Regions reg;
    reg.logo = QRectF(a.left(), a.top(), logoW, topH);
    reg.keyboard = QRectF(a.left() + logoW + 14.0, a.top(), a.width() - logoW - 14.0, topH);

    const qreal trunkH = a.height() * 0.16;
    reg.trunk = QRectF(a.left(), a.bottom() - trunkH, a.width(), trunkH);

    const qreal midTop = reg.keyboard.bottom() + 18.0;
    const qreal midBottom = reg.trunk.top() - 18.0;
    const qreal gap = 16.0;
    const qreal sw = (a.width() - 2.0 * gap) / 3.0;
    const qreal mh = qMax(24.0, midBottom - midTop);
    reg.stripA = QRectF(a.left(), midTop, sw, mh);
    reg.stripB = QRectF(a.left() + sw + gap, midTop, sw, mh);
    reg.stripC = QRectF(a.left() + 2.0 * (sw + gap), midTop, sw, mh);
    return reg;
}

Lighting::Zone Schematic::zoneAt(const QPointF &p) const
{
    const Regions reg = layoutRegions();
    if (has(Lighting::Zone::Logo) && reg.logo.contains(p))
        return Lighting::Zone::Logo;
    if (has(Lighting::Zone::Keyboard) && reg.keyboard.contains(p))
        return Lighting::Zone::Keyboard;
    if (has(Lighting::Zone::ZoneA) && reg.stripA.contains(p))
        return Lighting::Zone::ZoneA;
    if (has(Lighting::Zone::ZoneB) && reg.stripB.contains(p))
        return Lighting::Zone::ZoneB;
    if (has(Lighting::Zone::ZoneC) && reg.stripC.contains(p))
        return Lighting::Zone::ZoneC;
    if (has(Lighting::Zone::Trunk) && reg.trunk.contains(p))
        return Lighting::Zone::Trunk;
    return Lighting::Zone::All;
}

void Schematic::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;
    const QPointF pos = e->position();
    const Regions reg = layoutRegions();
    const bool onZone = (has(Lighting::Zone::Logo) && reg.logo.contains(pos))
        || (has(Lighting::Zone::Keyboard) && reg.keyboard.contains(pos))
        || (has(Lighting::Zone::ZoneA) && reg.stripA.contains(pos))
        || (has(Lighting::Zone::ZoneB) && reg.stripB.contains(pos))
        || (has(Lighting::Zone::ZoneC) && reg.stripC.contains(pos))
        || (has(Lighting::Zone::Trunk) && reg.trunk.contains(pos));
    if (!onZone)
        return;
    setFocus(Qt::MouseFocusReason);
    emit zoneActivated(zoneAt(pos));
    e->accept();
}

void Schematic::mouseMoveEvent(QMouseEvent *e)
{
    const int idx = m_zones.indexOf(zoneAt(e->position()));
    if (idx != m_hover) {
        m_hover = idx;
        if (idx >= 0)
            m_hoverZone = m_zones.at(idx);
        setCursor(idx >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
}

void Schematic::leaveEvent(QEvent *)
{
    if (m_hover >= 0) {
        m_hover = -1;
        update();
    }
}

void Schematic::keyPressEvent(QKeyEvent *e)
{
    if (m_zones.isEmpty()) {
        QWidget::keyPressEvent(e);
        return;
    }
    const int n = m_zones.size();
    const int idx = m_zones.indexOf(m_selected);
    if (e->key() == Qt::Key_Right || e->key() == Qt::Key_Down) {
        const int base = idx < 0 ? -1 : idx;
        emit zoneActivated(m_zones.at((base + 1 + n) % n));
        e->accept();
        return;
    }
    if (e->key() == Qt::Key_Left || e->key() == Qt::Key_Up) {
        const int base = idx < 0 ? 0 : idx;
        emit zoneActivated(m_zones.at((base - 1 + n) % n));
        e->accept();
        return;
    }
    QWidget::keyPressEvent(e);
}

void Schematic::drawRegion(QPainter &p, const QRectF &r, Lighting::Zone zone, const QString &label,
                           const QBrush &fill, const QColor &ink, bool badge)
{
    const bool available = has(zone);
    const bool selected = (m_selected == zone);
    const bool hovered = (m_hover >= 0 && m_hoverZone == zone && available);

    const qreal radius = qMin(10.0, r.height() / 3.0);

    if (available) {
        QColor border = Theme::border();
        border.setAlpha(190);
        QPen pen(border, 1);
        if (selected)
            pen = QPen(Theme::accent(), 2);
        else if (hovered)
            pen = QPen(Theme::textMuted(), 1);
        p.setPen(pen);
        p.setBrush(fill);
    } else {
        // Not present on this machine: dashed outline, no fill.
        p.setPen(QPen(Theme::textMuted(), 1, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
    }
    p.drawRoundedRect(r, radius, radius);

    const QFont labelFont = Theme::labelFont(9, QFont::DemiBold, 130);

    // A badge is only drawn when no single ink can clear WCAG AA across the
    // label's whole gradient slice (see inkForSpan). The plate is a solid theme
    // surface that the ink was measured against, so the text is readable no
    // matter what the gradient beneath it does. It is kept tight around the
    // glyphs so it reads as a zone tag, not a panel.
    if (available && badge) {
        const QFontMetricsF fm(labelFont);
        const qreal padX = qMax<qreal>(6.0, fm.height() * 0.45);
        const qreal padY = qMax<qreal>(2.0, fm.height() * 0.14);
        QRectF plate(0, 0, fm.horizontalAdvance(label) + 2.0 * padX, fm.height() + 2.0 * padY);
        plate.moveCenter(r.center());
        QColor plateBorder = Theme::border();
        plateBorder.setAlpha(210);
        p.setPen(QPen(plateBorder, 1));
        p.setBrush(Theme::surface());
        p.drawRoundedRect(plate, plate.height() / 2.0, plate.height() / 2.0);
    }

    // Readable label over the (arbitrary) fill colour; muted when unavailable.
    p.setPen(available ? ink : Theme::textMuted());
    p.setFont(labelFont);
    p.drawText(r, Qt::AlignCenter, label);
}

void Schematic::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF body = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.setPen(Qt::NoPen);
    p.setBrush(Theme::surfaceElevated());
    p.drawRoundedRect(body, Theme::Radius, Theme::Radius);

    const Regions reg = layoutRegions();
    const QColor base = representativeColor(m_data);
    const bool cycle = m_data.effect == Lighting::Effect::ColorfulCycle;
    const bool ambi = m_data.effect == Lighting::Effect::Ambilight;

    // Gradient stops (position, colour) for a strip. Shared by the brush and by
    // the label-ink sampling below, so the two can never drift apart. Empty for
    // a flat (non-gradient) fill.
    const auto stripStops = [&](int i) -> QGradientStops {
        QGradientStops stops;
        if (cycle) {
            stops << QGradientStop(0.00, QColor::fromHsvF(0.00, 0.70, 0.90))
                  << QGradientStop(0.33, QColor::fromHsvF(0.33, 0.70, 0.90))
                  << QGradientStop(0.66, QColor::fromHsvF(0.66, 0.70, 0.90))
                  << QGradientStop(1.00, QColor::fromHsvF(0.95, 0.70, 0.90));
        } else if (ambi) {
            const qreal hue = base.hsvHueF() < 0.0 ? 0.0 : base.hsvHueF();
            stops << QGradientStop(0.0, QColor::fromHsvF(hue, 0.55, 0.95))
                  << QGradientStop(
                         1.0, QColor::fromHsvF(std::fmod(hue + 0.08 * (i + 1), 1.0), 0.55, 0.75));
        }
        return stops;
    };

    const auto stripBrush = [&](const QRectF &r, int i) -> QBrush {
        const QGradientStops stops = stripStops(i);
        if (stops.isEmpty())
            return QBrush(base);
        QLinearGradient g(r.topLeft(), r.topRight());
        g.setStops(stops);
        return QBrush(g);
    };

    // Keyboard region: a small key grid behind the label, so it reads as a
    // keyboard without claiming exact positions.
    if (has(Lighting::Zone::Keyboard)) {
        const qreal keysW = reg.keyboard.width() * 0.60;
        const qreal keysH = reg.keyboard.height() * 0.50;
        const QRectF keys(reg.keyboard.left() + (reg.keyboard.width() - keysW) / 2.0,
                          reg.keyboard.top() + 8.0, keysW, keysH);
        QColor key = base;
        key.setAlpha(140);
        p.setPen(Qt::NoPen);
        p.setBrush(key);
        const int cols = 8;
        const int rows = 2;
        const qreal kw = keys.width() / cols;
        const qreal kh = keys.height() / rows;
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < cols; ++x)
                p.drawRoundedRect(QRectF(keys.left() + x * kw + 1.0, keys.top() + y * kh + 1.0,
                                         kw - 2.0, kh - 2.0),
                                  1.5, 1.5);
    }

    // Every region fill sits on the schematic card, so that is the base the ink
    // has to read against once the (possibly translucent) fill is composited.
    const QColor cardBase = Theme::surfaceElevated();
    const QColor solidInk = inkOver(base, cardBase);

    // The label sits centred in its region and the strip gradient runs
    // left->right, so the label's text box straddles a horizontal SLICE of it.
    // Sample that whole slice (plus a little padding for the antialiased edges):
    // judging a single "representative" stop is exactly the F3 bug, because the
    // slice's worst point is not the brightest stop.
    const QFont stripLabelFont = Theme::labelFont(9, QFont::DemiBold, 130);
    const QFontMetricsF stripLabelMetrics(stripLabelFont);

    const auto samplesForTextBox = [&](const QRectF &r, const QString &label,
                                       const QGradientStops &stops) -> QVector<QColor> {
        QVector<QColor> samples;
        if (stops.isEmpty() || r.width() <= 0.0)
            return samples;
        QRectF text(0, 0, stripLabelMetrics.horizontalAdvance(label), stripLabelMetrics.height());
        text.moveCenter(r.center());
        const qreal pad = qMax<qreal>(2.0, text.height() * 0.15);
        const qreal x0 = text.left() - pad;
        const qreal x1 = text.right() + pad;
        constexpr int kSamples = 12;
        for (int k = 0; k <= kSamples; ++k) {
            const qreal x = x0 + (x1 - x0) * (static_cast<qreal>(k) / kSamples);
            const qreal t = qBound<qreal>(0.0, (x - r.left()) / r.width(), 1.0);
            samples << gradientColorAt(stops, t);
        }
        return samples;
    };

    // Each strip gets its OWN ink, chosen from its own gradient slice -- the
    // three strips have different gradients, so the single shared ink the old
    // code computed from the union of all stops could not serve all of them.
    const auto stripInkFor = [&](const QRectF &r, const QString &label, int i) -> SpanInk {
        const QGradientStops stops = stripStops(i);
        if (stops.isEmpty())
            return {solidInk, false};
        const QVector<QColor> samples = samplesForTextBox(r, label, stops);
        return inkForSpan(samples, cardBase, Theme::surface());
    };

    const SpanInk inkA = stripInkFor(reg.stripA, QStringLiteral("A"), 0);
    const SpanInk inkB = stripInkFor(reg.stripB, QStringLiteral("B"), 1);
    const SpanInk inkC = stripInkFor(reg.stripC, QStringLiteral("C"), 2);

    drawRegion(p, reg.logo, Lighting::Zone::Logo, QStringLiteral("Logo"), QBrush(base), solidInk,
               false);
    drawRegion(p, reg.keyboard, Lighting::Zone::Keyboard,
               zoneDisplayName(Lighting::Zone::Keyboard), QBrush(base), solidInk, false);
    drawRegion(p, reg.stripA, Lighting::Zone::ZoneA, QStringLiteral("A"), stripBrush(reg.stripA, 0),
               inkA.ink, inkA.badge);
    drawRegion(p, reg.stripB, Lighting::Zone::ZoneB, QStringLiteral("B"), stripBrush(reg.stripB, 1),
               inkB.ink, inkB.badge);
    drawRegion(p, reg.stripC, Lighting::Zone::ZoneC, QStringLiteral("C"), stripBrush(reg.stripC, 2),
               inkC.ink, inkC.badge);
    drawRegion(p, reg.trunk, Lighting::Zone::Trunk, QStringLiteral("Trunk"), QBrush(base),
               solidInk, false);

    if (hasFocus()) {
        p.setPen(QPen(Theme::accent(), 2, Qt::DotLine));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(body.adjusted(3.0, 3.0, -3.0, -3.0), Theme::Radius - 2, Theme::Radius - 2);
    }
}

} // namespace LightingUi

// ===========================================================================
// A SurfacePanel with a warning-coloured bar on the left, used for the
// "protocol not verified" note.
// ===========================================================================
namespace {
class WarningPanel : public SurfacePanel
{
    Q_OBJECT

protected:
    void paintOverlay(QPainter &p, const QRectF &surface) override
    {
        QPainterPath clip;
        clip.addRoundedRect(surface, Theme::Radius, Theme::Radius);
        p.save();
        p.setClipPath(clip);
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::warning());
        p.drawRect(QRectF(surface.left(), surface.top(), 3.0, surface.height()));
        p.restore();
    }
};

// ActionButton paints the same face whether or not it is enabled, which would
// make a blocked "Apply" look actionable. This thin subclass keeps the product
// styling for the enabled case and draws a clearly dimmed face when disabled, so
// the dry-run state is honest.
class GateButton : public ActionButton
{
public:
    using ActionButton::ActionButton;

protected:
    void paintEvent(QPaintEvent *e) override
    {
        if (isEnabled()) {
            ActionButton::paintEvent(e);
            return;
        }
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

        QColor bg = Theme::surfaceElevated();
        bg.setAlpha(120);
        QColor border = Theme::border();
        border.setAlpha(120);
        p.setPen(QPen(border, 1));
        p.setBrush(bg);
        p.drawRoundedRect(r, Theme::RadiusSmall, Theme::RadiusSmall);

        p.setPen(Theme::textMuted());
        p.setFont(font());
        p.drawText(rect(), Qt::AlignCenter, text());
    }
};

} // namespace

// ===========================================================================
// LightingPage
// ===========================================================================
namespace {

QString ledBasePath()
{
#ifdef EXCALIBUR_DEV_HOOKS
    const QString env = qEnvironmentVariable("EXCALIBUR_LED_BASE");
    if (!env.isEmpty())
        return env;
#endif
    return QStringLiteral("/sys/class/leds");
}

} // namespace

LightingPage::LightingPage(QWidget *parent)
    : QWidget(parent)
    , m_client(ledBasePath())
{
    // Starting composition for a fresh, unread zone: a static, mid-bright accent
    // colour. Purely in-memory; nothing is written to any device.
    m_data.effect = Lighting::Effect::Static;
    m_data.brightness = 8;
    m_data.red = 0xFF;
    m_data.green = 0x4D;
    m_data.blue = 0x3D;
    m_lastOn = m_data;

    buildUi();
    rescan();

#ifdef EXCALIBUR_DEV_HOOKS
    // --- dev-hook seam (EXCALIBUR_DEV_HOOKS only) --------------------------
    // Lets a single, automated offscreen run open the application write gate and
    // Apply once, so the write outcome (Applied / Partial write / Write failed /
    // Not applied) can be captured as a screenshot without a user session. Both
    // variables are unset in normal use, and this whole block is compiled out of
    // a release build (EXCALIBUR_DEV_HOOKS is OFF by default), so the production
    // path is unchanged.
    //   EXCALIBUR_UI_GATE=1     open the in-session application gate
    //   EXCALIBUR_AUTO_APPLY=1  also Apply once, right after the gate opens
    // The kernel gate is still required and is read from EXCALIBUR_KERNEL_GATE_FILE
    // (existing seam); the client's write switch is still only set by refreshGate().
    const auto devFlag = [](const char *name) {
        const QString v = qEnvironmentVariable(name).trimmed().toLower();
        return !v.isEmpty() && v != QLatin1String("0") && v != QLatin1String("false");
    };
    if (devFlag("EXCALIBUR_UI_GATE")) {
        m_uiGate = true;
        if (m_uiGateBox) {
            // Route through the checkbox, exactly as a real click would, so the
            // toggled() slot runs refreshGate() and grants the client its write
            // switch. Nothing here bypasses the two-gate rule.
            m_uiGateBox->setChecked(true);
        }
        refreshGate();

        if (devFlag("EXCALIBUR_AUTO_APPLY")) {
            // The page hydrates its editor from the device on load, so applying
            // the hydrated values would be indistinguishable from a no-op.
            // Compose a target that differs from the device's starting state in
            // every field, so whichever attribute the device refuses to accept
            // leaves an observable partial state.
            m_data.effect = Lighting::Effect::Breathing;
            m_data.brightness = 3;
            m_data.red = 0x09;
            m_data.green = 0x09;
            m_data.blue = 0x09;
            applyStateToControls();
            refreshPreview();
            // Deferred by one event-loop turn so the widget tree has been laid
            // out once before Apply runs -- a closer match to a user clicking
            // the button on a visible window.
            QTimer::singleShot(0, this, [this]() { onApply(); });
        }
    }
#endif // EXCALIBUR_DEV_HOOKS
}

QString LightingPage::kernelGatePath() const
{
#ifdef EXCALIBUR_DEV_HOOKS
    const QString env = qEnvironmentVariable("EXCALIBUR_KERNEL_GATE_FILE");
    if (!env.isEmpty())
        return env;
#endif
    return QString::fromLatin1(kKernelGateDefault);
}

LightingPage::KernelGate LightingPage::readKernelGate() const
{
    KernelGate gate;
    const QString path = kernelGatePath();
    gate.exists = QFileInfo::exists(path);
    if (!gate.exists)
        return gate;

    QFile f(path);
    // READ-ONLY — this call never opens the parameter for writing.
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return gate;
    gate.value = QString::fromUtf8(f.readAll()).trimmed();
    const QString v = gate.value.toLower();
    gate.open = (v == QLatin1String("y") || v == QLatin1String("1") || v == QLatin1String("true")
                 || v == QLatin1String("on") || v == QLatin1String("yes"));
    return gate;
}

bool LightingPage::writeGateEffective() const
{
    // The application gate can never exceed the kernel gate: BOTH must be open.
    return m_uiGate && m_kernelGateOpen;
}

void LightingPage::buildUi()
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    outer->addWidget(buildHeader());

    m_stack = new QStackedWidget;
    m_content = buildContent();

    m_empty = new EmptyState;
    m_empty->setMessage(
        tr("LED interface unavailable"),
        tr("No lighting control interface was found. The prototype led-class-multicolor "
           "module (excalibur::* LEDs) is not loaded, so there is nothing to configure. "
           "Nothing is written to the hardware."));

    m_stack->addWidget(m_content);
    m_stack->addWidget(m_empty);
    outer->addWidget(m_stack, 1);
}

QWidget *LightingPage::buildHeader()
{
    auto *header = new QWidget;
    auto *hv = PageUtils::makeCenteredColumn(header, PageUtils::kMaxContentWidth);
    hv->setContentsMargins(Theme::Space::XXL, Theme::Space::XL, Theme::Space::XXL, Theme::Space::M);
    hv->setSpacing(Theme::Space::S);

    hv->addWidget(PageUtils::pageTitle(tr("Lighting")));
    hv->addWidget(PageUtils::pageSubtitle(
        tr("Compose an effect, colour and brightness for each LED zone. Nothing is written "
           "to the hardware unless both write gates are open.")));

    auto *row = new QHBoxLayout;
    row->setContentsMargins(0, Theme::Space::XS, 0, 0);
    row->setSpacing(Theme::Space::M);

    m_availabilityPill = new StatusPill;
    m_rescan = new ActionButton(tr("Rescan"));
    m_rescan->setAccessibleName(tr("Rescan for LED zones"));
    connect(m_rescan, &ActionButton::clicked, this, &LightingPage::rescan);

    row->addWidget(m_availabilityPill, 0, Qt::AlignVCenter);
    row->addStretch(1);
    row->addWidget(m_rescan, 0, Qt::AlignVCenter);
    hv->addLayout(row);
    return header;
}

QWidget *LightingPage::buildContent()
{
    auto *scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *inner = new QWidget;
    auto *root = PageUtils::makeCenteredColumn(inner, PageUtils::kMaxContentWidth);
    root->setContentsMargins(Theme::Space::XXL, Theme::Space::M, Theme::Space::XXL, Theme::Space::XL);
    root->setSpacing(Theme::Space::L);

    // --- ZONES -------------------------------------------------------------
    {
        auto *head = new QHBoxLayout;
        head->setContentsMargins(0, 0, 0, 0);
        head->setSpacing(Theme::Space::S);
        head->addWidget(sectionLabel(tr("ZONES")));
        head->addStretch(1);
        m_zoneSummary = new QLabel;
        m_zoneSummary->setFont(Theme::font(11, QFont::DemiBold));
        {
            QPalette p = m_zoneSummary->palette();
            p.setColor(QPalette::WindowText, Theme::accent());
            m_zoneSummary->setPalette(p);
        }
        head->addWidget(m_zoneSummary, 0, Qt::AlignVCenter);
        root->addLayout(head);

        m_chipHost = new QWidget;
        m_chipLayout = new QHBoxLayout(m_chipHost);
        m_chipLayout->setContentsMargins(0, 0, 0, 0);
        m_chipLayout->setSpacing(Theme::Space::S);
        root->addWidget(m_chipHost);
    }

    // --- SCHEMATIC ---------------------------------------------------------
    {
        root->addWidget(sectionLabel(tr("SCHEMATIC")));
        auto *panel = new SurfacePanel;
        auto *pv = new QVBoxLayout(panel);
        pv->setContentsMargins(Theme::Space::L, Theme::Space::L, Theme::Space::L, Theme::Space::M);
        pv->setSpacing(Theme::Space::S);

        m_schematic = new LightingUi::Schematic;
        connect(m_schematic, &LightingUi::Schematic::zoneActivated, this,
                &LightingPage::selectZone);
        pv->addWidget(m_schematic);

        auto *note = PageUtils::makeLabel(
            tr("Representative schematic — NOT a live hardware view and NOT a physical key map. "
               "Every region is painted with the SELECTED zone's current edit/preview values "
               "only, so the other regions do not show their own real state. The A/B/C "
               "positions and letters are illustrative; the zone-to-key mapping is not "
               "verified. Nothing here reflects the device until Apply succeeds."),
            Theme::font(10, QFont::Normal), Theme::textMuted());
        note->setWordWrap(true);
        pv->addWidget(note);
        root->addWidget(panel);
    }

    root->addWidget(buildControls());
    root->addWidget(buildPreview());
    root->addWidget(buildGate());
    root->addWidget(buildProtocolNote());

    root->addStretch(1);
    scroll->setWidget(inner);
    return scroll;
}

QWidget *LightingPage::buildControls()
{
    auto *panel = new SurfacePanel;
    m_controlsPanel = panel;
    auto *v = new QVBoxLayout(panel);
    v->setContentsMargins(Theme::Space::XL, Theme::Space::L, Theme::Space::XL, Theme::Space::L);
    v->setSpacing(Theme::Space::L);

    // On/off row.
    {
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(Theme::Space::M);
        m_toggle = new LightingUi::ToggleSwitch;
        connect(m_toggle, &LightingUi::ToggleSwitch::toggled, this, &LightingPage::onToggle);
        row->addWidget(m_toggle, 0, Qt::AlignVCenter);

        auto *texts = new QVBoxLayout;
        texts->setContentsMargins(0, 0, 0, 0);
        texts->setSpacing(1);
        texts->addWidget(PageUtils::makeLabel(tr("LED on/off"), Theme::font(12, QFont::DemiBold),
                                              Theme::textPrimary()));
        texts->addWidget(PageUtils::makeLabel(
            tr("Switches the selected zone between the OFF effect and the last live setting."),
            Theme::font(10, QFont::Normal), Theme::textSecondary()));
        row->addLayout(texts);
        row->addStretch(1);
        v->addLayout(row);
    }

    // Colour row: main swatch + presets + dialog.
    {
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(Theme::Space::S);
        row->addWidget(PageUtils::makeLabel(tr("Colour"), Theme::font(11, QFont::Medium),
                                            Theme::textSecondary()));

        m_mainSwatch = new LightingUi::ColorSwatch(QColor(m_data.red, m_data.green, m_data.blue),
                                                   true);
        row->addWidget(m_mainSwatch, 0, Qt::AlignVCenter);

        for (const char *hex : kPaletteHex) {
            auto *sw = new LightingUi::ColorSwatch(QColor(QString::fromLatin1(hex)), false);
            connect(sw, &QAbstractButton::clicked, this, [this, sw]() { onColorPicked(sw->color()); });
            m_palette.append(sw);
            row->addWidget(sw, 0, Qt::AlignVCenter);
        }

        row->addStretch(1);
        m_chooseColor = new ActionButton(tr("Choose colour…"));
        connect(m_chooseColor, &ActionButton::clicked, this, &LightingPage::onChooseColor);
        row->addWidget(m_chooseColor, 0, Qt::AlignVCenter);
        v->addLayout(row);

        auto *rgb = new QHBoxLayout;
        rgb->setContentsMargins(0, 0, 0, 0);
        rgb->setSpacing(Theme::Space::S);
        const auto makeSpin = [this, rgb](const QString &label) {
            rgb->addWidget(PageUtils::makeLabel(label, Theme::font(11, QFont::DemiBold),
                                                Theme::textMuted()));
            auto *spin = new QSpinBox;
            spin->setRange(0, 255);
            spin->setFixedWidth(74);
            spin->setAccessibleName(label);
            connect(spin, &QSpinBox::valueChanged, this, &LightingPage::onRgbEdited);
            rgb->addWidget(spin);
            return spin;
        };
        m_red = makeSpin(QStringLiteral("R"));
        m_green = makeSpin(QStringLiteral("G"));
        m_blue = makeSpin(QStringLiteral("B"));
        rgb->addStretch(1);
        v->addLayout(rgb);
    }

    // Brightness row.
    {
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(Theme::Space::M);
        row->addWidget(PageUtils::makeLabel(tr("Brightness"), Theme::font(11, QFont::Medium),
                                            Theme::textSecondary()));
        m_brightness = new QSlider(Qt::Horizontal);
        m_brightness->setRange(Lighting::kBrightnessMin, Lighting::kBrightnessMax);
        m_brightness->setPageStep(1);
        m_brightness->setTickInterval(1);
        m_brightness->setTickPosition(QSlider::TicksBelow);
        m_brightness->setAccessibleName(tr("Brightness"));
        m_brightness->setAccessibleDescription(tr("Brightness from %1 to %2.")
                                                   .arg(Lighting::kBrightnessMin)
                                                   .arg(Lighting::kBrightnessMax));
        connect(m_brightness, &QSlider::valueChanged, this, &LightingPage::onBrightnessChanged);
        row->addWidget(m_brightness, 1);

        m_brightnessValue = new QLabel;
        m_brightnessValue->setFont(Theme::font(11, QFont::DemiBold));
        m_brightnessValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_brightnessValue->setMinimumWidth(56);
        {
            QPalette p = m_brightnessValue->palette();
            p.setColor(QPalette::WindowText, Theme::textPrimary());
            m_brightnessValue->setPalette(p);
        }
        row->addWidget(m_brightnessValue, 0);
        v->addLayout(row);
    }

    // Effect cards.
    {
        v->addWidget(PageUtils::makeLabel(tr("Effect"), Theme::font(11, QFont::Medium),
                                          Theme::textSecondary()));
        auto *cards = new QHBoxLayout;
        cards->setContentsMargins(0, 0, 0, 0);
        cards->setSpacing(Theme::Space::M);
        for (Lighting::Effect effect : kEffectOrder) {
            auto *card = new LightingUi::EffectCard(effect, effectDisplayName(effect));
            connect(card, &QAbstractButton::clicked, this,
                    [this, effect]() { onEffectPicked(effect); });
            m_effectCards.append(card);
            cards->addWidget(card);
        }
        cards->addStretch(1);
        v->addLayout(cards);

        auto *note = PageUtils::makeLabel(
            tr("Effect names come from the OEM panel; the actual product software behaviour "
               "has not been verified."),
            Theme::font(10, QFont::Normal), Theme::textMuted());
        note->setWordWrap(true);
        v->addWidget(note);
    }

    return panel;
}

QWidget *LightingPage::buildPreview()
{
    auto *wrap = new QWidget;
    auto *wv = new QVBoxLayout(wrap);
    wv->setContentsMargins(0, 0, 0, 0);
    wv->setSpacing(Theme::Space::S);
    wv->addWidget(sectionLabel(tr("PREVIEW")));

    auto *panel = new SurfacePanel;
    auto *hv = new QHBoxLayout(panel);
    hv->setContentsMargins(Theme::Space::XL, Theme::Space::L, Theme::Space::XL, Theme::Space::L);
    hv->setSpacing(Theme::Space::XL);

    auto *grid = new QGridLayout;
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(Theme::Space::L);
    grid->setVerticalSpacing(Theme::Space::S);
    grid->setColumnStretch(1, 1);

    m_prevZone = previewValue();
    m_prevEffect = previewValue();
    m_prevBrightness = previewValue();
    m_prevIntensity = previewValue();
    m_prevPacked = previewValue();

    int r = 0;
    grid->addWidget(previewCaption(tr("Zone")), r, 0, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(m_prevZone, r++, 1);
    grid->addWidget(previewCaption(tr("Effect")), r, 0, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(m_prevEffect, r++, 1);
    grid->addWidget(previewCaption(tr("Brightness")), r, 0, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(m_prevBrightness, r++, 1);
    grid->addWidget(previewCaption(QStringLiteral("multi_intensity")), r, 0,
                    Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(m_prevIntensity, r++, 1);
    grid->addWidget(previewCaption(tr("Packed word")), r, 0, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(m_prevPacked, r++, 1);

    m_previewNote = PageUtils::makeLabel(QString(), Theme::font(10, QFont::Normal),
                                         Theme::textMuted());
    m_previewNote->setWordWrap(true);
    grid->addWidget(m_previewNote, r++, 0, 1, 2);

    hv->addLayout(grid, 1);

    m_glow = new LightingUi::GlowPreview;
    m_glow->setFixedWidth(150);
    hv->addWidget(m_glow, 0);

    wv->addWidget(panel);
    return wrap;
}

QWidget *LightingPage::buildGate()
{
    auto *wrap = new QWidget;
    auto *wv = new QVBoxLayout(wrap);
    wv->setContentsMargins(0, 0, 0, 0);
    wv->setSpacing(Theme::Space::S);
    wv->addWidget(sectionLabel(tr("HARDWARE WRITE")));

    auto *panel = new SurfacePanel;
    m_gatePanel = panel;
    auto *v = new QVBoxLayout(panel);
    v->setContentsMargins(Theme::Space::XL, Theme::Space::L, Theme::Space::XL, Theme::Space::L);
    v->setSpacing(Theme::Space::S);

    m_uiGateBox = new QCheckBox(tr("Enable the application write gate (this session only)"));
    m_uiGateBox->setAccessibleDescription(
        tr("Off by default and never saved. Even when enabled, writing only happens if the "
           "kernel gate is also open."));
    connect(m_uiGateBox, &QCheckBox::toggled, this, [this](bool on) {
        m_uiGate = on;
        refreshGate();
    });
    v->addWidget(m_uiGateBox);

    auto *pills = new QHBoxLayout;
    pills->setContentsMargins(0, 0, 0, 0);
    pills->setSpacing(Theme::Space::M);
    m_uiGatePill = new StatusPill;
    m_kernelGatePill = new StatusPill;
    pills->addWidget(m_uiGatePill, 0, Qt::AlignVCenter);
    pills->addWidget(m_kernelGatePill, 0, Qt::AlignVCenter);
    pills->addStretch(1);
    v->addLayout(pills);

    m_gateWarning = PageUtils::makeLabel(QString(), Theme::font(10, QFont::Normal),
                                         Theme::warning());
    m_gateWarning->setWordWrap(true);
    m_gateWarning->setVisible(false);
    v->addWidget(m_gateWarning);

    auto *applyRow = new QHBoxLayout;
    applyRow->setContentsMargins(0, Theme::Space::XS, 0, 0);
    applyRow->setSpacing(Theme::Space::M);
    m_apply = new GateButton(tr("Apply to hardware"));
    m_apply->setAccessibleName(tr("Apply to hardware"));
    connect(m_apply, &ActionButton::clicked, this, &LightingPage::onApply);
    // Persistent outcome indicator for the last Apply attempt (applied / partial
    // / failed / not applied), so a failure is never mistaken for success.
    m_writePill = new StatusPill;
    m_writePill->setVisible(false);
    m_writeStatus = PageUtils::makeLabel(QString(), Theme::font(10, QFont::Normal),
                                         Theme::textSecondary());
    applyRow->addWidget(m_apply, 0, Qt::AlignVCenter);
    applyRow->addWidget(m_writePill, 0, Qt::AlignVCenter);
    applyRow->addWidget(m_writeStatus, 0, Qt::AlignVCenter);
    applyRow->addStretch(1);
    v->addLayout(applyRow);

    wv->addWidget(panel);
    return wrap;
}

QWidget *LightingPage::buildProtocolNote()
{
    auto *panel = new WarningPanel;
    auto *v = new QVBoxLayout(panel);
    v->setContentsMargins(Theme::Space::XL, Theme::Space::L, Theme::Space::XL, Theme::Space::L);
    v->setSpacing(Theme::Space::S);

    auto *title = PageUtils::makeLabel(tr("NOT — protocol not verified"),
                                       Theme::font(11, QFont::DemiBold), Theme::warning());
    v->addWidget(title);

    auto *body = PageUtils::makeLabel(
        tr("The firmware behaviour of these effects, the zone-to-LED mapping and the physical "
           "point mapping have not been verified. The 0xFB00 / 0x0100 SMI calls used by the "
           "vendor tool carry a hang risk. Treat everything on this page as a prototype, not "
           "as a verified product feature."),
        Theme::font(10, QFont::Normal), Theme::textSecondary());
    body->setWordWrap(true);
    v->addWidget(body);
    return panel;
}

void LightingPage::rebuildZoneSelectors()
{
    while (QLayoutItem *item = m_chipLayout->takeAt(0)) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }
    m_chips.clear();

    for (Lighting::Zone zone : m_availableZones) {
        auto *chip = new LightingUi::ZoneChip(zone, zoneDisplayName(zone));
        connect(chip, &QAbstractButton::clicked, this, [this, zone]() { selectZone(zone); });
        m_chipLayout->addWidget(chip);
        m_chips.append(chip);
    }
    if (m_availableZones.isEmpty()) {
        auto *none = PageUtils::makeLabel(tr("No LED zones reported."), Theme::font(11, QFont::Normal),
                                          Theme::textMuted());
        m_chipLayout->addWidget(none);
    }
    m_chipLayout->addStretch(1);

    if (m_schematic)
        m_schematic->setAvailableZones(m_availableZones);
}

void LightingPage::selectZone(Lighting::Zone zone)
{
    if (!m_availableZones.contains(zone))
        return;
    m_zone = zone;
    if (m_schematic)
        m_schematic->setSelected(zone);
    for (auto *chip : m_chips)
        chip->setSelected(chip->zone() == zone);
    hydrateZone(zone);
    refreshPreview();
}

void LightingPage::hydrateZone(Lighting::Zone zone)
{
    const auto got = m_client.read(zone);
    if (got) {
        // Re-validate the freshly read fields through the same raw-input
        // factory the controls use, so a device value that is out of range can
        // never be narrowed into m_data. On rejection the previous in-memory
        // composition is kept (see below), never an invented one.
        if (const auto composed =
                Lighting::makeLedData(static_cast<int>(got->effect), got->brightness, got->red,
                                      got->green, got->blue)) {
            m_data = *composed;
        }
    }
    // On a failed read the current in-memory composition is kept; we never invent
    // a device state that was not actually read.
    if (m_data.effect != Lighting::Effect::Off)
        m_lastOn = m_data;
    applyStateToControls();
}

void LightingPage::applyStateToControls()
{
    m_updating = true;
    const QSignalBlocker b1(m_red);
    const QSignalBlocker b2(m_green);
    const QSignalBlocker b3(m_blue);
    const QSignalBlocker b4(m_brightness);
    const QSignalBlocker b5(m_toggle);

    const QColor c(m_data.red, m_data.green, m_data.blue);
    m_mainSwatch->setColor(c);
    m_red->setValue(m_data.red);
    m_green->setValue(m_data.green);
    m_blue->setValue(m_data.blue);
    m_brightness->setValue(m_data.brightness);
    m_brightnessValue->setText(
        tr("%1 / %2").arg(m_data.brightness).arg(Lighting::kBrightnessMax));
    m_toggle->setCheckedSilently(m_data.effect != Lighting::Effect::Off);

    for (auto *card : m_effectCards)
        card->setSelected(card->effect() == m_data.effect);
    for (auto *sw : m_palette)
        sw->setSelected(sw->color().rgb() == c.rgb());

    m_updating = false;
}

void LightingPage::refreshPreview()
{
    if (!m_schematic || !m_glow)
        return;

    m_schematic->setData(m_data);
    m_glow->setState(m_data);
    for (auto *card : m_effectCards)
        card->setHint(m_data);

    m_prevZone->setText(
        QStringLiteral("%1  \u00B7  %2").arg(Lighting::zoneToString(m_zone), zoneDisplayName(m_zone)));
    m_prevEffect->setText(QStringLiteral("%1  \u00B7  %2")
                              .arg(Lighting::effectToString(m_data.effect),
                                   effectDisplayName(m_data.effect)));
    m_prevBrightness->setText(
        QStringLiteral("%1 / %2").arg(m_data.brightness).arg(Lighting::kBrightnessMax));
    m_prevIntensity->setText(RgbClient::multiIntensityLine(m_data));
    m_prevPacked->setText(QStringLiteral("0x")
                          + QString::number(Lighting::pack(m_data), 16).rightJustified(
                                8, QLatin1Char('0')));

    if (m_zoneSummary)
        m_zoneSummary->setText(zoneDisplayName(m_zone));

    m_previewNote->setText(
        writeGateEffective()
            ? tr("Write gate open — Apply will write these values to the device.")
            : tr("Dry-run — nothing is written."));
}

void LightingPage::refreshGate()
{
    const KernelGate gate = readKernelGate();
    m_kernelGateExists = gate.exists;
    m_kernelGateOpen = gate.open;

    // The single place the client is told whether writing is allowed.
    m_client.setHardwareWriteEnabled(writeGateEffective());

    if (!gate.exists) {
        m_kernelGatePill->setText(tr("Kernel gate: not present"));
        m_kernelGatePill->setTone(StatusPill::Tone::Neutral);
    } else if (gate.open) {
        m_kernelGatePill->setText(tr("Kernel gate: open"));
        m_kernelGatePill->setTone(StatusPill::Tone::Success);
    } else {
        m_kernelGatePill->setText(tr("Kernel gate: closed"));
        m_kernelGatePill->setTone(StatusPill::Tone::Warning);
    }

    m_uiGatePill->setText(m_uiGate ? tr("Application gate: open")
                                   : tr("Application gate: closed"));
    m_uiGatePill->setTone(m_uiGate ? StatusPill::Tone::Info : StatusPill::Tone::Neutral);

    QString warning;
    bool showWarning = false;
    if (m_uiGate && !gate.open) {
        warning = tr("The application gate is open, but the kernel gate is closed, so writing "
                     "stays blocked. To open the kernel gate, run this yourself: echo Y > "
                     "/sys/module/excalibur_wmi/parameters/enable_write");
        showWarning = true;
    } else if (!m_uiGate && gate.open) {
        warning = tr("The kernel gate is open, but the application gate is off, so Apply does "
                     "nothing.");
        showWarning = true;
    }
    m_gateWarning->setText(warning);
    m_gateWarning->setVisible(showWarning);

    m_apply->setEnabled(m_client.available() && writeGateEffective());

    // Short, honest reason next to the (possibly disabled) button. The button is
    // only actionable when both gates are open and the interface is present.
    const auto setStatus = [this](const QString &text) {
        // A gate refresh invalidates the previous Apply outcome, so clear the
        // outcome pill and fall back to the neutral gate hint.
        if (m_writePill)
            m_writePill->setVisible(false);
        m_writeStatus->setText(text);
        QPalette p = m_writeStatus->palette();
        p.setColor(QPalette::WindowText, Theme::textMuted());
        m_writeStatus->setPalette(p);
    };
    if (!m_client.available())
        setStatus(tr("Interface unavailable."));
    else if (!writeGateEffective())
        setStatus(tr("Blocked — both gates must be open."));
    else
        setStatus(QString());

    refreshPreview();
}

void LightingPage::updateAvailabilityPill()
{
    if (m_client.available()) {
        m_availabilityPill->setText(tr("LED interface available"));
        m_availabilityPill->setTone(StatusPill::Tone::Success);
    } else {
        m_availabilityPill->setText(tr("LED interface unavailable"));
        m_availabilityPill->setTone(StatusPill::Tone::Critical);
    }
}

void LightingPage::setControlsEnabled(bool enabled)
{
    if (m_controlsPanel)
        m_controlsPanel->setEnabled(enabled);
    if (m_gatePanel)
        m_gatePanel->setEnabled(enabled);
    if (m_schematic)
        m_schematic->setEnabled(enabled);
}

void LightingPage::rescan()
{
    m_client.discover();

    const QVector<Lighting::Zone> found = m_client.availableZones();
    m_availableZones.clear();
    for (Lighting::Zone zone : kPresentationOrder) {
        if (found.contains(zone))
            m_availableZones.append(zone);
    }

    rebuildZoneSelectors();
    updateAvailabilityPill();

    const bool available = m_client.available();
    m_stack->setCurrentWidget(available ? m_content : m_empty);
    setControlsEnabled(available);

    if (available) {
        if (!m_availableZones.contains(m_zone))
            m_zone = m_availableZones.first();
        selectZone(m_zone);
    }

    refreshGate();
}

void LightingPage::onToggle(bool on)
{
    if (m_updating)
        return;
    if (on) {
        Lighting::LedData restored = m_lastOn;
        if (restored.effect == Lighting::Effect::Off)
            restored.effect = Lighting::Effect::Static;
        m_data = restored;
    } else {
        if (m_data.effect != Lighting::Effect::Off)
            m_lastOn = m_data;
        m_data.effect = Lighting::Effect::Off;
    }
    applyStateToControls();
    refreshPreview();
}

void LightingPage::onEffectPicked(Lighting::Effect effect)
{
    if (m_updating)
        return;
    m_data.effect = effect;
    if (effect != Lighting::Effect::Off)
        m_lastOn = m_data;
    applyStateToControls();
    refreshPreview();
}

void LightingPage::onColorPicked(const QColor &color)
{
    if (m_updating || !color.isValid())
        return;
    // Raw-input factory: validate the wide colour channels before narrowing.
    const auto composed = Lighting::makeLedData(static_cast<int>(m_data.effect),
                                                m_data.brightness, color.red(), color.green(),
                                                color.blue());
    if (!composed)
        return; // out of range -> refuse, keep the last valid composition
    m_data = *composed;
    if (m_data.effect != Lighting::Effect::Off)
        m_lastOn = m_data;
    applyStateToControls();
    refreshPreview();
}

void LightingPage::onRgbEdited()
{
    if (m_updating)
        return;
    // Raw-input factory: the spin boxes yield wide ints; makeLedData() rejects
    // any value a narrowing cast to the 8-bit field would wrap or clamp.
    const auto composed =
        Lighting::makeLedData(static_cast<int>(m_data.effect), m_data.brightness, m_red->value(),
                              m_green->value(), m_blue->value());
    if (!composed)
        return; // out of range -> refuse, keep the last valid composition
    m_data = *composed;
    if (m_data.effect != Lighting::Effect::Off)
        m_lastOn = m_data;
    const QColor c(m_data.red, m_data.green, m_data.blue);
    m_mainSwatch->setColor(c);
    for (auto *sw : m_palette)
        sw->setSelected(sw->color().rgb() == c.rgb());
    refreshPreview();
}

void LightingPage::onBrightnessChanged(int value)
{
    if (m_updating)
        return;
    // Raw-input factory: the slider hands over a wide int; makeLedData() rejects
    // anything outside 0..15 (e.g. a value that would wrap a nibble) instead of
    // silently masking it.
    const auto composed = Lighting::makeLedData(static_cast<int>(m_data.effect), value, m_data.red,
                                                m_data.green, m_data.blue);
    if (!composed)
        return; // out of range -> refuse, keep the last valid composition
    m_data = *composed;
    m_brightnessValue->setText(
        tr("%1 / %2").arg(m_data.brightness).arg(Lighting::kBrightnessMax));
    if (m_data.effect != Lighting::Effect::Off)
        m_lastOn = m_data;
    refreshPreview();
}

void LightingPage::onChooseColor()
{
    const QColor current(m_data.red, m_data.green, m_data.blue);
    const QColor picked = QColorDialog::getColor(current, this, tr("Choose colour"));
    if (picked.isValid())
        onColorPicked(picked);
}

void LightingPage::showWriteStatus(WriteOutcome outcome, const QString &detail)
{
    StatusPill::Tone tone = StatusPill::Tone::Neutral;
    QColor color = Theme::textMuted();
    QString label;
    switch (outcome) {
    case WriteOutcome::Applied:
        tone = StatusPill::Tone::Success;
        color = Theme::success();
        label = tr("Applied");
        break;
    case WriteOutcome::Partial:
        tone = StatusPill::Tone::Critical;
        color = Theme::critical();
        label = tr("Partial write");
        break;
    case WriteOutcome::Failed:
        tone = StatusPill::Tone::Critical;
        color = Theme::critical();
        label = tr("Write failed");
        break;
    case WriteOutcome::Blocked:
        tone = StatusPill::Tone::Warning;
        color = Theme::warning();
        label = tr("Not applied");
        break;
    }

    if (m_writePill) {
        m_writePill->setText(label);
        m_writePill->setTone(tone);
        m_writePill->setVisible(true);
    }
    m_writeStatus->setText(detail);
    QPalette p = m_writeStatus->palette();
    p.setColor(QPalette::WindowText, color);
    m_writeStatus->setPalette(p);
}

void LightingPage::onApply()
{
    // Dry-run guard: write() is never reached unless BOTH gates are open, the
    // interface is present, and the client itself was granted write permission.
    // Every blocked path reports WHY, instead of returning silently.
    if (!m_client.available()) {
        showWriteStatus(WriteOutcome::Blocked,
                        tr("Not applied — the LED interface is unavailable."));
        return;
    }
    if (!writeGateEffective() || !m_client.hardwareWriteEnabled()) {
        showWriteStatus(WriteOutcome::Blocked, tr("Not applied — the write gate is closed."));
        return;
    }

    const Lighting::LedData intended = currentData();
    if (m_client.write(m_zone, intended)) {
        showWriteStatus(WriteOutcome::Applied,
                        tr("Applied to %1.").arg(zoneDisplayName(m_zone)));
        return;
    }

    // write() sets three separate sysfs attributes one after another, so a false
    // return does NOT mean "nothing changed": an earlier attribute may already
    // have landed. Never report "values unchanged"; read the zone back and say
    // exactly what the device holds.
    const auto actual = m_client.read(m_zone);
    if (actual && sameLedState(*actual, intended)) {
        // The attributes match the request despite the error return: the write
        // effectively succeeded (e.g. a read-only attribute already held the
        // value). Report it as applied, confirmed by reading the current LED
        // state back from the device.
        showWriteStatus(
            WriteOutcome::Applied,
            tr("Applied to %1 (confirmed by reading the current LED state).")
                .arg(zoneDisplayName(m_zone)));
    } else if (actual) {
        showWriteStatus(WriteOutcome::Partial,
                        tr("Write FAILED — %1 was left in a PARTIAL state (the device does not "
                           "match the requested values); re-read before retrying.")
                            .arg(zoneDisplayName(m_zone)));
    } else {
        showWriteStatus(WriteOutcome::Failed,
                        tr("Write FAILED — the device state is unknown; re-read before retrying."));
    }
}

#include "lightingpage.moc"
