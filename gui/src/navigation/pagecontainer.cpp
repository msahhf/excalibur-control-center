#include "pagecontainer.h"

#include <QStackedWidget>
#include <QVBoxLayout>

PageContainer::PageContainer(QWidget *parent)
    : QWidget(parent)
{
    m_stack = new QStackedWidget(this);
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);
    v->addWidget(m_stack);
}

int PageContainer::addPage(QWidget *page)
{
    // Pages are created once and kept alive. Navigation only changes which one is
    // visible, so a page never loses its last-known telemetry while off-screen.
    return m_stack->addWidget(page);
}

int PageContainer::currentIndex() const
{
    return m_current;
}

int PageContainer::count() const
{
    return m_stack->count();
}

void PageContainer::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_stack->count() || index == m_current)
        return;

    // Instant switch (no fade, no offset/reflow). A transition on a telemetry page
    // looked like a reload, and the animated left margin reflowed the page on every
    // navigation. The pages already hold their last-known snapshot, so the swap is
    // complete on the next frame.
    m_current = index;
    m_stack->setCurrentIndex(index);
}
