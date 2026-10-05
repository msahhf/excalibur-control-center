#include "sidebar.h"

#include "app/version.h"
#include "theme/theme.h"

#include <QEnterEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QVariantAnimation>

#include <algorithm>
#include <cmath>

namespace {

QColor mixColor(const QColor &a, const QColor &b, qreal t)
{
    return QColor(qRound(a.red() + (b.red() - a.red()) * t),
                  qRound(a.green() + (b.green() - a.green()) * t),
                  qRound(a.blue() + (b.blue() - a.blue()) * t),
                  qRound(a.alpha() + (b.alpha() - a.alpha()) * t));
}

// One navigation entry: icon + label, painted (no stylesheet) so it stays
// consistent across palettes. Active state = filled pill + accent marker.
class NavButton : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)

public:
    NavButton(const QString &label, Sidebar::Icon icon, int index, QWidget *parent)
        : QWidget(parent), m_label(label), m_icon(icon), m_index(index)
    {
        setFixedHeight(44);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);
        setFocusPolicy(Qt::StrongFocus);
        setFont(Theme::font(11, QFont::Medium));
        setToolTip(label);
        m_hoverAnim = new QPropertyAnimation(this, "hoverProgress", this);
        m_hoverAnim->setDuration(120);
        m_hoverAnim->setEasingCurve(QEasingCurve::OutCubic);
    }

    int navIndex() const { return m_index; }
    qreal hoverProgress() const { return m_hover; }
    void setHoverProgress(qreal v)
    {
        m_hover = v;
        update();
    }
    void setActive(bool active)
    {
        if (m_active == active)
            return;
        m_active = active;
        update();
    }

signals:
    void clicked(int index);
    void navigate(int delta);

protected:
    void enterEvent(QEnterEvent *) override { animateTo(1.0); }
    void leaveEvent(QEvent *) override { animateTo(0.0); }
    void focusInEvent(QFocusEvent *e) override
    {
        m_focusVisible = (e->reason() == Qt::TabFocusReason
                          || e->reason() == Qt::BacktabFocusReason
                          || e->reason() == Qt::ShortcutFocusReason);
        update();
    }
    void focusOutEvent(QFocusEvent *) override
    {
        m_focusVisible = false;
        update();
    }

    void keyPressEvent(QKeyEvent *e) override
    {
        switch (e->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            emit clicked(m_index);
            e->accept();
            break;
        case Qt::Key_Down:
            emit navigate(1);
            e->accept();
            break;
        case Qt::Key_Up:
            emit navigate(-1);
            e->accept();
            break;
        default:
            QWidget::keyPressEvent(e);
        }
    }

    void mouseReleaseEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton && rect().contains(e->position().toPoint())) {
            setFocus(Qt::MouseFocusReason);
            emit clicked(m_index);
        }
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const QRectF r = QRectF(rect()).adjusted(0.5, 1.5, -0.5, -1.5);

        // The active pill is drawn by the Sidebar (so it can slide between
        // items); here we only draw the hover wash for inactive entries.
        if (!m_active && m_hover > 0.0) {
            p.setPen(Qt::NoPen);
            p.setBrush(mixColor(QColor(Theme::surfaceHover().red(), Theme::surfaceHover().green(),
                                       Theme::surfaceHover().blue(), 0),
                                Theme::surfaceHover(), m_hover));
            p.drawRoundedRect(r, Theme::RadiusSmall + 2, Theme::RadiusSmall + 2);
        }

        const QColor baseFg = m_active ? Theme::textPrimary() : Theme::textSecondary();
        const QColor fg = mixColor(baseFg, Theme::textPrimary(), m_active ? 1.0 : m_hover);
        const QColor iconColor = m_active ? Theme::accent() : fg;

        drawIcon(p, QRectF(18.0, (height() - 20.0) / 2.0, 20.0, 20.0), iconColor);

        p.setPen(fg);
        p.setFont(font());
        p.drawText(r.adjusted(50.0, 0.0, -12.0, 0.0), Qt::AlignLeft | Qt::AlignVCenter, m_label);

        // Keyboard focus ring (only when focus came from the keyboard).
        if (m_focusVisible) {
            p.setPen(QPen(Theme::accent(), 2));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(r.adjusted(1.0, 1.0, -1.0, -1.0),
                              Theme::RadiusSmall + 2, Theme::RadiusSmall + 2);
        }
    }

