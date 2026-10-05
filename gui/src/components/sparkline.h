#pragma once

#include <QColor>
#include <QVector>
#include <QWidget>

// One history sample. `valid == false` represents a missing reading and breaks
// the line instead of being drawn as a fake zero.
struct SparkPoint {
    double value = 0.0;
    bool valid = false;
};

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
