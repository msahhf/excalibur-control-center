#pragma once

#include <QWidget>

class QStackedWidget;

// Holds the product pages and switches between them with a restrained
// fade + small horizontal slide (Section 12). Pages are plain QWidgets; the
// container owns presentation only.
class PageContainer : public QWidget
{
    Q_OBJECT

public:
    explicit PageContainer(QWidget *parent = nullptr);

    int addPage(QWidget *page);
    void setCurrentIndex(int index);
    int currentIndex() const;

private:
    QStackedWidget *m_stack = nullptr;
    int m_current = -1;
};
