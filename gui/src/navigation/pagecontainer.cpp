#include "pagecontainer.h"

#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

// Wraps a page and exposes a settable left inset used for the slide-in, plus a
// child opacity effect used for the fade-in.
class PageHost : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int offset READ offset WRITE setOffset)

public:
    PageHost(QWidget *page, QWidget *parent)
        : QWidget(parent), m_layout(new QVBoxLayout(this)), m_effect(new QGraphicsOpacityEffect(this))
    {
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(0);
        m_layout->addWidget(page);
        m_effect->setOpacity(1.0);
        page->setGraphicsEffect(m_effect);
    }

    int offset() const { return m_offset; }
    void setOffset(int v)
    {
        m_offset = v;
        m_layout->setContentsMargins(v, 0, 0, 0);
    }

    QGraphicsOpacityEffect *effect() const { return m_effect; }

private:
    QVBoxLayout *m_layout;
    QGraphicsOpacityEffect *m_effect;
    int m_offset = 0;
};

} // namespace

PageContainer::PageContainer(QWidget *parent)
    : QWidget(parent)
{
    m_stack = new QStackedWidget(this);
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->addWidget(m_stack);
}

int PageContainer::addPage(QWidget *page)
{
    auto *host = new PageHost(page, m_stack);
    return m_stack->addWidget(host);
}

int PageContainer::currentIndex() const
{
    return m_current;
}

void PageContainer::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_stack->count() || index == m_current)
        return;

    const int old = m_current;
    auto *host = qobject_cast<PageHost *>(m_stack->widget(index));
    m_stack->setCurrentIndex(index);
    m_current = index;

    bool animate = true;
    if (const char *e = qgetenv("EXCALIBUR_NO_ANIM").constData(); e && *e)
        animate = false;
    if (!animate || !host)
        return;

    const int dir = (old < 0 || index > old) ? 1 : -1;
    const int start = 18 * dir;

    host->setOffset(start);
    host->effect()->setOpacity(0.0);

    auto *group = new QParallelAnimationGroup(host);
    auto *fade = new QPropertyAnimation(host->effect(), "opacity", group);
    fade->setDuration(190);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->setEasingCurve(QEasingCurve::OutCubic);

    auto *slide = new QPropertyAnimation(host, "offset", group);
    slide->setDuration(210);
    slide->setStartValue(start);
    slide->setEndValue(0);
    slide->setEasingCurve(QEasingCurve::OutCubic);

    connect(group, &QParallelAnimationGroup::finished, host, [host]() { host->setOffset(0); });
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

#include "pagecontainer.moc"