private:
    void animateTo(qreal target)
    {
        m_hoverAnim->stop();
        m_hoverAnim->setStartValue(m_hover);
        m_hoverAnim->setEndValue(target);
        m_hoverAnim->start();
    }

    void drawIcon(QPainter &p, const QRectF &box, const QColor &c)
    {
        QPen pen(c, 1.6);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        const qreal x = box.left();
        const qreal y = box.top();
        const qreal s = box.width();

        switch (m_icon) {
        case Sidebar::Icon::Dashboard: {
            const qreal g = s * 0.13;
            const qreal t = (s - g) / 2.0;
            p.drawRoundedRect(QRectF(x, y, t, t), 2.5, 2.5);
            p.drawRoundedRect(QRectF(x + t + g, y, t, t), 2.5, 2.5);
            p.drawRoundedRect(QRectF(x, y + t + g, t, t), 2.5, 2.5);
            p.drawRoundedRect(QRectF(x + t + g, y + t + g, t, t), 2.5, 2.5);
            break;
        }
        case Sidebar::Icon::Cooling: {
            const QPointF ctr = box.center();
            const qreal rad = s * 0.44;
            for (int i = 0; i < 3; ++i) {
                const qreal a = static_cast<qreal>(M_PI) * i / 3.0;
                const QPointF d(std::cos(a), std::sin(a));
                p.drawLine(ctr - d * rad, ctr + d * rad);
            }
            const qreal tip = s * 0.15;
            for (int i = 0; i < 6; ++i) {
                const qreal a = static_cast<qreal>(M_PI) * i / 3.0 + static_cast<qreal>(M_PI) / 6.0;
                const QPointF d(std::cos(a), std::sin(a));
                const QPointF base = ctr + d * (rad * 0.6);
                p.drawLine(base, base + QPointF(-d.y(), d.x()) * tip);
            }
            break;
        }
        case Sidebar::Icon::Device: {
            const QRectF body = box.adjusted(s * 0.24, s * 0.24, -s * 0.24, -s * 0.24);
            p.drawRoundedRect(body, 3.0, 3.0);
            for (int i = 0; i < 3; ++i) {
                const qreal f = (i + 1.0) / 4.0;
                const qreal yy = body.top() + body.height() * f;
                const qreal xx = body.left() + body.width() * f;
                p.drawLine(QPointF(x, yy), QPointF(body.left(), yy));
                p.drawLine(QPointF(body.right(), yy), QPointF(x + s, yy));
                p.drawLine(QPointF(xx, y), QPointF(xx, body.top()));
                p.drawLine(QPointF(xx, body.bottom()), QPointF(xx, y + s));
            }
            break;
        }
        case Sidebar::Icon::About: {
            p.drawEllipse(box.adjusted(s * 0.08, s * 0.08, -s * 0.08, -s * 0.08));
            const QPointF ctr = box.center();
            p.setBrush(c);
            p.setPen(Qt::NoPen);
            p.drawEllipse(ctr - QPointF(0.0, s * 0.22), 1.3, 1.3);
            QPen dot(c, 1.7);
            dot.setCapStyle(Qt::RoundCap);
            p.setPen(dot);
            p.drawLine(ctr + QPointF(0.0, -s * 0.04), ctr + QPointF(0.0, s * 0.24));
            break;
        }
        }
    }

    QString m_label;
    Sidebar::Icon m_icon;
    int m_index;
    bool m_active = false;
    qreal m_hover = 0.0;
    bool m_focusVisible = false;
    QPropertyAnimation *m_hoverAnim = nullptr;
};

} // namespace

namespace {

// Move keyboard focus to the neighbouring navigation entry.
void focusNeighbor(QWidget *container, NavButton *from, int delta)
{
    auto buttons = container->findChildren<NavButton *>();
    std::sort(buttons.begin(), buttons.end(), [](NavButton *a, NavButton *b) {
        return a->navIndex() < b->navIndex();
    });
    const int pos = buttons.indexOf(from);
    if (pos < 0 || buttons.isEmpty())
        return;
    int next = pos + delta;
    next = qBound(0, next, buttons.size() - 1);
    buttons.at(next)->setFocus(Qt::TabFocusReason);
}

} // namespace

