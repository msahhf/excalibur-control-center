#include "aboutpage.h"

#include "app/version.h"
#include "pageutils.h"
#include "theme/theme.h"

#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>

namespace {

// Small product mark: a rounded surface tile with a restrained accent "E".
class ProductMark : public QWidget
{
public:
    explicit ProductMark(QWidget *parent = nullptr) : QWidget(parent) { setFixedSize(54, 54); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        QColor border = Theme::border();
        border.setAlpha(170);
        p.setPen(QPen(border, 1));
        p.setBrush(Theme::surfaceElevated());
        p.drawRoundedRect(r, 14, 14);

        const qreal x = r.left() + 15.0;
        const qreal y = r.top() + 15.0;
        const qreal w = r.width() - 30.0;
        const qreal h = r.height() - 30.0;
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::accent());
        p.drawRoundedRect(QRectF(x, y, w, 3.2), 1.6, 1.6);
        p.drawRoundedRect(QRectF(x, y + h / 2.0 - 1.6, w * 0.68, 3.2), 1.6, 1.6);
        p.drawRoundedRect(QRectF(x, y + h - 3.2, w, 3.2), 1.6, 1.6);
        p.drawRoundedRect(QRectF(x, y, 3.2, h), 1.6, 1.6);
    }
};

} // namespace

AboutPage::AboutPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = PageUtils::makeCenteredColumn(this, 720);

    root->addWidget(PageUtils::pageTitle(QStringLiteral("About")));
    root->addSpacing(Theme::Space::S);

    auto *mark = new ProductMark;
    root->addWidget(mark);
    root->addSpacing(Theme::Space::M);

    auto *name = PageUtils::makeLabel(QStringLiteral("EXCALIBUR CONTROL CENTER"),
                                      Theme::font(19, QFont::Bold, 104), Theme::textPrimary());
    auto *description = PageUtils::makeLabel(
        QStringLiteral("Native Linux control center for CASPER EXCALIBUR hardware."),
        Theme::font(12, QFont::Normal), Theme::textSecondary());
    description->setWordWrap(true);
    root->addWidget(name);
    root->addWidget(description);
    root->addSpacing(Theme::Space::L);

    auto *version = PageUtils::makeLabel(QStringLiteral("Version %1").arg(
                                             QString::fromLatin1(AppVersion::string())),
                                         Theme::font(11, QFont::Medium), Theme::textSecondary());
    auto *edition = PageUtils::makeLabel(QStringLiteral("Read-only telemetry edition"),
                                         Theme::font(11, QFont::Normal), Theme::textMuted());
    root->addWidget(version);
    root->addWidget(edition);
    root->addStretch(1);
}
