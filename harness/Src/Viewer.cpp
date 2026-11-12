#include "Viewer.h"

#include "Icons.h"
#include "Theme.h"
#include "Widgets.h"

#include <QFileInfo>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QImageReader>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtMath>

// ------------------------------------------------------------------ ImageView

ImageView::ImageView(QWidget *parent)
    : QWidget(parent)
    , m_zoom(1.0)
    , m_fit(true)
    , m_panning(false)
{
    setMinimumSize(200, 160);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

void ImageView::setImage(const QImage &image)
{
    m_image = image;
    m_fit = true;
    m_offset = QPointF();
    fitToWindow();
    update();
}

void ImageView::setImagePath(const QString &path)
{
    m_path = path;
    QImageReader reader(path);
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    reader.setAutoTransform(true);
#endif
    setImage(reader.read());
}

void ImageView::fitToWindow()
{
    if (m_image.isNull() || width() <= 0 || height() <= 0)
        return;

    const qreal sx = qreal(width() - 48) / qreal(m_image.width());
    const qreal sy = qreal(height() - 48) / qreal(m_image.height());
    m_zoom = qMax<qreal>(0.02, qMin(sx, sy));
    m_offset = QPointF();
    m_fit = true;
    emit zoomChanged(m_zoom);
    update();
}

void ImageView::setZoom(qreal zoom)
{
    if (m_image.isNull())
        return;
    const qreal clamped = qBound<qreal>(0.02, zoom, 16.0);
    if (qFuzzyCompare(clamped, m_zoom))
        return;

    // Keep the widget centre anchored.
    const QPointF center(width() / 2.0, height() / 2.0);
    const QPointF imgCenter = center + m_offset;
    const qreal ratio = clamped / m_zoom;
    m_offset = (imgCenter - center) * ratio - (imgCenter - center) + m_offset;
    m_zoom = clamped;
    m_fit = false;
    emit zoomChanged(m_zoom);
    update();
}

QRectF ImageView::imageRect() const
{
    if (m_image.isNull())
        return QRectF();
    const QSizeF s(m_image.width() * m_zoom, m_image.height() * m_zoom);
    return QRectF(QPointF((width() - s.width()) / 2.0 + m_offset.x(),
                          (height() - s.height()) / 2.0 + m_offset.y()),
                  s);
}

void ImageView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_fit)
        fitToWindow();
}

void ImageView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Theme::Bg0);

    if (m_image.isNull()) {
        p.setPen(Theme::TextFaint);
        p.setFont(Theme::ui(12));
        p.drawText(rect(), Qt::AlignCenter, "No image");
        return;
    }

    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const QRectF target = imageRect();

    // Checkerboard behind the image so transparent PNGs read correctly.
    const int cell = 12;
    const int x0 = int(target.left());
    const int y0 = int(target.top());
    for (int y = y0; y < int(target.bottom()); y += cell) {
        for (int x = x0; x < int(target.right()); x += cell) {
            const bool even = ((x - x0) / cell + (y - y0) / cell) % 2 == 0;
            p.fillRect(QRect(x, y, cell, cell), even ? QColor(0x16, 0x18, 0x1C) : QColor(0x1A, 0x1D, 0x22));
        }
    }

    p.drawImage(target, m_image);
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    if (m_image.isNull()) {
        QWidget::wheelEvent(event);
        return;
    }

    const qreal steps = event->angleDelta().y() / 120.0;
    if (qFuzzyIsNull(steps)) {
        QWidget::wheelEvent(event);
        return;
    }

    const QPointF cursor(event->pos());
    const QRectF before = imageRect();
    const QPointF rel = (cursor - before.topLeft()) / before.width();

    setZoom(m_zoom * qPow(1.18, steps));

    const QRectF after = imageRect();
    m_offset += cursor - (after.topLeft() + QPointF(rel.x() * after.width(), rel.y() * after.height()));
    update();
    event->accept();
}

