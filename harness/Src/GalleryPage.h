#ifndef GALLERYPAGE_H
#define GALLERYPAGE_H

#include <QStringList>
#include <QWidget>

class QLabel;
class QListWidget;
class QFileSystemWatcher;
class QStackedWidget;
class QTimer;

/*! Plain image browser for the output directory. No metadata, just pictures. */
class GalleryPage : public QWidget
{
    Q_OBJECT

public:
    explicit GalleryPage(QWidget *parent = nullptr);

    void refresh();
    void removePath(const QString &path);

signals:
    void viewRequested(const QStringList &paths, int index);
    void revealRequested(const QString &path);
    void discardRequested(const QString &path);

private slots:
    void onDirectoryChanged();
    void openCurrent();

private:
    void rebuild();
    void openItem(int row);
    void showContextMenu(const QPoint &pos);

    QStackedWidget *m_stack;
    QListWidget *m_list;
    QLabel *m_count;
    QFileSystemWatcher *m_watcher;
    QTimer *m_debounce;
    QStringList m_paths;
};

#endif // GALLERYPAGE_H
