#pragma once

#include <QPushButton>

class QPropertyAnimation;

// A quiet, product-styled button with a soft animated hover and a visible
// keyboard focus ring. Keeps QPushButton's keyboard/signal behaviour.
class ActionButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)

public:
    explicit ActionButton(const QString &text, QWidget *parent = nullptr);

    qreal hoverProgress() const { return m_hover; }
    void setHoverProgress(qreal value);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *) override;
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;

private:
    void animateTo(qreal target);

    QPropertyAnimation *m_anim = nullptr;
    qreal m_hover = 0.0;
    bool m_focusVisible = false;
};
