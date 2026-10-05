#pragma once

#include <QWidget>

class QVBoxLayout;

// Product navigation rail. Owns the wordmark, the primary page items and a
// bottom-pinned About entry. Emits the target page index; it never changes the
// pages itself (the page container owns that).
class Sidebar : public QWidget
{
    Q_OBJECT

public:
    enum class Icon { Dashboard, Cooling, Device, About };

    explicit Sidebar(QWidget *parent = nullptr);

    // Adds a navigation entry and returns its index (top-to-bottom order).
    int addItem(const QString &label, Icon icon);
    // Adds the About entry pinned at the bottom.
    void addBottomItem(const QString &label, Icon icon);

    void setCurrentIndex(int index);
    int currentIndex() const { return m_current; }

signals:
    void pageSelected(int index);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QVBoxLayout *m_itemsLayout = nullptr;
    QVBoxLayout *m_bottomLayout = nullptr;
    int m_current = -1;
    int m_nextIndex = 0;
};