void ImageView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_panning = true;
        m_panStart = event->pos();
        m_offsetStart = m_offset;
        setCursor(Qt::ClosedHandCursor);
    }
    QWidget::mousePressEvent(event);
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_panning) {
        m_offset = m_offsetStart + (event->pos() - m_panStart);
        m_fit = false;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void ImageView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_panning) {
        m_panning = false;
        unsetCursor();
    }
    QWidget::mouseReleaseEvent(event);
}

void ImageView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_offset = QPointF();
        fitToWindow();
    }
    QWidget::mouseDoubleClickEvent(event);
}

// --------------------------------------------------------- ImageViewerOverlay

ImageViewerOverlay::ImageViewerOverlay(QWidget *parent)
    : QWidget(parent)
    , m_index(-1)
    , m_view(nullptr)
    , m_title(nullptr)
    , m_counter(nullptr)
    , m_prev(nullptr)
    , m_next(nullptr)
    , m_zoomLabel(nullptr)
{
    setVisible(false);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_StyledBackground, false);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 14, 20, 16);
    layout->setSpacing(10);

    // Header
    QHBoxLayout *header = new QHBoxLayout();
    header->setSpacing(10);

    m_title = new QLabel(this);
    m_title->setFont(Theme::ui(13, QFont::DemiBold));
    m_title->setStyleSheet(QString("color: %1;").arg(Theme::Text.name()));
    header->addWidget(m_title);
    header->addStretch(1);

    m_counter = new QLabel(this);
    m_counter->setFont(Theme::mono(11));
    m_counter->setStyleSheet(QString("color: %1;").arg(Theme::TextDim.name()));
    header->addWidget(m_counter);
    header->addSpacing(8);

    QToolButton *close = Widgets::iconButton(Icons::Close, "Close (Esc)", this, 18);
    connect(close, &QToolButton::clicked, this, &ImageViewerOverlay::closeOverlay);
    header->addWidget(close);

    layout->addLayout(header);

    // Image
    m_view = new ImageView(this);
    m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(m_view, 1);

    connect(m_view, &ImageView::zoomChanged, this, [this](qreal z) {
        if (m_zoomLabel)
            m_zoomLabel->setText(QString("%1%").arg(int(z * 100)));
    });

    // Footer
    QHBoxLayout *footer = new QHBoxLayout();
    footer->setSpacing(8);

    m_prev = new QPushButton("Previous", this);
    m_prev->setObjectName("Ghost");
    m_prev->setCursor(Qt::PointingHandCursor);
    connect(m_prev, &QPushButton::clicked, this, [this]() { showIndex(m_index - 1); });
    footer->addWidget(m_prev);

    m_next = new QPushButton("Next", this);
    m_next->setObjectName("Ghost");
    m_next->setCursor(Qt::PointingHandCursor);
    connect(m_next, &QPushButton::clicked, this, [this]() { showIndex(m_index + 1); });
    footer->addWidget(m_next);

    footer->addStretch(1);

    QToolButton *zoomOut = Widgets::iconButton(Icons::ZoomOut, "Zoom out", this, 17);
    connect(zoomOut, &QToolButton::clicked, this, [this]() { m_view->setZoom(m_view->zoom() / 1.25); });
    footer->addWidget(zoomOut);

    m_zoomLabel = new QLabel("100%", this);
    m_zoomLabel->setFont(Theme::mono(11));
    m_zoomLabel->setFixedWidth(52);
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    m_zoomLabel->setStyleSheet(QString("color: %1;").arg(Theme::TextDim.name()));
    footer->addWidget(m_zoomLabel);

    QToolButton *zoomIn = Widgets::iconButton(Icons::ZoomIn, "Zoom in", this, 17);
    connect(zoomIn, &QToolButton::clicked, this, [this]() { m_view->setZoom(m_view->zoom() * 1.25); });
    footer->addWidget(zoomIn);

    QToolButton *fit = Widgets::iconButton(Icons::Fit, "Fit to window (0)", this, 17);
    connect(fit, &QToolButton::clicked, this, [this]() { m_view->fitToWindow(); });
    footer->addWidget(fit);

    footer->addSpacing(10);

    QPushButton *reveal = new QPushButton("Show in folder", this);
    reveal->setObjectName("Ghost");
    reveal->setCursor(Qt::PointingHandCursor);
    connect(reveal, &QPushButton::clicked, this, [this]() {
        if (!currentPath().isEmpty())
            emit revealRequested(currentPath());
    });
    footer->addWidget(reveal);

    QPushButton *discard = new QPushButton("Discard", this);
    discard->setObjectName("Danger");
    discard->setCursor(Qt::PointingHandCursor);
    connect(discard, &QPushButton::clicked, this, [this]() {
        const QString path = currentPath();
        if (path.isEmpty())
            return;
        emit discardRequested(path);
        if (m_paths.size() <= 1) {
            closeOverlay();
        } else {
            m_paths.removeAt(m_index);
            showIndex(qMin(m_index, m_paths.size() - 1));
        }
    });
    footer->addWidget(discard);

    layout->addLayout(footer);

    // Forward key events from children to the overlay.
    const QList<QWidget *> all = findChildren<QWidget *>();
    for (int i = 0; i < all.size(); ++i)
        all.at(i)->installEventFilter(this);
}

