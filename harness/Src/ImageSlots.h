#ifndef IMAGESLOTS_H
#define IMAGESLOTS_H

#include <QPoint>
#include <QStringList>
#include <QWidget>

class QGridLayout;
class QMimeData;
class QVBoxLayout;

/*! One tile in the input grid: drop target, thumbnail and hover actions. */
class ImageSlot : public QWidget
{
    Q_OBJECT

public:
    explicit ImageSlot(int index, bool primary, QWidget *parent = nullptr);

    void setPath(const QString &path);
    QString path() const { return m_path; }
    bool isPrimary() const { return m_primary; }
    int index() const { return m_index; }

    void setPrimaryLabel(const QString &label, const QString &hint);

signals:
    void browseRequested(int index);
    void previewRequested(int index, const QString &path);
    void removeRequested(int index);
    void filesDropped(int index, const QStringList &paths);
    void moveRequested(int from, int to);
    void makePrimaryRequested(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void rebuildThumb();
    bool acceptsFiles(const QMimeData *mime) const;

    int m_index;
    bool m_primary;
    QString m_path;
    QPixmap m_thumb;
    QString m_primaryLabel;
    QString m_primaryHint;
    bool m_hover;
    bool m_dragOver;
    QPoint m_pressPos;
    bool m_pressed;
};

/*!
 * The 1 + n image model: one bold primary tile plus up to fifteen reference
 * tiles. The primary can be hidden for the mask workflow, where the canvas
 * itself owns the primary image.
 */
class ImageSlots : public QWidget
{
    Q_OBJECT

public:
    explicit ImageSlots(QWidget *parent = nullptr);

    void setShowPrimary(bool on);
    void setPrimaryLabel(const QString &label, const QString &hint);

    QStringList paths() const;
    QStringList refPaths() const;
    QString primaryPath() const;
    int count() const;
    int capacity() const { return m_showPrimary ? 16 : 15; }

    void setPaths(const QStringList &paths);
    void setPrimaryPath(const QString &path);
    void addPaths(const QStringList &paths);
    void clear();

signals:
    void changed();
    void viewRequested(const QString &path);

private slots:
    void onBrowseRequested(int index);
    void onRemoveRequested(int index);
    void onFilesDropped(int index, const QStringList &paths);
    void onMoveRequested(int from, int to);
    void onMakePrimaryRequested(int index);
    void onPreviewRequested(int index, const QString &path);

private:
    void browseInto(int slotIndex);
    void rebuildGrid();

    bool m_showPrimary;
    ImageSlot *m_primary;
    QVector<ImageSlot *> m_refs;
    QVBoxLayout *m_layout;
    QGridLayout *m_grid;
};

#endif // IMAGESLOTS_H
