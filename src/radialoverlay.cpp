#include "radialoverlay.h"

#include <QPainter>
#include <QPainterPath>
#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <QtMath>
#include <cmath>

RadialOverlay::RadialOverlay(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Window
                 | Qt::FramelessWindowHint
                 | Qt::WindowStaysOnTopHint
                 | Qt::X11BypassWindowManagerHint
                 | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
}

void RadialOverlay::setConfig(const Config& cfg)
{
    m_cfg = cfg;
    m_scrollMode = (cfg.selectionMode == QStringLiteral("scroll"));
}

void RadialOverlay::begin()
{
    if (m_cfg.items.isEmpty()) return;

    m_selected = 0;

    QScreen* scr = QGuiApplication::screenAt(QCursor::pos());
    if (!scr) scr = QGuiApplication::primaryScreen();
    setGeometry(scr->geometry());

    // Seed the virtual cursor from the real one (best effort).
    m_virtualPos = mapFromGlobal(QCursor::pos());

    show();
    raise();

    recomputeSelection();
}

void RadialOverlay::end()
{
    hide();
}

void RadialOverlay::scroll(int delta)
{
    if (!isVisible()) return;
    const int n = m_cfg.items.size();
    if (n == 0) return;
    m_selected = ((m_selected + delta) % n + n) % n;
    update();
}

void RadialOverlay::applyMotion(int dx, int dy)
{
    if (!isVisible()) return;
    if (m_scrollMode)  return;   // wheel mode ignores pointer motion

    m_virtualPos += QPointF(dx, dy);

    // Keep the virtual cursor inside the widget so the angle is well-defined.
    const QRectF r = rect();
    m_virtualPos.setX(qBound(r.left() + 1.0, m_virtualPos.x(), r.right()  - 1.0));
    m_virtualPos.setY(qBound(r.top()  + 1.0, m_virtualPos.y(), r.bottom() - 1.0));

    recomputeSelection();
}

void RadialOverlay::recomputeSelection()
{
    const QPointF c = QRectF(rect()).center();

    const double dx = m_virtualPos.x() - c.x();
    const double dy = m_virtualPos.y() - c.y();
    if (qFuzzyIsNull(dx) && qFuzzyIsNull(dy)) return;

    const double qtAngle = -qRadiansToDegrees(std::atan2(dy, dx));

    const int    n    = m_cfg.items.size();
    const double span = 360.0 / n;

    // Item 0 sits at 90° (top of screen); indices advance clockwise.
    double t = (90.0 - qtAngle) / span;
    int idx  = static_cast<int>(std::lround(t)) % n;
    if (idx < 0) idx += n;

    if (idx != m_selected) {
        m_selected = idx;
        update();
    }
}

void RadialOverlay::paintEvent(QPaintEvent*)
{
    const int n = m_cfg.items.size();
    if (n == 0) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QPointF c      = QRectF(rect()).center();
    const double  inner  = m_cfg.innerRadius;
    const double  outer  = m_cfg.outerRadius;
    const double  span   = 360.0 / n;

    const QRectF outerRect(c.x() - outer, c.y() - outer, outer * 2, outer * 2);
    const QRectF innerRect(c.x() - inner, c.y() - inner, inner * 2, inner * 2);

    QFont font = p.font();
    font.setPointSizeF(qMax(10.0, outer * 0.12));
    font.setBold(true);
    p.setFont(font);

    for (int i = 0; i < n; ++i) {
        const double centerDeg = 90.0 - i * span;
        const double startDeg  = centerDeg + span / 2.0;
        const double sweepDeg  = -span;             // clockwise

        QPainterPath path;
        path.arcMoveTo(outerRect, startDeg);
        path.arcTo(outerRect, startDeg, sweepDeg);
        path.arcTo(innerRect, startDeg + sweepDeg, -sweepDeg);
        path.closeSubpath();

        const bool selected = (i == m_selected);
        const QColor bg = selected ? QColor(245, 245, 245, 210)
                                   : QColor( 25,  25,  30, 165);
        const QColor fg = selected ? QColor( 15,  15,  15)
                                   : QColor(235, 235, 235);

        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawPath(path);

        // Hairline separator between segments.
        p.setPen(QPen(QColor(0, 0, 0, 110), 1));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        // Label at the middle of the ring.
        const double midRad = qDegreesToRadians(centerDeg);
        const double rMid   = (inner + outer) / 2.0;
        const QPointF textPos(c.x() + rMid * std::cos(midRad),
                              c.y() - rMid * std::sin(midRad));

        const double arcLen = rMid * qDegreesToRadians(span);
        const double w      = qBound(40.0, arcLen * 0.9, 220.0);

        p.setPen(fg);
        p.drawText(QRectF(textPos.x() - w / 2, textPos.y() - 22, w, 44),
                   Qt::AlignCenter, m_cfg.items[i].label);
    }
}