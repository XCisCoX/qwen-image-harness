#include "Icons.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QtMath>

namespace Icons {

static QPainterPath sparklePath(qreal cx, qreal cy, qreal r)
{
    QPainterPath p;
    for (int i = 0; i < 8; ++i) {
        const qreal rr = (i % 2 == 0) ? r : r * 0.26;
        const qreal a = -M_PI / 2 + i * M_PI / 4;
        const QPointF pt(cx + rr * qCos(a), cy + rr * qSin(a));
        if (i == 0)
            p.moveTo(pt);
        else
            p.lineTo(pt);
    }
    p.closeSubpath();
    return p;
}

static QPainterPath starPath(qreal cx, qreal cy, qreal rOuter, qreal rInner)
{
    QPainterPath p;
    for (int i = 0; i < 10; ++i) {
        const qreal r = (i % 2 == 0) ? rOuter : rInner;
        const qreal a = -M_PI / 2 + i * M_PI / 5;
        const QPointF pt(cx + r * qCos(a), cy + r * qSin(a));
        if (i == 0)
            p.moveTo(pt);
        else
            p.lineTo(pt);
    }
    p.closeSubpath();
    return p;
}

static void arrowHead(QPainter &p, const QPointF &tip, const QPointF &dir, qreal len)
{
    const QPointF n(-dir.y(), dir.x());
    const QPointF a = tip - dir * len + n * (len * 0.55);
    const QPointF b = tip - dir * len - n * (len * 0.55);
    p.drawLine(tip, a);
    p.drawLine(tip, b);
}

QPixmap pixmap(Id id, int size, const QColor &color)
{
    if (size <= 0)
        size = 18;

    QPixmap pm(size, size);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.scale(size / 24.0, size / 24.0);

    QPen pen(color, 1.7);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    switch (id) {
    case Sparkles: {
        p.drawPath(sparklePath(9.5, 11.0, 6.4));
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPath(sparklePath(18.0, 5.5, 3.2));
        p.drawPath(sparklePath(18.0, 18.0, 2.4));
        break;
    }
    case Pen: {
        QPainterPath path;
        path.moveTo(4.6, 19.4);
        path.lineTo(7.2, 13.2);
        path.lineTo(15.6, 4.8);
        path.lineTo(19.2, 8.4);
        path.lineTo(10.8, 16.8);
        path.closeSubpath();
        p.drawPath(path);
        p.drawLine(QPointF(13.9, 6.5), QPointF(17.5, 10.1));
        break;
    }
    case Swap:
    case Layers: {
        if (id == Swap) {
            p.drawLine(QPointF(4.5, 9.0), QPointF(17.5, 9.0));
            arrowHead(p, QPointF(19.5, 9.0), QPointF(1, 0), 5.0);
            p.drawLine(QPointF(19.5, 15.0), QPointF(6.5, 15.0));
            arrowHead(p, QPointF(4.5, 15.0), QPointF(-1, 0), 5.0);
        } else {
            QPainterPath a;
            a.moveTo(12, 3.5); a.lineTo(20, 8); a.lineTo(12, 12.5); a.lineTo(4, 8); a.closeSubpath();
            p.drawPath(a);
            p.drawLine(QPointF(4, 12.5), QPointF(12, 17));
            p.drawLine(QPointF(20, 12.5), QPointF(12, 17));
            p.drawLine(QPointF(4, 17.0), QPointF(12, 21.5));
            p.drawLine(QPointF(20, 17.0), QPointF(12, 21.5));
        }
        break;
    }
    case Mask: {
        p.drawRoundedRect(QRectF(3.2, 3.2, 17.6, 17.6), 3.5, 3.5);
        QPainterPath stroke;
        stroke.moveTo(6.8, 16.6);
        stroke.lineTo(11.4, 6.6);
        stroke.lineTo(17.4, 10.6);
        p.drawPath(stroke);
        break;
    }
    case Grid: {
        const qreal s = 7.0, r = 2.2;
        p.drawRoundedRect(QRectF(3.5, 3.5, s, s), r, r);
        p.drawRoundedRect(QRectF(13.5, 3.5, s, s), r, r);
        p.drawRoundedRect(QRectF(3.5, 13.5, s, s), r, r);
        p.drawRoundedRect(QRectF(13.5, 13.5, s, s), r, r);
        break;
    }
    case Terminal: {
        p.drawPolyline(QPolygonF() << QPointF(6.5, 6.5) << QPointF(11.0, 12.0) << QPointF(6.5, 17.5));
        p.drawLine(QPointF(13.0, 17.5), QPointF(18.5, 17.5));
        break;
    }
    case Gear: {
        p.drawEllipse(QPointF(12, 12), 3.4, 3.4);
        for (int i = 0; i < 8; ++i) {
            const qreal a = i * M_PI / 4;
            p.drawLine(QPointF(12 + 6.2 * qCos(a), 12 + 6.2 * qSin(a)),
                       QPointF(12 + 8.6 * qCos(a), 12 + 8.6 * qSin(a)));
        }
        break;
    }
    case Play: {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        QPainterPath t;
        t.moveTo(8.0, 5.0);
        t.lineTo(19.0, 12.0);
        t.lineTo(8.0, 19.0);
        t.closeSubpath();
        p.drawPath(t);
        break;
    }
    case Stop: {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(QRectF(6.0, 6.0, 12.0, 12.0), 2.5, 2.5);
        break;
    }
    case Pause: {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(QRectF(7.0, 5.5, 3.2, 13.0), 1.2, 1.2);
        p.drawRoundedRect(QRectF(13.8, 5.5, 3.2, 13.0), 1.2, 1.2);
        break;
    }
    case Plus:
        p.drawLine(QPointF(12, 5), QPointF(12, 19));
        p.drawLine(QPointF(5, 12), QPointF(19, 12));
        break;
    case Close:
        p.drawLine(QPointF(6.5, 6.5), QPointF(17.5, 17.5));
        p.drawLine(QPointF(17.5, 6.5), QPointF(6.5, 17.5));
        break;
    case Minimize:
        p.drawLine(QPointF(5.5, 12), QPointF(18.5, 12));
        break;
    case Maximize:
        p.drawRoundedRect(QRectF(5.5, 5.5, 13.0, 13.0), 2.0, 2.0);
        break;
    case Restore:
        p.drawRoundedRect(QRectF(4.5, 7.5, 11.0, 11.0), 2.0, 2.0);
        p.drawPolyline(QPolygonF() << QPointF(8.5, 7.5) << QPointF(8.5, 4.5)
                                   << QPointF(19.5, 4.5) << QPointF(19.5, 15.5)
                                   << QPointF(15.5, 15.5));
        break;
    case ChevronDown:
        p.drawPolyline(QPolygonF() << QPointF(7.0, 9.8) << QPointF(12.0, 14.8) << QPointF(17.0, 9.8));
        break;
    case ChevronRight:
        p.drawPolyline(QPolygonF() << QPointF(9.8, 7.0) << QPointF(14.8, 12.0) << QPointF(9.8, 17.0));
        break;
    case ArrowLeft:
        p.drawLine(QPointF(19.0, 12.0), QPointF(5.0, 12.0));
        p.drawPolyline(QPolygonF() << QPointF(11.0, 6.0) << QPointF(5.0, 12.0) << QPointF(11.0, 18.0));
        break;
    case ArrowRight:
        p.drawLine(QPointF(5.0, 12.0), QPointF(19.0, 12.0));
        p.drawPolyline(QPolygonF() << QPointF(13.0, 6.0) << QPointF(19.0, 12.0) << QPointF(13.0, 18.0));
        break;
    case Folder: {
        QPainterPath f;
        f.moveTo(3.2, 18.8);
        f.lineTo(3.2, 6.4);
        f.lineTo(9.2, 6.4);
        f.lineTo(11.2, 8.8);
        f.lineTo(20.8, 8.8);
        f.lineTo(20.8, 18.8);
        f.closeSubpath();
        p.drawPath(f);
        break;
    }
    case Trash: {
        p.drawLine(QPointF(4.5, 7.0), QPointF(19.5, 7.0));
        p.drawPolyline(QPolygonF() << QPointF(9.5, 7.0) << QPointF(9.5, 4.8)
                                   << QPointF(14.5, 4.8) << QPointF(14.5, 7.0));
        p.drawPolyline(QPolygonF() << QPointF(6.0, 7.0) << QPointF(7.0, 19.8)
                                   << QPointF(17.0, 19.8) << QPointF(18.0, 7.0));
        p.drawLine(QPointF(10.4, 10.4), QPointF(10.8, 16.4));
        p.drawLine(QPointF(13.6, 10.4), QPointF(13.2, 16.4));
        break;
    }
    case Refresh: {
        p.drawArc(QRectF(5.0, 5.0, 14.0, 14.0), 50 * 16, 280 * 16);
        arrowHead(p, QPointF(17.6, 8.6), QPointF(0.7, -0.7), 5.0);
        break;
    }
    case Copy: {
        p.drawRoundedRect(QRectF(8.5, 8.5, 11.5, 11.5), 2.5, 2.5);
        QPainterPath back;
        back.moveTo(15.5, 4.5);
        back.lineTo(5.0, 4.5);
        back.lineTo(5.0, 15.0);
        p.drawPath(back);
        break;
    }
    case Save:
    case Download: {
        p.drawLine(QPointF(12, 4.2), QPointF(12, 15.0));
        p.drawPolyline(QPolygonF() << QPointF(7.4, 10.4) << QPointF(12.0, 15.0) << QPointF(16.6, 10.4));
        p.drawPolyline(QPolygonF() << QPointF(4.5, 16.0) << QPointF(4.5, 19.8)
                                   << QPointF(19.5, 19.8) << QPointF(19.5, 16.0));
        break;
    }
    case Undo: {
        p.drawArc(QRectF(6.5, 7.0, 12.0, 11.0), 100 * 16, 250 * 16);
        arrowHead(p, QPointF(7.0, 8.4), QPointF(-0.2, -0.98), 4.6);
        break;
    }
    case Redo: {
        p.drawArc(QRectF(5.5, 7.0, 12.0, 11.0), 80 * 16, -250 * 16);
        arrowHead(p, QPointF(17.0, 8.4), QPointF(0.2, -0.98), 4.6);
        break;
    }
    case Brush: {
        p.drawEllipse(QPointF(8.6, 15.4), 3.3, 3.3);
        p.drawLine(QPointF(10.9, 13.1), QPointF(18.6, 5.4));
        p.drawLine(QPointF(17.2, 4.0), QPointF(20.0, 6.8));
        break;
    }
    case Eraser: {
        QPainterPath e;
        e.moveTo(3.6, 14.6);
        e.lineTo(10.4, 5.6);
        e.lineTo(19.2, 11.4);
        e.lineTo(12.2, 20.0);
        e.closeSubpath();
        p.drawPath(e);
        p.drawLine(QPointF(7.4, 17.6), QPointF(14.6, 10.4));
        break;
    }
    case Image: {
        p.drawRoundedRect(QRectF(3.2, 5.0, 17.6, 14.0), 3.0, 3.0);
        p.drawEllipse(QPointF(8.6, 9.8), 1.7, 1.7);
        p.drawPolyline(QPolygonF() << QPointF(5.4, 17.2) << QPointF(10.6, 12.2)
                                   << QPointF(13.8, 15.2) << QPointF(16.6, 12.8)
                                   << QPointF(19.6, 16.0));
        break;
    }
    case Check:
        p.drawPolyline(QPolygonF() << QPointF(5.0, 12.4) << QPointF(9.8, 17.2) << QPointF(19.2, 7.0));
        break;
    case Alert: {
        QPainterPath t;
        t.moveTo(12.0, 3.6);
        t.lineTo(21.2, 20.0);
        t.lineTo(2.8, 20.0);
        t.closeSubpath();
        p.drawPath(t);
        p.drawLine(QPointF(12, 9.4), QPointF(12, 14.2));
        p.drawPoint(QPointF(12, 17.0));
        break;
    }
    case Dice: {
        p.drawRoundedRect(QRectF(3.6, 3.6, 16.8, 16.8), 4.0, 4.0);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(8.6, 8.6), 1.35, 1.35);
        p.drawEllipse(QPointF(15.4, 8.6), 1.35, 1.35);
        p.drawEllipse(QPointF(12.0, 12.0), 1.35, 1.35);
        p.drawEllipse(QPointF(8.6, 15.4), 1.35, 1.35);
        p.drawEllipse(QPointF(15.4, 15.4), 1.35, 1.35);
        break;
    }
    case Lock: {
        p.drawRoundedRect(QRectF(4.8, 10.8, 14.4, 9.6), 2.4, 2.4);
        p.drawArc(QRectF(8.0, 4.0, 8.0, 10.0), 0, 180 * 16);
        break;
    }
    case Star: {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPath(starPath(12, 12, 8.6, 3.6));
        break;
    }
    case External: {
        p.drawPolyline(QPolygonF() << QPointF(13.0, 4.5) << QPointF(5.0, 4.5)
                                   << QPointF(5.0, 19.0) << QPointF(19.5, 19.0)
                                   << QPointF(19.5, 11.0));
        p.drawLine(QPointF(11.0, 13.0), QPointF(19.0, 5.0));
        p.drawPolyline(QPolygonF() << QPointF(13.4, 5.0) << QPointF(19.0, 5.0) << QPointF(19.0, 10.6));
        break;
    }
    case ZoomIn:
    case ZoomOut: {
        p.drawEllipse(QPointF(10.4, 10.4), 6.2, 6.2);
        p.drawLine(QPointF(15.0, 15.0), QPointF(20.2, 20.2));
        p.drawLine(QPointF(7.2, 10.4), QPointF(13.6, 10.4));
        if (id == ZoomIn)
            p.drawLine(QPointF(10.4, 7.2), QPointF(10.4, 13.6));
        break;
    }
    case Fit: {
        p.drawPolyline(QPolygonF() << QPointF(4.5, 9.0) << QPointF(4.5, 4.5) << QPointF(9.0, 4.5));
        p.drawPolyline(QPolygonF() << QPointF(15.0, 4.5) << QPointF(19.5, 4.5) << QPointF(19.5, 9.0));
        p.drawPolyline(QPolygonF() << QPointF(19.5, 15.0) << QPointF(19.5, 19.5) << QPointF(15.0, 19.5));
        p.drawPolyline(QPolygonF() << QPointF(9.0, 19.5) << QPointF(4.5, 19.5) << QPointF(4.5, 15.0));
        break;
    }
    case Server: {
        p.drawRoundedRect(QRectF(4.0, 4.4, 16.0, 6.2), 2.2, 2.2);
        p.drawRoundedRect(QRectF(4.0, 13.4, 16.0, 6.2), 2.2, 2.2);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(7.4, 7.5), 1.0, 1.0);
        p.drawEllipse(QPointF(7.4, 16.5), 1.0, 1.0);
        break;
    }
    case Info: {
        p.drawEllipse(QPointF(12, 12), 8.4, 8.4);
        p.drawLine(QPointF(12, 11.0), QPointF(12, 17.0));
        p.drawPoint(QPointF(12, 7.8));
        break;
    }
    }

    p.end();
    return pm;
}

QIcon stateIcon(Id id, const QColor &off, const QColor &on, int size)
{
    QIcon ic;
    const QColor disabled = off.darker(160);
    ic.addPixmap(pixmap(id, size, off), QIcon::Normal, QIcon::Off);
    ic.addPixmap(pixmap(id, size, on), QIcon::Normal, QIcon::On);
    ic.addPixmap(pixmap(id, size, on), QIcon::Active, QIcon::Off);
    ic.addPixmap(pixmap(id, size, on), QIcon::Active, QIcon::On);
    ic.addPixmap(pixmap(id, size, on), QIcon::Selected, QIcon::Off);
    ic.addPixmap(pixmap(id, size, on), QIcon::Selected, QIcon::On);
    ic.addPixmap(pixmap(id, size, disabled), QIcon::Disabled, QIcon::Off);
    ic.addPixmap(pixmap(id, size, disabled), QIcon::Disabled, QIcon::On);
    return ic;
}

} // namespace Icons
