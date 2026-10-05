#include "coolingpage.h"

#include "pageutils.h"
#include "theme/theme.h"

#include <QVBoxLayout>

CoolingPage::CoolingPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::Space::XXL, Theme::Space::XL, Theme::Space::XXL, Theme::Space::XL);
    root->setSpacing(Theme::Space::L);

    root->addWidget(PageUtils::pageTitle(QStringLiteral("Cooling")));
    root->addWidget(PageUtils::pageSubtitle(
        QStringLiteral("How the cooling system responds to thermal load.")));

    m_body = new QVBoxLayout;
    m_body->setContentsMargins(0, Theme::Space::M, 0, 0);
    m_body->setSpacing(Theme::Space::L);
    root->addLayout(m_body);
    root->addStretch(1);
}
