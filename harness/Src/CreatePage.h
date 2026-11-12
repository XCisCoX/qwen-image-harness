#ifndef CREATEPAGE_H
#define CREATEPAGE_H

#include "Backend.h"
#include "Settings.h"

#include <QSize>
#include <QWidget>

class Card;
class ImageSlots;
class MaskCanvas;
class OutputPane;
class ParamPanel;
class QLabel;
class QSlider;
class QToolButton;

/*! One workflow page: inputs, prompt/parameters and the output viewer. */
class CreatePage : public QWidget
{
    Q_OBJECT

public:
    explicit CreatePage(PageMode mode, QWidget *parent = nullptr);

    PageMode mode() const { return m_mode; }
    QString pageId() const;

    void setBackendReady(bool ready);
    void applyDefaults(const GenerationDefaults &defaults);
    void setBusy(bool busy);
    void setProgress(int percent, const QString &stage);
    void showResult(const GenerationResult &result);
    void removePath(const QString &path);
    void clearOutput();
    QStringList currentPaths() const;
    void focusPrompt();
    //! Result of the "Scale" dialog; invalid means "keep the source size".
    QSize scaleTarget() const { return m_scaleTarget; }

signals:
    void submitRequested(const GenerationRequest &request, const QString &pageId);
    void viewRequested(const QStringList &paths, int index);
    void revealRequested(const QString &path);
    void galleryRequested();
    void cancelRequested();
    void discardRequested(const QString &path);

private slots:
    void onSubmit();
    void updateCounts();
    void updateMaskButtons();
    void chooseBaseImage();
    void chooseMaskFile();
    void saveMaskFile();
    void promptScale();

private:
    QWidget *buildHeader();
    QWidget *buildInputColumn();
    QWidget *buildMaskColumn();
    QWidget *buildCenterColumn();
    QWidget *buildMaskToolbar(QWidget *parent);

    PageMode m_mode;
    ImageSlots *m_images;
    MaskCanvas *m_canvas;
    ParamPanel *m_params;
    OutputPane *m_output;
    QLabel *m_countBadge;
    QLabel *m_scaleLabel;
    QSize m_scaleTarget;
    QLabel *m_refsBadge;
    QSlider *m_brushSlider;
    QLabel *m_brushLabel;
    QToolButton *m_undoButton;
    QToolButton *m_redoButton;
    QToolButton *m_clearButton;
    QToolButton *m_invertButton;
    QToolButton *m_loadMaskButton;
    QToolButton *m_saveMaskButton;
    bool m_backendReady;
};

#endif // CREATEPAGE_H
