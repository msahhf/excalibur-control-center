#include "aboutpage.h"

#include "app/identity.h"
#include "app/version.h"
#include "pageutils.h"
#include "theme/theme.h"

#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>

AboutPage::AboutPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = PageUtils::makeCenteredColumn(this, 720);

    root->addWidget(PageUtils::pageTitle(QStringLiteral("About")));
    root->addSpacing(Theme::Space::S);

    // The real application icon, so branding matches the launcher/window/tray.
    auto *mark = new QLabel;
    mark->setFixedSize(54, 54);
    mark->setPixmap(QIcon(QStringLiteral(":/icons/excalibur-control-center-128.png"))
                        .pixmap(54, 54));
    root->addWidget(mark);
    root->addSpacing(Theme::Space::M);

    auto *name = PageUtils::makeLabel(AppIdentity::displayName().toUpper(),
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
