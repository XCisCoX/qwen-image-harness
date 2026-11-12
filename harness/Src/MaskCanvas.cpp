#include "MaskCanvas.h"

#include "Icons.h"
#include "Theme.h"

#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QUrl>
#include <QWheelEvent>
#include <QtMath>

namespace {
const int kMaxHistory = 18;
const qreal kMinZoom = 0.05;
const qreal kMaxZoom = 8.0;

QImage readImage(const QString &path)
{
    QImageReader reader(path);
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    reader.setAutoTransform(true);
#endif
    return reader.read();
}
} // namespace

MaskCanvas::MaskCanvas(QWidget *parent)
    : QWidget(parent)
    , m_zoom(1.0)
    , m_fit(true)
    , m_tool(Brush)
    , m_brush(36)
    , m_hasContent(false)
    , m_painting(false)
    , m_panning(false)
    , m_hover(false)
    , m_dragOver(false)
    , m_tintDirty(false)
{
    setMinimumSize(280, 160);
    setMouseTracking(true);
    setAcceptDrops(true);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);
}

bool MaskCanvas::loadBase(const QString &path, QString *error)
{
    const QImage img = readImage(path);
    if (img.isNull()) {
        if (error)
            *error = QString("Cannot open image: %1").arg(QDir::toNativeSeparators(path));
        return false;
    }

    const bool sizeChanged = m_mask.size() != img.size();
    m_base = img;
    m_basePath = path;

    if (m_mask.isNull() || sizeChanged) {
        m_mask = QImage(m_base.size(), QImage::Format_ARGB32);
        m_mask.fill(Qt::transparent);
        m_hasContent = false;
        m_undo.clear();
        m_redo.clear();
        m_tintDirty = true;
        emit historyChanged();
    }

    m_fit = true;
    fitToView();
    emit baseChanged();
    emit maskChanged();
    update();
    return true;
}

void MaskCanvas::clearBase()
{
    m_base = QImage();
    m_basePath.clear();
    m_mask = QImage();
    m_tint = QImage();
    m_hasContent = false;
    m_undo.clear();
    m_redo.clear();
    emit baseChanged();
    emit maskChanged();
    emit historyChanged();
    update();
}

void MaskCanvas::setTool(Tool tool)
{
    m_tool = tool;
    update();
}

void MaskCanvas::setBrushSize(int pixels)
{
    m_brush = qBound(2, pixels, 320);
    update();
}

QRectF MaskCanvas::baseRect() const
{
    if (m_base.isNull())
        return QRectF();
    const QSizeF s(m_base.width() * m_zoom, m_base.height() * m_zoom);
    return QRectF(QPointF((width() - s.width()) / 2.0 + m_offset.x(),
                          (height() - s.height()) / 2.0 + m_offset.y()),
                  s);
}

QPointF MaskCanvas::toImage(const QPointF &widgetPos) const
{
    const QRectF r = baseRect();
    if (r.width() <= 0 || r.height() <= 0)
        return QPointF();
    return (widgetPos - r.topLeft()) / m_zoom;
}

void MaskCanvas::fitToView()
{
    if (m_base.isNull() || width() <= 0 || height() <= 0)
        return;
    const qreal sx = qreal(width() - 40) / qreal(m_base.width());
    const qreal sy = qreal(height() - 40) / qreal(m_base.height());
    m_zoom = qBound<qreal>(kMinZoom, qMin(sx, sy), kMaxZoom);
    m_offset = QPointF();
    m_fit = true;
    update();
}

void MaskCanvas::zoomBy(qreal factor)
{
    if (m_base.isNull())
        return;
    m_zoom = qBound<qreal>(kMinZoom, m_zoom * factor, kMaxZoom);
    m_fit = false;
    update();
}

void MaskCanvas::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_fit)
        fitToView();
}

void MaskCanvas::rebuildTint()
{
    if (m_mask.isNull()) {
        m_tint = QImage();
        m_tintDirty = false;
        return;
    }

    m_tint = QImage(m_mask.size(), QImage::Format_ARGB32_Premultiplied);
    m_tint.fill(Qt::transparent);

    QPainter p(&m_tint);
    p.fillRect(m_tint.rect(), Theme::accent());
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawImage(0, 0, m_mask);
    p.end();

    m_tintDirty = false;
}

