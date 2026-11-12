#include "ImageSlots.h"

#include "Icons.h"
#include "Theme.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QImageReader>
#include <QLinearGradient>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QUrl>
#include <QVBoxLayout>

namespace {

const int RefCell = 60;
const int RefColumns = 5;
const int RefSpacing = 8;
const char *SlotMime = "application/x-qwen-slot";

QString imageFilter()
{
    return QString("Images (*.png *.jpg *.jpeg *.webp *.bmp *.gif *.tif *.tiff)");
}

QStringList imageFilesFrom(const QList<QUrl> &urls)
{
    QStringList files;
    const QStringList extensions = QStringList() << "png" << "jpg" << "jpeg" << "webp"
                                                 << "bmp" << "gif" << "tif" << "tiff";
    for (int i = 0; i < urls.size(); ++i) {
        if (!urls.at(i).isLocalFile())
            continue;
        const QString path = urls.at(i).toLocalFile();
        if (extensions.contains(QFileInfo(path).suffix().toLower()))
            files.append(path);
    }
    return files;
}

} // namespace

// ------------------------------------------------------------------ ImageSlot

ImageSlot::ImageSlot(int index, bool primary, QWidget *parent)
    : QWidget(parent)
    , m_index(index)
    , m_primary(primary)
    , m_primaryLabel("Primary image")
    , m_primaryHint("Click or drop an image")
    , m_hover(false)
    , m_dragOver(false)
    , m_pressed(false)
{
    setAcceptDrops(true);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);

    if (primary) {
        setFixedHeight(148);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    } else {
        setFixedSize(RefCell, RefCell);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
}

void ImageSlot::setPrimaryLabel(const QString &label, const QString &hint)
{
    m_primaryLabel = label;
    m_primaryHint = hint;
    update();
}

void ImageSlot::setPath(const QString &path)
{
    if (m_path == path)
        return;
    m_path = path;
    m_thumb = QPixmap();
    rebuildThumb();
    update();
}

void ImageSlot::rebuildThumb()
{
    if (m_path.isEmpty() || width() <= 0 || height() <= 0)
        return;

    const int dpr = qMax(1, devicePixelRatio());
    const QSize target(qMax(8, width() * dpr), qMax(8, height() * dpr));

    QImageReader reader(m_path);
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    reader.setAutoTransform(true);
#endif
    QImage img = reader.read();
    if (img.isNull())
        return;

    img = img.scaled(target, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const int x = qMax(0, (img.width() - target.width()) / 2);
    const int y = qMax(0, (img.height() - target.height()) / 2);
    m_thumb = QPixmap::fromImage(img.copy(x, y, target.width(), target.height()));
}

void ImageSlot::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (!m_path.isEmpty())
        rebuildThumb();
}