Sidebar::Sidebar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("Sidebar"));
    setFixedWidth(244);

    m_indicatorAnim = new QVariantAnimation(this);
    m_indicatorAnim->setDuration(200);
    m_indicatorAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_indicatorAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_indicator = v.toRectF();
        update();
    });

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 24, 14, 18);
    root->setSpacing(0);

    auto *brand = new QWidget;
    auto *bl = new QVBoxLayout(brand);
    bl->setContentsMargins(10, 0, 10, 0);
    bl->setSpacing(2);

    auto *word = new QLabel(QStringLiteral("EXCALIBUR"));
    word->setFont(Theme::font(15, QFont::Bold, 106));
    auto *sub = new QLabel(QStringLiteral("CONTROL CENTER"));
    sub->setFont(Theme::labelFont(9, QFont::DemiBold, 150));
    for (QLabel *l : {word, sub}) {
        QPalette p = l->palette();
        p.setColor(QPalette::WindowText, l == word ? Theme::textPrimary() : Theme::textMuted());
        l->setPalette(p);
    }
    bl->addWidget(word);
    bl->addWidget(sub);

    root->addWidget(brand);
    root->addSpacing(28);

    m_itemsLayout = new QVBoxLayout;
    m_itemsLayout->setContentsMargins(0, 0, 0, 0);
    m_itemsLayout->setSpacing(6);
    root->addLayout(m_itemsLayout);

    root->addStretch(1);

    m_bottomLayout = new QVBoxLayout;
    m_bottomLayout->setContentsMargins(0, 0, 0, 0);
    m_bottomLayout->setSpacing(6);
    root->addLayout(m_bottomLayout);

    auto *version = new QLabel(QStringLiteral("v%1").arg(QString::fromLatin1(AppVersion::string())));
    version->setFont(Theme::font(9, QFont::Normal));
    {
        QPalette p = version->palette();
        p.setColor(QPalette::WindowText, Theme::textMuted());
        version->setPalette(p);
    }
    auto *verRow = new QWidget;
    auto *vl = new QVBoxLayout(verRow);
    vl->setContentsMargins(18, 8, 0, 0);
    vl->addWidget(version);
    root->addWidget(verRow);
}

int Sidebar::addItem(const QString &label, Icon icon)
{
    const int index = m_nextIndex++;
    auto *btn = new NavButton(label, icon, index, this);
    connect(btn, &NavButton::clicked, this, &Sidebar::pageSelected);
    connect(btn, &NavButton::navigate, this, [this, btn](int delta) { focusNeighbor(this, btn, delta); });
    m_itemsLayout->addWidget(btn);
    return index;
}

void Sidebar::addBottomItem(const QString &label, Icon icon)
{
    const int index = m_nextIndex++;
    auto *btn = new NavButton(label, icon, index, this);
    connect(btn, &NavButton::clicked, this, &Sidebar::pageSelected);
    connect(btn, &NavButton::navigate, this, [this, btn](int delta) { focusNeighbor(this, btn, delta); });
    m_bottomLayout->addWidget(btn);
}

void Sidebar::setCurrentIndex(int index)
{
    m_current = index;
    const auto apply = [&](QVBoxLayout *l) {
        for (int i = 0; i < l->count(); ++i) {
            if (auto *b = qobject_cast<NavButton *>(l->itemAt(i)->widget()))
                b->setActive(b->navIndex() == index);
        }
    };
    apply(m_itemsLayout);
    apply(m_bottomLayout);
    syncIndicator(true);
}

QRectF Sidebar::activeItemRect() const
{
    const auto find = [&](QVBoxLayout *l) -> QRectF {
        for (int i = 0; i < l->count(); ++i) {
            if (auto *b = qobject_cast<NavButton *>(l->itemAt(i)->widget())) {
                if (b->navIndex() == m_current)
                    return QRectF(b->geometry()).adjusted(0.5, 1.5, -0.5, -1.5);
            }
        }
        return QRectF();
    };
    QRectF r = find(m_itemsLayout);
    if (r.isNull())
        r = find(m_bottomLayout);
    return r;
}

void Sidebar::syncIndicator(bool animate)
{
    const QRectF target = activeItemRect();
    if (target.isNull())
        return;
    if (!m_indicatorReady || !animate) {
        m_indicatorAnim->stop();
        m_indicator = target;
        m_indicatorReady = true;
        update();
        return;
    }
    if (m_indicator == target)
        return;
    m_indicatorAnim->stop();
    m_indicatorAnim->setStartValue(m_indicator);
    m_indicatorAnim->setEndValue(target);
    m_indicatorAnim->start();
}

void Sidebar::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    syncIndicator(false); // keep the pill on the active item when geometry changes
}

void Sidebar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), Theme::sidebar());

    // First paint before any layout pass: resolve the active rect directly.
    if (!m_indicatorReady) {
        m_indicator = activeItemRect();
        m_indicatorReady = !m_indicator.isNull();
    }

    if (m_indicatorReady && !m_indicator.isNull()) {
        const QRectF r = m_indicator;
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::surfaceElevated());
        p.drawRoundedRect(r, Theme::RadiusSmall + 2, Theme::RadiusSmall + 2);
        p.setBrush(Theme::accent());
        p.drawRoundedRect(QRectF(0.0, r.center().y() - 9.0, 3.0, 18.0), 1.5, 1.5);
    }
}

#include "sidebar.moc"