void MaskCanvas::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(0x11, 0x13, 0x18));

    if (m_base.isNull()) {
        QRectF box = QRectF(rect()).adjusted(16, 16, -16, -16);
        QPainterPath shape;
        shape.addRoundedRect(box, 12, 12);
        QPen pen(m_dragOver ? Theme::accent() : QColor(0x2A, 0x30, 0x3C), 1.4, Qt::DashLine);
        p.setPen(pen);
        p.setBrush(QColor(0x13, 0x15, 0x1A));
        p.drawPath(shape);

        const QPixmap icon = Icons::pixmap(Icons::Image, 34, Theme::TextFaint);
        p.drawPixmap(int(box.center().x() - 17), int(box.center().y() - 44), icon);

        p.setPen(Theme::TextDim);
        p.setFont(Theme::ui(13, QFont::DemiBold));
        p.drawText(QRectF(box.left(), box.center().y() + 4, box.width(), 24),
                   Qt::AlignHCenter | Qt::AlignTop, "No primary image");

        p.setPen(Theme::TextFaint);
        p.setFont(Theme::ui(11));
        p.drawText(QRectF(box.left(), box.center().y() + 28, box.width(), 40),
                   Qt::AlignHCenter | Qt::AlignTop,
                   "Drop an image here, or click to choose the picture\nyou want to mask.");
        return;
    }

    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const QRectF r = baseRect();
    p.drawImage(r, m_base);

    if (m_tintDirty)
        rebuildTint();

    if (!m_tint.isNull()) {
        p.setOpacity(0.55);
        p.drawImage(r, m_tint);
        p.setOpacity(1.0);
    }

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(0x2C, 0x33, 0x41), 1.0));
    p.drawRect(r.adjusted(-0.5, -0.5, 0.5, 0.5));

    if (m_hover && !m_panning) {
        const qreal radius = m_brush / 2.0;
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(0, 0, 0, 170), 3.0));
        p.drawEllipse(m_cursorPos, radius, radius);
        p.setPen(QPen(QColor(255, 255, 255, 230), 1.4));
        p.drawEllipse(m_cursorPos, radius, radius);
    }
}

void MaskCanvas::pushUndo()
{
    if (m_mask.isNull())
        return;
    m_undo.append(m_mask);
    if (m_undo.size() > kMaxHistory)
        m_undo.remove(0);
    m_redo.clear();
    emit historyChanged();
}

bool MaskCanvas::computeHasContent() const
{
    if (m_mask.isNull())
        return false;
    for (int y = 0; y < m_mask.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(m_mask.constScanLine(y));
        for (int x = 0; x < m_mask.width(); ++x) {
            if (qAlpha(line[x]) > 4)
                return true;
        }
    }
    return false;
}

