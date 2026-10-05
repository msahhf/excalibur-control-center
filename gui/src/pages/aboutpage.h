#pragma once

#include <QWidget>

class QVBoxLayout;

// About page: compact product identity. Filled in Stage 5.
class AboutPage : public QWidget
{
    Q_OBJECT

public:
    explicit AboutPage(QWidget *parent = nullptr);

    QVBoxLayout *body() const { return m_body; }

private:
    QVBoxLayout *m_body = nullptr;
};
