#include "sidebar.h"

#include "theme/theme.h"

#include <QEnterEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>

#include <cmath>

namespace {

// One navigation entry: icon + label, painted (no stylesheet) so it stays
// consistent across palettes. Active state = filled pill + accent marker.
class NavButton : public QWidget
{
    Q_OBJECT

public:
    NavButton(const QString &label, Sidebar::Icon icon, int index, QWidget *parent)
        : QWidget(parent), m_label(label), m_icon(icon), m_index(index)
    {
        setFixedHeight(44);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);
        setFont(Theme::font(11, QFont::Medium));
        setToolTip(label);
    }

    int navIndex() const { return m_index; }
    void setActive(bool active)
    {
        if (m_active == active)
            return;
        m_active = active;
        update();
    }

signals:
    void clicked(int index);

protected:
    void enterEvent(QEnterEvent *) override { m_hover = true; update(); }
    void leaveEvent(QEvent *) override { m_hover = false; update(); }
    void mouseReleaseEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton && rect().contains(e->position().toPoint()))
            emit clicked(m_index);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const QRectF r = QRectF(rect()).adjusted(0.5, 1.5, -0.5, -1.5);

        if (m_active)
            p.setBrush(Theme::surfaceElevated());
        else if (m_hover)
            p.setBrush(Theme::surfaceHover());
        else
            p.setBrush(Qt::NoBrush);
        p.setPen(Qt::NoPen);
        if (m_active || m_hover)
            p.drawRoundedRect(r, Theme::RadiusSmall + 2, Theme::RadiusSmall + 2);

        if (m_active) {
            p.setBrush(Theme::accent());
            p.drawRoundedRect(QRectF(0.0, r.center().y() - 9.0, 3.0, 18.0), 1.5, 1.5);
        }

        const QColor fg = (m_active || m_hover) ? Theme::textPrimary() : Theme::textSecondary();
        const QColor iconColor = m_active ? Theme::accent() : fg;

        drawIcon(p, QRectF(18.0, (height() - 20.0) / 2.0, 20.0, 20.0), iconColor);

        p.setPen(fg);
        p.setFont(font());
        p.drawText(r.adjusted(50.0, 0.0, -12.0, 0.0), Qt::AlignLeft | Qt::AlignVCenter, m_label);
    }

private:
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
    bool m_hover = false;
};

} // namespace

Sidebar::Sidebar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("Sidebar"));
    setFixedWidth(244);

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

    auto *version = new QLabel(QStringLiteral("v0.1.0"));
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
    m_itemsLayout->addWidget(btn);
    return index;
}

void Sidebar::addBottomItem(const QString &label, Icon icon)
{
    const int index = m_nextIndex++;
    auto *btn = new NavButton(label, icon, index, this);
    connect(btn, &NavButton::clicked, this, &Sidebar::pageSelected);
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
}

void Sidebar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Theme::sidebar());
}

#include "sidebar.moc"