void ImageSlot::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const qreal radius = m_primary ? 10.0 : 8.0;
    const QRectF outer(0.5, 0.5, width() - 1.0, height() - 1.0);
    QPainterPath shape;
    shape.addRoundedRect(outer, radius, radius);

    const QColor accent = Theme::accent();

    if (m_path.isEmpty()) {
        p.fillPath(shape, QColor(0x12, 0x14, 0x19));

        QPen pen;
        pen.setWidthF(m_primary || m_dragOver ? 1.4 : 1.0);
        if (m_dragOver)
            pen.setColor(accent);
        else if (m_hover && m_primary)
            pen.setColor(accent.darker(120));
        else if (m_primary)
            pen.setColor(QColor(0x2E, 0x34, 0x42));
        else
            pen.setColor(QColor(0x22, 0x26, 0x2F));

        if (m_primary) {
            pen.setStyle(Qt::DashLine);
            pen.setDashPattern(QVector<qreal>() << 5 << 4);
        } else {
            pen.setStyle(Qt::SolidLine);
        }
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawPath(shape);

        if (m_primary) {
            const QPixmap plus = Icons::pixmap(Icons::Plus, 22,
                                               m_hover || m_dragOver ? Theme::TextDim : Theme::TextFaint);
            p.drawPixmap((width() - 22) / 2, height() / 2 - 30, plus);

            QFont titleFont = Theme::ui(12, QFont::DemiBold);
            p.setFont(titleFont);
            p.setPen(m_hover ? Theme::Text : Theme::TextDim);
            p.drawText(QRect(8, height() / 2 - 2, width() - 16, 20), Qt::AlignCenter, m_primaryLabel);

            p.setFont(Theme::ui(10));
            p.setPen(Theme::TextFaint);
            p.drawText(QRect(8, height() / 2 + 18, width() - 16, 18), Qt::AlignCenter, m_primaryHint);
        } else {
            const QPixmap plus = Icons::pixmap(Icons::Plus, 15,
                                               m_hover || m_dragOver ? Theme::TextDim : QColor(0x3A, 0x41, 0x50));
            p.drawPixmap((width() - 15) / 2, (height() - 15) / 2, plus);
        }
        return;
    }

    p.save();
    p.setClipPath(shape);
    if (!m_thumb.isNull())
        p.drawPixmap(rect(), m_thumb);

    QLinearGradient shade(0, height() * 0.55, 0, height());
    shade.setColorAt(0.0, QColor(0, 0, 0, 0));
    shade.setColorAt(1.0, QColor(0, 0, 0, 120));
    p.fillPath(shape, shade);
    p.restore();

    QColor border = QColor(0x28, 0x2D, 0x38);
    qreal borderWidth = 1.0;
    if (m_primary) {
        border = accent;
        borderWidth = 1.6;
    } else if (m_dragOver) {
        border = accent;
        borderWidth = 1.6;
    } else if (m_hover) {
        border = QColor(0x46, 0x4E, 0x62);
    }

    p.setPen(QPen(border, borderWidth));
    p.setBrush(Qt::NoBrush);
    p.drawPath(shape);

    if (m_primary) {
        QFont chipFont = Theme::ui(8, QFont::Bold);
        QFontMetrics fm(chipFont);
        const int tw = fm.width("PRIMARY") + 12;
        const QRect chip(8, 8, tw, 16);
        p.setPen(Qt::NoPen);
        p.setBrush(accent);
        p.drawRoundedRect(chip, 4, 4);
        p.setPen(QColor(0xFF, 0xFF, 0xFF));
        p.setFont(chipFont);
        p.drawText(chip, Qt::AlignCenter, "PRIMARY");
    }

    if (m_hover) {
        const QRect closeRect(width() - 26, 6, 20, 20);
        p.setPen(QPen(QColor(255, 255, 255, 40), 1.0));
        p.setBrush(QColor(10, 11, 14, 200));
        p.drawEllipse(closeRect);
        p.drawPixmap(closeRect.adjusted(5, 5, -5, -5), Icons::pixmap(Icons::Close, 10, QColor(255, 255, 255)));
    }
}

void ImageSlot::enterEvent(QEvent *event)
{
    m_hover = true;
    update();
    QWidget::enterEvent(event);
}

void ImageSlot::leaveEvent(QEvent *event)
{
    m_hover = false;
    m_dragOver = false;
    update();
    QWidget::leaveEvent(event);
}

void ImageSlot::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (!m_path.isEmpty()) {
        const QRect closeRect(width() - 26, 6, 20, 20);
        if (closeRect.contains(event->pos())) {
            emit removeRequested(m_index);
            return;
        }
    }

    m_pressed = true;
    m_pressPos = event->pos();
}

void ImageSlot::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_pressed || m_path.isEmpty())
        return;
    if ((event->pos() - m_pressPos).manhattanLength() < QApplication::startDragDistance())
        return;

    m_pressed = false;

    QDrag *drag = new QDrag(this);
    QMimeData *mime = new QMimeData();
    mime->setUrls(QList<QUrl>() << QUrl::fromLocalFile(m_path));
    mime->setData(SlotMime, QByteArray::number(m_index));
    drag->setMimeData(mime);

    if (!m_thumb.isNull()) {
        const QPixmap preview = m_thumb.scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        drag->setPixmap(preview);
        drag->setHotSpot(QPoint(preview.width() / 2, preview.height() / 2));
    }
    drag->exec(Qt::CopyAction | Qt::MoveAction);
}

void ImageSlot::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_pressed) {
        m_pressed = false;
        if (rect().contains(event->pos())) {
            if (m_path.isEmpty())
                emit browseRequested(m_index);
            else
                emit previewRequested(m_index, m_path);
        }
    }
    QWidget::mouseReleaseEvent(event);
}

void ImageSlot::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit browseRequested(m_index);
    QWidget::mouseDoubleClickEvent(event);
}

