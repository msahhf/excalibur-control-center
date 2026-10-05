#pragma once

#include "theme/theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

// Small shared helpers for page scaffolding. Header-only on purpose: these are
// trivial label factories, not a component in their own right.
namespace PageUtils {

// Default maximum content width. Keeps labels and values from drifting apart on
// very wide windows; the content column stays centered with the same padding on
// every page.
constexpr int kMaxContentWidth = 880;

// Sets up `page` with a single centered content column (max `maxWidth`) and
// returns the inner vertical layout to add page content to. All pages use this
// so alignment/behaviour is consistent.
inline QVBoxLayout *makeCenteredColumn(QWidget *page, int maxWidth = kMaxContentWidth)
{
    auto *outer = new QHBoxLayout(page);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *column = new QWidget;
    column->setMaximumWidth(maxWidth);
    column->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *inner = new QVBoxLayout(column);
    inner->setContentsMargins(Theme::Space::XXL, Theme::Space::XL, Theme::Space::XXL, Theme::Space::XL);
    inner->setSpacing(Theme::Space::L);

    // A large stretch factor lets the column take (almost) all available width
    // up to its maximum; once capped, the leftover is split by the side
    // stretches, centering the column.
    outer->addStretch(1);
    outer->addWidget(column, 100);
    outer->addStretch(1);
    return inner;
}

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
