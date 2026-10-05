#pragma once

#include <QWidget>

// About page: a small, calm product identity block. Not a README.
class AboutPage : public QWidget
{
    Q_OBJECT

public:
    explicit AboutPage(QWidget *parent = nullptr);
};
