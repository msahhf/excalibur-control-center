#include "settingspage.h"

#include "app/usersettings.h"
#include "components/actionbutton.h"
#include "components/surfacepanel.h"
#include "pageutils.h"
#include "theme/theme.h"
#include "util/units.h"

#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace {

QLabel *captionLabel(const QString &text)
{
    auto *l = new QLabel(text);
    l->setFont(Theme::font(11, QFont::Normal));
    l->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    l->setMinimumHeight(34);
    QPalette p = l->palette();
    p.setColor(QPalette::WindowText, Theme::textSecondary());
    l->setPalette(p);
    return l;
}

QComboBox *makeSelector(QWidget *parent)
{
    auto *c = new QComboBox(parent);
    c->setMinimumWidth(170);
    c->setMinimumHeight(32);
    c->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return c;
}

} // namespace

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = PageUtils::makeCenteredColumn(this, 720);

    root->addWidget(PageUtils::pageTitle(QStringLiteral("Settings")));
    root->addWidget(PageUtils::pageSubtitle(
        QStringLiteral("Application preferences. Changes apply immediately and are saved.")));

    auto *title = new QLabel(QStringLiteral("PREFERENCES"));
    title->setFont(Theme::labelFont(9, QFont::DemiBold, 150));
    {
        QPalette p = title->palette();
        p.setColor(QPalette::WindowText, Theme::textMuted());
        title->setPalette(p);
    }

    auto *surface = new SurfacePanel;
    auto *grid = new QGridLayout(surface);
    grid->setContentsMargins(Theme::Space::XL, Theme::Space::M, Theme::Space::XL, Theme::Space::M);
    grid->setHorizontalSpacing(Theme::Space::XL);
    grid->setVerticalSpacing(Theme::Space::S);
    grid->setColumnStretch(0, 1);

    m_theme = makeSelector(surface);
    m_theme->addItem(QStringLiteral("System"), int(UserSettings::AppTheme::System));
    m_theme->addItem(QStringLiteral("Light"), int(UserSettings::AppTheme::Light));
    m_theme->addItem(QStringLiteral("Dark"), int(UserSettings::AppTheme::Dark));

    m_unit = makeSelector(surface);
    m_unit->addItem(QStringLiteral("Celsius"), int(Units::TemperatureUnit::Celsius));
    m_unit->addItem(QStringLiteral("Fahrenheit"), int(Units::TemperatureUnit::Fahrenheit));

    m_refresh = makeSelector(surface);
    for (int ms : UserSettings::refreshOptions())
        m_refresh->addItem(QStringLiteral("%1 s").arg(ms / 1000), ms);

    int r = 0;
    grid->addWidget(captionLabel(QStringLiteral("Theme")), r, 0, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(m_theme, r++, 1, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(captionLabel(QStringLiteral("Temperature unit")), r, 0, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(m_unit, r++, 1, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(captionLabel(QStringLiteral("Refresh interval")), r, 0, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(m_refresh, r++, 1, Qt::AlignRight | Qt::AlignVCenter);

    auto *group = new QWidget;
    auto *gv = new QVBoxLayout(group);
    gv->setContentsMargins(0, 0, 0, 0);
    gv->setSpacing(Theme::Space::S);
    gv->addWidget(title);
    gv->addWidget(surface);
    root->addWidget(group);

    m_reset = new ActionButton(QStringLiteral("Reset to Defaults"));
    root->addWidget(m_reset, 0, Qt::AlignLeft);
    root->addStretch(1);

    connect(m_theme, &QComboBox::currentIndexChanged, this, &SettingsPage::onThemeIndex);
    connect(m_unit, &QComboBox::currentIndexChanged, this, &SettingsPage::onUnitIndex);
    connect(m_refresh, &QComboBox::currentIndexChanged, this, &SettingsPage::onRefreshIndex);
    connect(m_reset, &ActionButton::clicked, this, [this]() {
        UserSettings::instance().resetToDefaults();
        syncFromSettings();
    });

    // Reflect external changes (reset, another surface) without loops.
    UserSettings &us = UserSettings::instance();
    connect(&us, &UserSettings::themeChanged, this, &SettingsPage::syncFromSettings);
    connect(&us, &UserSettings::tempUnitChanged, this, &SettingsPage::syncFromSettings);
    connect(&us, &UserSettings::refreshMsChanged, this, &SettingsPage::syncFromSettings);

    syncFromSettings();
}

void SettingsPage::syncFromSettings()
{
    UserSettings &us = UserSettings::instance();
    const QSignalBlocker b1(m_theme);
    const QSignalBlocker b2(m_unit);
    const QSignalBlocker b3(m_refresh);
    m_theme->setCurrentIndex(m_theme->findData(int(us.theme())));
    m_unit->setCurrentIndex(m_unit->findData(int(us.tempUnit())));
    m_refresh->setCurrentIndex(m_refresh->findData(us.refreshMs()));
}

void SettingsPage::onThemeIndex(int index)
{
    UserSettings::instance().setTheme(
        static_cast<UserSettings::AppTheme>(m_theme->itemData(index).toInt()));
}

void SettingsPage::onUnitIndex(int index)
{
    UserSettings::instance().setTempUnit(
        static_cast<Units::TemperatureUnit>(m_unit->itemData(index).toInt()));
}

void SettingsPage::onRefreshIndex(int index)
{
    UserSettings::instance().setRefreshMs(m_refresh->itemData(index).toInt());
}