void ImageSlot::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    if (!m_path.isEmpty()) {
        QAction *preview = menu.addAction("Preview");
        QAction *replace = menu.addAction("Replace...");
        QAction *makePrimary = m_primary ? nullptr : menu.addAction("Make primary");
        menu.addSeparator();
        QAction *remove = menu.addAction("Remove");
        QAction *reveal = menu.addAction("Show in folder");

        QAction *chosen = menu.exec(event->globalPos());
        if (chosen == preview)
            emit previewRequested(m_index, m_path);
        else if (chosen == replace)
            emit browseRequested(m_index);
        else if (chosen == remove)
            emit removeRequested(m_index);
        else if (chosen == reveal)
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_path).absolutePath()));
        else if (makePrimary && chosen == makePrimary)
            emit makePrimaryRequested(m_index);
    } else {
        QAction *add = menu.addAction("Add image...");
        if (menu.exec(event->globalPos()) == add)
            emit browseRequested(m_index);
    }
}

bool ImageSlot::acceptsFiles(const QMimeData *mime) const
{
    if (!mime)
        return false;
    if (mime->hasFormat(SlotMime))
        return true;
    return !imageFilesFrom(mime->urls()).isEmpty();
}

void ImageSlot::dragEnterEvent(QDragEnterEvent *event)
{
    if (acceptsFiles(event->mimeData())) {
        m_dragOver = true;
        update();
        event->acceptProposedAction();
    }
}

void ImageSlot::dragLeaveEvent(QDragLeaveEvent *event)
{
    m_dragOver = false;
    update();
    QWidget::dragLeaveEvent(event);
}

void ImageSlot::dropEvent(QDropEvent *event)
{
    m_dragOver = false;
    update();

    if (event->mimeData()->hasFormat(SlotMime)) {
        const int from = event->mimeData()->data(SlotMime).toInt();
        if (from != m_index)
            emit moveRequested(from, m_index);
        event->acceptProposedAction();
        return;
    }

    const QStringList files = imageFilesFrom(event->mimeData()->urls());
    if (!files.isEmpty()) {
        emit filesDropped(m_index, files);
        event->acceptProposedAction();
    }
}

// ----------------------------------------------------------------- ImageSlots

ImageSlots::ImageSlots(QWidget *parent)
    : QWidget(parent)
    , m_showPrimary(true)
    , m_primary(nullptr)
    , m_layout(nullptr)
    , m_grid(nullptr)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(10);

    m_primary = new ImageSlot(0, true, this);
    m_layout->addWidget(m_primary);

    connect(m_primary, &ImageSlot::browseRequested, this, &ImageSlots::onBrowseRequested);
    connect(m_primary, &ImageSlot::removeRequested, this, &ImageSlots::onRemoveRequested);
    connect(m_primary, &ImageSlot::filesDropped, this, &ImageSlots::onFilesDropped);
    connect(m_primary, &ImageSlot::previewRequested, this, &ImageSlots::onPreviewRequested);

    m_grid = new QGridLayout();
    m_grid->setContentsMargins(0, 0, 0, 0);
    m_grid->setHorizontalSpacing(RefSpacing);
    m_grid->setVerticalSpacing(RefSpacing);
    m_grid->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_layout->addLayout(m_grid);

    for (int i = 0; i < 15; ++i) {
        ImageSlot *slot = new ImageSlot(i + 1, false, this);
        m_refs.append(slot);
        m_grid->addWidget(slot, i / RefColumns, i % RefColumns);

        connect(slot, &ImageSlot::browseRequested, this, &ImageSlots::onBrowseRequested);
        connect(slot, &ImageSlot::removeRequested, this, &ImageSlots::onRemoveRequested);
        connect(slot, &ImageSlot::filesDropped, this, &ImageSlots::onFilesDropped);
        connect(slot, &ImageSlot::moveRequested, this, &ImageSlots::onMoveRequested);
        connect(slot, &ImageSlot::makePrimaryRequested, this, &ImageSlots::onMakePrimaryRequested);
        connect(slot, &ImageSlot::previewRequested, this, &ImageSlots::onPreviewRequested);
    }

    m_layout->addStretch(1);
}

void ImageSlots::setShowPrimary(bool on)
{
    m_showPrimary = on;
    m_primary->setVisible(on);
    emit changed();
}

void ImageSlots::setPrimaryLabel(const QString &label, const QString &hint)
{
    m_primary->setPrimaryLabel(label, hint);
}

QStringList ImageSlots::refPaths() const
{
    QStringList out;
    for (int i = 0; i < m_refs.size(); ++i) {
        if (!m_refs.at(i)->path().isEmpty())
            out.append(m_refs.at(i)->path());
    }
    return out;
}

QStringList ImageSlots::paths() const
{
    QStringList out;
    if (m_showPrimary && !m_primary->path().isEmpty())
        out.append(m_primary->path());
    out.append(refPaths());
    return out;
}

