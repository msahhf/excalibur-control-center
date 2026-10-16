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

    root->addWidget(PageUtils::pageTitle(tr("Settings")));
    root->addWidget(PageUtils::pageSubtitle(
        tr("Application preferences. Changes are saved immediately; language "
           "changes apply on restart.")));

    auto *title = new QLabel(tr("PREFERENCES"));
    title->setFont(Theme::labelFont(9, QFont::DemiBold, 150));
    {
        QPalette p = title->palette();
        p.setColor(QPalette::WindowText, Theme::textMuted());
        title->setPalette(p);
    }

    auto *surface = new SurfacePanel;
    // The panel hosts the grid plus one extra row: the restart hint that only
    // appears while a language change is pending. Keeping both in a column
    // layout (instead of a grid row) lets the hint collapse to nothing when it
    // is hidden, so the panel keeps its symmetric padding.
    auto *surfaceLayout = new QVBoxLayout(surface);
    surfaceLayout->setContentsMargins(Theme::Space::XL, Theme::Space::M, Theme::Space::XL,
                                      Theme::Space::M);
    surfaceLayout->setSpacing(Theme::Space::S);

    auto *gridHost = new QWidget(surface);
    auto *grid = new QGridLayout(gridHost);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(Theme::Space::XL);
    grid->setVerticalSpacing(Theme::Space::S);
    grid->setColumnStretch(0, 1);

    // Language sits first: it is the one preference that needs a restart, so it
    // should be read before the "changes apply immediately" rows. The language
    // names are endonyms (English / Türkçe) and are never passed to tr(); the
    // shared tr("System") is the same word the theme combo uses.
    m_language = makeSelector(gridHost);
    m_language->addItem(tr("System"), int(UserSettings::Language::System));
    m_language->addItem(QStringLiteral("English"), int(UserSettings::Language::English));
    m_language->addItem(QStringLiteral("Türkçe"), int(UserSettings::Language::Turkish));
    // The caption is a separate QLabel, so the combo needs its own accessible
    // name to be announced by assistive technology.
    m_language->setAccessibleName(tr("Language"));

    m_theme = makeSelector(gridHost);
    m_theme->addItem(tr("System"), int(UserSettings::AppTheme::System));
    m_theme->addItem(tr("Light"), int(UserSettings::AppTheme::Light));
    m_theme->addItem(tr("Dark"), int(UserSettings::AppTheme::Dark));

    m_unit = makeSelector(gridHost);
    m_unit->addItem(tr("Celsius"), int(Units::TemperatureUnit::Celsius));
    m_unit->addItem(tr("Fahrenheit"), int(Units::TemperatureUnit::Fahrenheit));

    m_refresh = makeSelector(gridHost);
    for (int ms : UserSettings::refreshOptions())
        m_refresh->addItem(tr("%1 s").arg(ms / 1000), ms);

    int r = 0;
    grid->addWidget(captionLabel(tr("Language")), r, 0, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(m_language, r++, 1, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(captionLabel(tr("Theme")), r, 0, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(m_theme, r++, 1, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(captionLabel(tr("Temperature unit")), r, 0, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(m_unit, r++, 1, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(captionLabel(tr("Refresh interval")), r, 0, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(m_refresh, r++, 1, Qt::AlignRight | Qt::AlignVCenter);

    // Restart hint, in the page-subtitle style. It only shows while the stored
    // language differs from the one this page was built with (see syncFromSettings),
    // i.e. exactly while the change is still pending.
    m_languageHint = PageUtils::makeLabel(
        tr("The new language will be used the next time you start the application."),
        Theme::font(11, QFont::Normal), Theme::textSecondary());
    m_languageHint->setWordWrap(true);
    m_languageHint->setVisible(false);
    surfaceLayout->addWidget(gridHost);
    surfaceLayout->addWidget(m_languageHint);

    auto *group = new QWidget;
    auto *gv = new QVBoxLayout(group);
    gv->setContentsMargins(0, 0, 0, 0);
    gv->setSpacing(Theme::Space::S);
    gv->addWidget(title);
    gv->addWidget(surface);
    root->addWidget(group);

    m_reset = new ActionButton(tr("Reset to Defaults"));
    root->addWidget(m_reset, 0, Qt::AlignLeft);
    root->addStretch(1);

    connect(m_language, &QComboBox::currentIndexChanged, this, &SettingsPage::onLanguageIndex);
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
    connect(&us, &UserSettings::languageChanged, this, &SettingsPage::syncFromSettings);

    m_startLanguage = us.language();
    syncFromSettings();
}

void SettingsPage::syncFromSettings()
{
    UserSettings &us = UserSettings::instance();
    const QSignalBlocker b1(m_language);
    const QSignalBlocker b2(m_theme);
    const QSignalBlocker b3(m_unit);
    const QSignalBlocker b4(m_refresh);
    m_language->setCurrentIndex(m_language->findData(int(us.language())));
    m_theme->setCurrentIndex(m_theme->findData(int(us.theme())));
    m_unit->setCurrentIndex(m_unit->findData(int(us.tempUnit())));
    m_refresh->setCurrentIndex(m_refresh->findData(us.refreshMs()));

    // Only a *pending* language change gets the restart hint: the stored value
    // differs from the one this page (i.e. this run) was built with. Going back
    // to the original value hides it again, so it never lies.
    m_languageHint->setVisible(us.language() != m_startLanguage);
}

void SettingsPage::onLanguageIndex(int index)
{
    UserSettings::instance().setLanguage(
        static_cast<UserSettings::Language>(m_language->itemData(index).toInt()));
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
