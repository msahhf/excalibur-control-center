#pragma once

#include <QColor>
#include <QString>
#include <QVector>
#include <QWidget>

// One history sample. `valid == false` represents a missing reading and breaks
// the line instead of being drawn as a fake zero. `timestampMs` is the monotonic
// sample time (see TelemetryModel::HistorySample); 0 means "unknown".
struct SparkPoint {
    double value = 0.0;
    bool valid = false;
    qint64 timestampMs = 0;
};

// Human-readable age of a history sample for the hover readout, e.g. "now",
// "5s ago", "42s ago". Kept as a single small helper so the exact wording can be
// changed in one place. Negative/future ages are reported as "now".
QString formatSampleAge(qint64 ageMs);

// Age (ms) of the sample at `index` relative to the newest sample in `points`
// (the chart's right edge). Uses real sample timestamps when present; falls back
// to assuming 1 Hz sampling for timestamp-less points. Returns 0 if unknown.
qint64 sampleAgeMs(const QVector<SparkPoint> &points, int index);

// A restrained history chart drawn with QPainter (no Qt Charts). Smooth line,
// soft fill under the line, one quiet gridline, no axis numbers and the current
// value highlighted. Deliberately not a scientific monitor look.
class Sparkline : public QWidget
{
    Q_OBJECT

public:
    explicit Sparkline(QWidget *parent = nullptr);

    void setPoints(const QVector<SparkPoint> &points);
    void setLineColor(const QColor &color);
    void setFillAlpha(int alpha); // 0 disables the fill
    void setMinimumSpan(double span); // avoid exaggerating a nearly flat line
    void setShowGrid(bool on);

    // Formats the optional hover readout (e.g. "46 °C").
    void setDecimals(int decimals);
    void setValueSuffix(const QString &suffix);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    QVector<SparkPoint> m_points;
    QColor m_color;
    int m_fillAlpha = 70;
    double m_minSpan = 1.0;
    bool m_showGrid = true;
    int m_decimals = 0;
    QString m_suffix;
    int m_hoverIndex = -1;
};
