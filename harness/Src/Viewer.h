#ifndef VIEWER_H
#define VIEWER_H

#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QStringList>
#include <QWidget>

class QLabel;
class QPushButton;

/*! Pan / zoom canvas for a single image with a checkerboard backdrop. */
class ImageView : public QWidget
{
    Q_OBJECT

public:
    explicit ImageView(QWidget *parent = nullptr);

    void setImage(const QImage &image);
    void setImagePath(const QString &path);
    void fitToWindow();
    void setZoom(qreal zoom);
    qreal zoom() const { return m_zoom; }

signals:
    void zoomChanged(qreal zoom);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QRectF imageRect() const;

    QImage m_image;
    QString m_path;
    qreal m_zoom;
    QPointF m_offset;
    bool m_fit;
    bool m_panning;
    QPoint m_panStart;
    QPointF m_offsetStart;
};

/*! Full-window lightbox used by the gallery and the output pane. */
class ImageViewerOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit ImageViewerOverlay(QWidget *parent);

    void open(const QStringList &paths, int index);
    void closeOverlay();
    bool isOpen() const { return isVisible(); }
    QString currentPath() const;

signals:
    void discardRequested(const QString &path);
    void revealRequested(const QString &path);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void showIndex(int index);
    void updateChrome();

    QStringList m_paths;
    int m_index;
    ImageView *m_view;
    QLabel *m_title;
    QLabel *m_counter;
    QPushButton *m_prev;
    QPushButton *m_next;
    QLabel *m_zoomLabel;
};

#endif // VIEWER_H
