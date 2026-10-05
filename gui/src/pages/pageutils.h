#pragma once

#include "theme/theme.h"

#include <QLabel>
#include <QString>

// Small shared helpers for page scaffolding. Header-only on purpose: these are
// trivial label factories, not a component in their own right.
namespace PageUtils {

inline void colorLabel(QLabel *l, const QColor &c)
{
    QPalette p = l->palette();
    p.setColor(QPalette::WindowText, c);
    l->setPalette(p);
}

inline QLabel *makeLabel(const QString &text, const QFont &f, const QColor &c)
{
    auto *l = new QLabel(text);
    l->setFont(f);
    colorLabel(l, c);
    return l;
}

inline QLabel *pageTitle(const QString &text)
{
    return makeLabel(text, Theme::font(20, QFont::Bold, 100), Theme::textPrimary());
}

inline QLabel *pageSubtitle(const QString &text)
{
    auto *l = makeLabel(text, Theme::font(11, QFont::Normal), Theme::textSecondary());
    l->setWordWrap(true);
    return l;
}

inline QLabel *label(const QString &text)
{
    return makeLabel(text, Theme::labelFont(9, QFont::DemiBold, 145), Theme::textMuted());
}

} // namespace PageUtils
