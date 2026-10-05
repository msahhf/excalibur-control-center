#include "devicepage.h"

#include "pageutils.h"
#include "theme/theme.h"

#include <QVBoxLayout>

DevicePage::DevicePage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::Space::XXL, Theme::Space::XL, Theme::Space::XXL, Theme::Space::XL);
    root->setSpacing(Theme::Space::L);

    root->addWidget(PageUtils::pageTitle(QStringLiteral("Device")));
    root->addWidget(PageUtils::pageSubtitle(
        QStringLiteral("Hardware and software interface details for this machine.")));

    m_body = new QVBoxLayout;
    m_body->setContentsMargins(0, Theme::Space::M, 0, 0);
    m_body->setSpacing(Theme::Space::L);
    root->addLayout(m_body);
    root->addStretch(1);
}