void ImageViewerOverlay::open(const QStringList &paths, int index)
{
    m_paths = paths;
    if (m_paths.isEmpty())
        return;
    setGeometry(parentWidget()->rect());
    raise();
    setVisible(true);
    setFocus(Qt::OtherFocusReason);
    showIndex(qBound(0, index, m_paths.size() - 1));
}

void ImageViewerOverlay::closeOverlay()
{
    setVisible(false);
    m_paths.clear();
    m_index = -1;
    m_view->setImage(QImage());
}

QString ImageViewerOverlay::currentPath() const
{
    if (m_index < 0 || m_index >= m_paths.size())
        return QString();
    return m_paths.at(m_index);
}

void ImageViewerOverlay::showIndex(int index)
{
    if (m_paths.isEmpty())
        return;
    m_index = qBound(0, index, m_paths.size() - 1);
    m_view->setImagePath(m_paths.at(m_index));
    updateChrome();
}

void ImageViewerOverlay::updateChrome()
{
    if (m_paths.isEmpty())
        return;
    m_title->setText(QFileInfo(m_paths.at(m_index)).fileName());
    m_counter->setText(QString("%1 / %2").arg(m_index + 1).arg(m_paths.size()));
    m_prev->setEnabled(m_index > 0);
    m_next->setEnabled(m_index < m_paths.size() - 1);
    if (m_zoomLabel)
        m_zoomLabel->setText(QString("%1%").arg(int(m_view->zoom() * 100)));
}

void ImageViewerOverlay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void ImageViewerOverlay::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(8, 9, 12));
}

void ImageViewerOverlay::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        closeOverlay();
        event->accept();
        return;
    case Qt::Key_Left:
        showIndex(m_index - 1);
        event->accept();
        return;
    case Qt::Key_Right:
        showIndex(m_index + 1);
        event->accept();
        return;
    case Qt::Key_0:
        m_view->fitToWindow();
        event->accept();
        return;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        m_view->setZoom(m_view->zoom() * 1.25);
        event->accept();
        return;
    case Qt::Key_Minus:
        m_view->setZoom(m_view->zoom() / 1.25);
        event->accept();
        return;
    case Qt::Key_Delete: {
        const QString path = currentPath();
        if (!path.isEmpty()) {
            emit discardRequested(path);
            if (m_paths.size() <= 1) {
                closeOverlay();
            } else {
                m_paths.removeAt(m_index);
                showIndex(qMin(m_index, m_paths.size() - 1));
            }
        }
        event->accept();
        return;
    }
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

bool ImageViewerOverlay::eventFilter(QObject *watched, QEvent *event)
{
    if (isVisible() && event->type() == QEvent::KeyPress) {
        QKeyEvent *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Escape || key->key() == Qt::Key_Left
            || key->key() == Qt::Key_Right || key->key() == Qt::Key_Delete) {
            keyPressEvent(key);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
