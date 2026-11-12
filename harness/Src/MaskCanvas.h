#ifndef MASKCANVAS_H
#define MASKCANVAS_H

#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QString>
#include <QVector>
#include <QWidget>

/*!
 * Zoomable painting surface for the mask workflow.
 *
 * The primary image is shown underneath a translucent mask overlay. White
 * strokes mean "regenerate here"; the mask is exported as a grayscale PNG where
 * white marks the area to infill.
 */
class MaskCanvas : public QWidget
{
    Q_OBJECT

public:
    enum Tool { Brush, Eraser };

    explicit MaskCanvas(QWidget *parent = nullptr);

    bool loadBase(const QString &path, QString *error = nullptr);
    void clearBase();
    bool hasBase() const { return !m_base.isNull(); }
    QString basePath() const { return m_basePath; }
    QSize baseSize() const { return m_base.size(); }

    void setTool(Tool tool);
    Tool tool() const { return m_tool; }

    void setBrushSize(int pixels);
    int brushSize() const { return m_brush; }

    bool hasMask() const { return m_hasContent; }
    QImage maskImage() const { return m_mask; }

    void clearMask();
    void invertMask();
    bool undo();
    bool redo();
    bool canUndo() const { return !m_undo.isEmpty(); }
    bool canRedo() const { return !m_redo.isEmpty(); }
    void resetHistory();

    bool loadMaskFile(const QString &path, QString *error = nullptr);
    bool saveMaskFile(const QString &path, QString *error = nullptr);

    void fitToView();
    void zoomBy(qreal factor);

signals:
    void maskChanged();
    void historyChanged();
    void baseDropped(const QString &path);
    void baseChanged();
    void browseRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QRectF baseRect() const;
    QPointF toImage(const QPointF &widgetPos) const;
    void paintDot(const QPointF &imagePos);
    void paintLine(const QPointF &from, const QPointF &to);
    void pushUndo();
    void rebuildTint();
    void updateCursorShape();
    bool computeHasContent() const;

    QImage m_base;
    QImage m_mask;
    QImage m_tint;
    QString m_basePath;

    qreal m_zoom;
    QPointF m_offset;
    bool m_fit;

    Tool m_tool;
    int m_brush;
    bool m_hasContent;

    bool m_painting;
    QPointF m_lastImagePos;

    bool m_panning;
    QPoint m_panStart;
    QPointF m_offsetStart;

    bool m_hover;
    QPointF m_cursorPos;
    bool m_dragOver;

    bool m_tintDirty;
    QVector<QImage> m_undo;
    QVector<QImage> m_redo;
};

#endif // MASKCANVAS_H
