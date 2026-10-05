#include "dashboardpage.h"

#include "pageutils.h"
#include "theme/theme.h"

#include <QVBoxLayout>

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::Space::XXL, Theme::Space::XL, Theme::Space::XXL, Theme::Space::XL);
    root->setSpacing(Theme::Space::L);

    root->addWidget(PageUtils::pageTitle(QStringLiteral("Dashboard")));
    root->addWidget(PageUtils::pageSubtitle(
        QStringLiteral("System state and hardware telemetry at a glance.")));

    m_body = new QVBoxLayout;
    m_body->setContentsMargins(0, Theme::Space::M, 0, 0);
    m_body->setSpacing(Theme::Space::L);
    root->addLayout(m_body);
    root->addStretch(1);
}
