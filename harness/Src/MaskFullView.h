#ifndef MASKFULLVIEW_H
#define MASKFULLVIEW_H

#include <QDialog>
#include <QPointer>

class MaskCanvas;
class QLabel;
class QVBoxLayout;
class QWidget;

/*!
 * Full-window host for the mask editor.
 *
 * Qt cannot reparent a widget into a layout it does not own cleanly, so the canvas and
 * its toolbar are *moved* into this dialog and moved back on close. The owning page keeps
 * a placeholder in the original layout so the card does not collapse while the editor is
 * away, and the canvas keeps every bit of its state (mask, history, zoom).
 */
class MaskFullView : public QDialog
{
    Q_OBJECT

public:
    explicit MaskFullView(QWidget *parent = nullptr);

    //! Takes ownership of the canvas and toolbar for the duration of the full view.
    void showEditor(MaskCanvas *canvas, QWidget *toolbar, const QRect &geometry);
    //! Gives them back; returns false when nothing was hosted.
    bool releaseEditor();

    QWidget *hostedCanvas() const;
    bool isHosting() const { return m_canvas != nullptr; }

signals:
    void closing();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    MaskCanvas *m_canvas;
    QWidget *m_toolbar;
    QVBoxLayout *m_layout;
    QLabel *m_hint;
    QWidget *m_header;
};

#endif // MASKFULLVIEW_H
