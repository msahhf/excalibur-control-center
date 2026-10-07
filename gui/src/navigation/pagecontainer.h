#pragma once

#include <QWidget>

class QStackedWidget;

// Holds the product pages. Pages are created once and kept alive; switching just
// changes which one is visible (an instant swap, no fade/reflow), so navigating
// never appears to reload and never resets a page's last-known telemetry.
class PageContainer : public QWidget
{
    Q_OBJECT

public:
    explicit PageContainer(QWidget *parent = nullptr);

    int addPage(QWidget *page);
    void setCurrentIndex(int index);
    int currentIndex() const;
    int count() const;

private:
    QStackedWidget *m_stack = nullptr;
    int m_current = -1;
};
