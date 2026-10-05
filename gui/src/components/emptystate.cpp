#include "emptystate.h"

#include "theme/theme.h"

#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>

namespace {

// Simple "offline" glyph: a ring with a diagonal slash, plus a dim chip hint.
class OfflineGlyph : public QWidget
{
public:
    explicit OfflineGlyph(QWidget *parent = nullptr) : QWidget(parent) { setFixedSize(64, 64); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const QPointF c = QRectF(rect()).center();
        const qreal rad = 20.0;

        QPen ring(Theme::textMuted(), 2.0);
        ring.setCapStyle(Qt::RoundCap);
        p.setPen(ring);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c, rad, rad);

        p.drawLine(c + QPointF(-rad * 0.72, rad * 0.72),
                   c + QPointF(rad * 0.72, -rad * 0.72));
    }
};

} // namespace

EmptyState::EmptyState(QWidget *parent)
    : QWidget(parent)
{
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);
    v->addStretch(1);

    auto *glyph = new OfflineGlyph;
    v->addWidget(glyph, 0, Qt::AlignHCenter);
    v->addSpacing(Theme::Space::XL);

    auto *title = new QLabel(m_title);
    title->setObjectName(QStringLiteral("EmptyStateTitle"));
    title->setFont(Theme::font(19, QFont::Bold));
    title->setAlignment(Qt::AlignCenter);
    {
        QPalette p = title->palette();
        p.setColor(QPalette::WindowText, Theme::textPrimary());
        title->setPalette(p);
    }
    v->addWidget(title);
    v->addSpacing(Theme::Space::S);

    auto *body = new QLabel(m_body);
    body->setObjectName(QStringLiteral("EmptyStateBody"));
    body->setFont(Theme::font(11, QFont::Normal));
    body->setAlignment(Qt::AlignCenter);
    body->setWordWrap(true);
    {
        QPalette p = body->palette();
        p.setColor(QPalette::WindowText, Theme::textSecondary());
        body->setPalette(p);
    }
    v->addWidget(body);
    v->addStretch(1);
}

void EmptyState::setMessage(const QString &title, const QString &body)
{
    m_title = title;
    m_body = body;
    if (auto *t = findChild<QLabel *>(QStringLiteral("EmptyStateTitle")))
        t->setText(title);
    if (auto *b = findChild<QLabel *>(QStringLiteral("EmptyStateBody")))
        b->setText(body);
}
