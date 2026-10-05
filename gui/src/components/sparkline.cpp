#include "sparkline.h"

#include "theme/theme.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

#include <algorithm>
#include <cmath>

Sparkline::Sparkline(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(28);
    m_color = Theme::textSecondary();
}

void Sparkline::setPoints(const QVector<SparkPoint> &points)
{
    m_points = points;
    update();
}

void Sparkline::setLineColor(const QColor &color)
{
    m_color = color;
    update();
}

void Sparkline::setFillAlpha(int alpha)
{
    m_fillAlpha = alpha;
    update();
}

void Sparkline::setMinimumSpan(double span)
{
    m_minSpan = span;
    update();
}

void Sparkline::setShowGrid(bool on)
{
    m_showGrid = on;
    update();
}

QSize Sparkline::sizeHint() const { return QSize(200, 40); }
QSize Sparkline::minimumSizeHint() const { return QSize(80, 28); }

void Sparkline::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = QRectF(rect()).adjusted(1.0, 2.0, -1.0, -2.0);
    const qreal top = r.top();
    const qreal bot = r.bottom();

    // Range over valid samples only.
    double mn = 0.0;
    double mx = 0.0;
    int validCount = 0;
    for (const SparkPoint &s : m_points) {
        if (!s.valid)
            continue;
        if (validCount == 0) {
            mn = mx = s.value;
        } else {
            mn = std::min(mn, s.value);
            mx = std::max(mx, s.value);
        }
        ++validCount;
    }

    if (validCount == 0) {
        // No data: a quiet dotted baseline.
        QPen pen(Theme::border(), 1.4, Qt::DotLine);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        const qreal y = (top + bot) / 2.0;
        p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
        return;
    }

    if (mx - mn < m_minSpan) {
        const double mid = (mx + mn) / 2.0;
        mn = mid - m_minSpan / 2.0;
        mx = mid + m_minSpan / 2.0;
    }

    const auto xFor = [&](int i) {
        const double f = (m_points.size() <= 1) ? 0.0
                                                : static_cast<double>(i) / (m_points.size() - 1);
        return r.left() + f * r.width();
    };
    const auto yFor = [&](double v) {
        const double f = (mx > mn) ? (v - mn) / (mx - mn) : 0.5;
        return bot - f * (bot - top);
    };

    if (m_showGrid) {
        QColor grid = Theme::border();
        grid.setAlpha(70);
        p.setPen(QPen(grid, 1));
        const qreal y = (top + bot) / 2.0;
        p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
    }

    // Draw each contiguous valid segment as a smooth line (+ soft fill).
    int i = 0;
    while (i < m_points.size()) {
        if (!m_points.at(i).valid) {
            ++i;
            continue;
        }
        int j = i;
        while (j + 1 < m_points.size() && m_points.at(j + 1).valid)
            ++j;

        if (j > i) {
            QPainterPath path;
            path.moveTo(xFor(i), yFor(m_points.at(i).value));
            for (int k = i + 1; k < j; ++k) {
                const QPointF cur(xFor(k), yFor(m_points.at(k).value));
                const QPointF next(xFor(k + 1), yFor(m_points.at(k + 1).value));
                const QPointF mid((cur.x() + next.x()) / 2.0, (cur.y() + next.y()) / 2.0);
                path.quadTo(cur, mid);
            }
            path.lineTo(xFor(j), yFor(m_points.at(j).value));

            if (m_fillAlpha > 0) {
                QPainterPath fill = path;
                fill.lineTo(xFor(j), bot);
                fill.lineTo(xFor(i), bot);
                fill.closeSubpath();
                QLinearGradient grad(0, top, 0, bot);
                QColor c = m_color;
                c.setAlpha(m_fillAlpha);
                grad.setColorAt(0.0, c);
                c.setAlpha(0);
                grad.setColorAt(1.0, c);
                p.setPen(Qt::NoPen);
                p.setBrush(grad);
                p.drawPath(fill);
            }

            QPen pen(m_color, 1.8);
            pen.setJoinStyle(Qt::RoundJoin);
            pen.setCapStyle(Qt::RoundCap);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawPath(path);
        }
        i = j + 1;
    }

    // Highlight the last valid sample.
    for (int k = m_points.size() - 1; k >= 0; --k) {
        if (!m_points.at(k).valid)
            continue;
        const QPointF pt(xFor(k), yFor(m_points.at(k).value));
        QColor halo = m_color;
        halo.setAlpha(55);
        p.setPen(Qt::NoPen);
        p.setBrush(halo);
        p.drawEllipse(pt, 6.0, 6.0);
        p.setBrush(m_color);
        p.drawEllipse(pt, 3.0, 3.0);
        break;
    }
}
