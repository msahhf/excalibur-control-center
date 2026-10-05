#pragma once

#include <QWidget>

#include "app/appstate.h"

class QLabel;
class StatusPill;
class QGraphicsOpacityEffect;
class QPropertyAnimation;

// Hero system-state block. This is the first thing the user reads on the
// Dashboard: a plain-language state derived from real telemetry. Raw technical
// terms (DEGRADED, hwmon paths) never appear here.
class HeroStatus : public QWidget
{
    Q_OBJECT

public:
    explicit HeroStatus(QWidget *parent = nullptr);

    void setSystemState(AppState::Status status, AppState::Band band);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QLabel *m_label = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_subtitle = nullptr;
    StatusPill *m_pill = nullptr;
    QWidget *m_content = nullptr;
    QGraphicsOpacityEffect *m_effect = nullptr;
    QPropertyAnimation *m_fade = nullptr;
    AppState::Band m_band = AppState::Band::Normal;
    AppState::Status m_status = AppState::Status::Connected;
    bool m_hasState = false;
};