void MaskCanvas::paintDot(const QPointF &imagePos)
{
    if (m_mask.isNull())
        return;
    QPainter p(&m_mask);
    p.setRenderHint(QPainter::Antialiasing, true);
    const qreal width = qMax<qreal>(1.0, m_brush / m_zoom);
    QPen pen(Qt::white, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::white);
    if (m_tool == Eraser) {
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        p.setBrush(Qt::black);
        p.setPen(QPen(Qt::black, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    }
    p.drawPoint(imagePos);
    p.end();
    m_tintDirty = true;
}

void MaskCanvas::paintLine(const QPointF &from, const QPointF &to)
{
    if (m_mask.isNull())
        return;
    QPainter p(&m_mask);
    p.setRenderHint(QPainter::Antialiasing, true);
    const qreal width = qMax<qreal>(1.0, m_brush / m_zoom);
    QPen pen(Qt::white, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    if (m_tool == Eraser) {
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        p.setPen(QPen(Qt::black, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    }
    p.drawLine(from, to);
    p.end();
    m_tintDirty = true;
}

void MaskCanvas::mousePressEvent(QMouseEvent *event)
{
    if (m_base.isNull()) {
        if (event->button() == Qt::LeftButton)
            emit browseRequested();
        return;
    }

    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_panStart = event->pos();
        m_offsetStart = m_offset;
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() != Qt::LeftButton)
        return;

    pushUndo();
    m_painting = true;
    m_lastImagePos = toImage(event->pos());
    paintDot(m_lastImagePos);
    m_hasContent = true;
    emit maskChanged();
    update();
}

void MaskCanvas::mouseMoveEvent(QMouseEvent *event)
{
    m_cursorPos = event->pos();

    if (m_panning) {
        m_offset = m_offsetStart + (event->pos() - m_panStart);
        m_fit = false;
        update();
        return;
    }

    if (m_painting) {
        const QPointF current = toImage(event->pos());
        paintLine(m_lastImagePos, current);
        m_lastImagePos = current;
        emit maskChanged();
        update();
    } else if (m_hover) {
        update();
    }
}

void MaskCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton && m_panning) {
        m_panning = false;
        updateCursorShape();
        return;
    }

    if (event->button() == Qt::LeftButton && m_painting) {
        m_painting = false;
        m_hasContent = computeHasContent();
        m_tintDirty = true;
        emit maskChanged();
        emit historyChanged();
        update();
    }
}

void MaskCanvas::enterEvent(QEvent *event)
{
    m_hover = true;
    updateCursorShape();
    update();
    QWidget::enterEvent(event);
}

void MaskCanvas::leaveEvent(QEvent *event)
{
    m_hover = false;
    unsetCursor();
    update();
    QWidget::leaveEvent(event);
}

void MaskCanvas::updateCursorShape()
{
    if (m_base.isNull()) {
        setCursor(Qt::CrossCursor);
        return;
    }
    setCursor(m_hover ? Qt::BlankCursor : Qt::CrossCursor);
}

void MaskCanvas::wheelEvent(QWheelEvent *event)
{
    if (m_base.isNull()) {
        QWidget::wheelEvent(event);
        return;
    }

    const qreal steps = event->angleDelta().y() / 120.0;
    if (qFuzzyIsNull(steps)) {
        QWidget::wheelEvent(event);
        return;
    }

    const QPointF cursor(event->pos());
    const QRectF before = baseRect();
    const QPointF rel = (cursor - before.topLeft()) / before.width();

    m_zoom = qBound<qreal>(kMinZoom, m_zoom * qPow(1.15, steps), kMaxZoom);
    m_fit = false;

    const QRectF after = baseRect();
    m_offset += cursor - (after.topLeft() + QPointF(rel.x() * after.width(), rel.y() * after.height()));
    update();
    event->accept();
}

void MaskCanvas::clearMask()
{
    if (m_mask.isNull())
        return;
    pushUndo();
    m_mask.fill(Qt::transparent);
    m_hasContent = false;
    m_tintDirty = true;
    emit maskChanged();
    emit historyChanged();
    update();
}

void MaskCanvas::invertMask()
{
    if (m_mask.isNull())
        return;
    pushUndo();

    for (int y = 0; y < m_mask.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(m_mask.scanLine(y));
        for (int x = 0; x < m_mask.width(); ++x) {
            const int alpha = 255 - qAlpha(line[x]);
            line[x] = qRgba(255, 255, 255, alpha);
        }
    }

    m_hasContent = true;
    m_tintDirty = true;
    emit maskChanged();
    emit historyChanged();
    update();
}

bool MaskCanvas::undo()
{
    if (m_undo.isEmpty())
        return false;
    m_redo.append(m_mask);
    m_mask = m_undo.takeLast();
    m_hasContent = computeHasContent();
    m_tintDirty = true;
    emit maskChanged();
    emit historyChanged();
    update();
    return true;
}

bool MaskCanvas::redo()
{
    if (m_redo.isEmpty())
        return false;
    m_undo.append(m_mask);
    m_mask = m_redo.takeLast();
    m_hasContent = computeHasContent();
    m_tintDirty = true;
    emit maskChanged();
    emit historyChanged();
    update();
    return true;
}

void MaskCanvas::resetHistory()
{
    m_undo.clear();
    m_redo.clear();
    emit historyChanged();
}

bool MaskCanvas::loadMaskFile(const QString &path, QString *error)
{
    if (m_base.isNull()) {
        if (error)
            *error = "Choose a primary image before loading a mask.";
        return false;
    }

    QImage img = readImage(path);
    if (img.isNull()) {
        if (error)
            *error = QString("Cannot open mask: %1").arg(QDir::toNativeSeparators(path));
        return false;
    }

    if (img.size() != m_base.size())
        img = img.scaled(m_base.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    img = img.convertToFormat(QImage::Format_ARGB32);

    pushUndo();

    QImage mask(m_base.size(), QImage::Format_ARGB32);
    for (int y = 0; y < mask.height(); ++y) {
        const QRgb *src = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        QRgb *dst = reinterpret_cast<QRgb *>(mask.scanLine(y));
        for (int x = 0; x < mask.width(); ++x)
            dst[x] = qRgba(255, 255, 255, qGray(src[x]));
    }

    m_mask = mask;
    m_hasContent = computeHasContent();
    m_tintDirty = true;
    emit maskChanged();
    emit historyChanged();
    update();
    return true;
}

bool MaskCanvas::saveMaskFile(const QString &path, QString *error)
{
    if (m_mask.isNull()) {
        if (error)
            *error = "Nothing to save yet.";
        return false;
    }

    QImage out(m_mask.size(), QImage::Format_RGB32);
    out.fill(Qt::black);
    for (int y = 0; y < m_mask.height(); ++y) {
        const QRgb *src = reinterpret_cast<const QRgb *>(m_mask.constScanLine(y));
        QRgb *dst = reinterpret_cast<QRgb *>(out.scanLine(y));
        for (int x = 0; x < m_mask.width(); ++x) {
            const int a = qAlpha(src[x]);
            dst[x] = qRgb(a, a, a);
        }
    }

    if (!out.save(path, "PNG")) {
        if (error)
            *error = QString("Cannot write mask: %1").arg(QDir::toNativeSeparators(path));
        return false;
    }
    return true;
}

void MaskCanvas::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        m_dragOver = true;
        update();
        event->acceptProposedAction();
    }
}

void MaskCanvas::dragLeaveEvent(QDragLeaveEvent *event)
{
    m_dragOver = false;
    update();
    QWidget::dragLeaveEvent(event);
}

void MaskCanvas::dropEvent(QDropEvent *event)
{
    m_dragOver = false;
    update();

    const QList<QUrl> urls = event->mimeData()->urls();
    for (int i = 0; i < urls.size(); ++i) {
        if (!urls.at(i).isLocalFile())
            continue;
        const QString path = urls.at(i).toLocalFile();
        const QString suffix = QFileInfo(path).suffix().toLower();
        if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" || suffix == "webp"
            || suffix == "bmp" || suffix == "tif" || suffix == "tiff") {
            emit baseDropped(path);
            event->acceptProposedAction();
            return;
        }
    }
}
