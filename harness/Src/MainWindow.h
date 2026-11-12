#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Backend.h"
#include "Widgets.h"

#include <QMainWindow>
#include <QVector>

class CreatePage;
class GalleryPage;
class ImageViewerOverlay;
class SettingsPage;
class TerminalPage;
class QStackedWidget;
class Sidebar;
class StatusStrip;
class TitleBar;
class Toast;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString pageIdForIndex(int index) const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void goToPage(const QString &pageId);
    void toggleServer();
    void onBackendState(int state);
    void onJobRequested(const GenerationRequest &request, const QString &pageId);
    void onProgress(const QString &jobId, int percent, const QString &stage);
    void onJobFinished(const GenerationResult &result);
    void onJobFailed(const QString &jobId, const QString &message);
    void onViewRequested(const QStringList &paths, int index);
    void onRevealRequested(const QString &path);
    void onDiscardRequested(const QString &path);
    void onSettingsSaved();
    void onOutputCleared();

private:
    void buildUi();
    void buildPages();
    void buildShortcuts();
    void buildGrips();
    void layoutGrips();
    void updateMaximizedState();
    void updateBackendState();
    CreatePage *pageForId(const QString &pageId) const;
    void openFolder(const QString &path);
    void showToast(const QString &text, int kind = 0);

    TitleBar *m_titleBar;
    Sidebar *m_sidebar;
    StatusStrip *m_status;
    Toast *m_toast;
    ImageViewerOverlay *m_viewer;
    QStackedWidget *m_pages;
    QVector<QWidget *> m_grips;
    QVector<QString> m_pageIds;
    QVector<CreatePage *> m_createPages;

    GalleryPage *m_gallery;
    TerminalPage *m_terminal;
    SettingsPage *m_settings;

    Backend *m_backend;
    QString m_activeJobId;
    QString m_jobPageId;
    bool m_resizing;
    int m_resizeEdge;
    QPoint m_resizeStartPos;
    QRect m_resizeStartGeom;
};

#endif // MAINWINDOW_H