QString ImageSlots::primaryPath() const
{
    return m_primary->path();
}

int ImageSlots::count() const
{
    return paths().size();
}

void ImageSlots::setPaths(const QStringList &paths)
{
    clear();
    int slot = 0;
    for (int i = 0; i < paths.size(); ++i) {
        if (m_showPrimary && slot == 0) {
            m_primary->setPath(paths.at(i));
            ++slot;
            continue;
        }
        const int refIndex = m_showPrimary ? slot - 1 : slot;
        if (refIndex >= 0 && refIndex < m_refs.size())
            m_refs.at(refIndex)->setPath(paths.at(i));
        ++slot;
    }
    emit changed();
}

void ImageSlots::setPrimaryPath(const QString &path)
{
    m_primary->setPath(path);
    emit changed();
}

void ImageSlots::addPaths(const QStringList &paths)
{
    if (paths.isEmpty())
        return;

    int next = 0;
    if (m_showPrimary && m_primary->path().isEmpty()) {
        m_primary->setPath(paths.at(0));
        next = 1;
    } else if (m_showPrimary) {
        next = 0;
    }

    int done = next;
    for (int i = next; i < paths.size(); ++i) {
        bool placed = false;
        for (int r = 0; r < m_refs.size(); ++r) {
            if (m_refs.at(r)->path().isEmpty()) {
                m_refs.at(r)->setPath(paths.at(i));
                placed = true;
                break;
            }
        }
        if (!placed)
            break;
        ++done;
    }

    if (done > 0)
        emit changed();
}

void ImageSlots::clear()
{
    m_primary->setPath(QString());
    for (int i = 0; i < m_refs.size(); ++i)
        m_refs.at(i)->setPath(QString());
    emit changed();
}

void ImageSlots::onBrowseRequested(int index)
{
    browseInto(index);
}

void ImageSlots::browseInto(int slotIndex)
{
    static QString lastDir;
    const QStringList files = QFileDialog::getOpenFileNames(this, "Choose images", lastDir, imageFilter());
    if (files.isEmpty())
        return;
    lastDir = QFileInfo(files.first()).absolutePath();

    ImageSlot *target = nullptr;
    if (slotIndex == 0 && m_showPrimary)
        target = m_primary;
    else if (slotIndex >= 1 && slotIndex <= m_refs.size())
        target = m_refs.at(slotIndex - 1);

    if (target && !target->path().isEmpty()) {
        target->setPath(files.first());
        if (files.size() > 1)
            addPaths(files.mid(1));
        emit changed();
        return;
    }

    addPaths(files);
}

void ImageSlots::onRemoveRequested(int index)
{
    if (index == 0 && m_showPrimary) {
        m_primary->setPath(QString());
    } else if (index >= 1 && index <= m_refs.size()) {
        // Close the gap so the reference list stays dense.
        for (int i = index - 1; i < m_refs.size() - 1; ++i)
            m_refs.at(i)->setPath(m_refs.at(i + 1)->path());
        m_refs.last()->setPath(QString());
    }
    emit changed();
}

void ImageSlots::onFilesDropped(int index, const QStringList &paths)
{
    if (paths.isEmpty())
        return;

    if (index == 0 && m_showPrimary) {
        m_primary->setPath(paths.first());
        if (paths.size() > 1)
            addPaths(paths.mid(1));
    } else if (index >= 1 && index <= m_refs.size()) {
        m_refs.at(index - 1)->setPath(paths.first());
        if (paths.size() > 1)
            addPaths(paths.mid(1));
    }
    emit changed();
}

void ImageSlots::onMoveRequested(int from, int to)
{
    QList<ImageSlot *> all;
    all.append(m_primary);
    for (int i = 0; i < m_refs.size(); ++i)
        all.append(m_refs.at(i));

    if (from < 0 || from >= all.size() || to < 0 || to >= all.size())
        return;

    ImageSlot *a = all.at(from);
    ImageSlot *b = all.at(to);
    const QString pa = a->path();
    const QString pb = b->path();
    a->setPath(pb);
    b->setPath(pa);
    emit changed();
}

void ImageSlots::onMakePrimaryRequested(int index)
{
    if (index <= 0 || index > m_refs.size())
        return;

    const QString ref = m_refs.at(index - 1)->path();
    const QString primary = m_primary->path();
    m_primary->setPath(ref);
    m_refs.at(index - 1)->setPath(primary);
    emit changed();
}

void ImageSlots::onPreviewRequested(int index, const QString &path)
{
    Q_UNUSED(index)
    emit viewRequested(path);
}
