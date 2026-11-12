#ifndef OUTPUTPANE_H
#define OUTPUTPANE_H

#include "Backend.h"

#include <QImage>
#include <QStringList>
#include <QVector>
#include <QWidget>

class Card;
class ElidedLabel;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QStackedWidget;
class ProgressBar;

/*! Result viewer: preview, batch thumbnails, discard and reveal. */
class OutputPane : public QWidget
{
    Q_OBJECT

public:
    explicit OutputPane(QWidget *parent = nullptr);

    void setBusy(bool busy);
    void setProgress(int percent, const QString &stage);
    void showResult(const GenerationResult &result);
    void removePath(const QString &path);
    void clear();

    QStringList currentPaths() const { return m_paths; }

signals:
    void discardRequested(const QString &path);
    void viewRequested(const QStringList &paths, int index);
    void revealRequested(const QString &path);
    void galleryRequested();
    void cancelRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void refreshPreview();
    void rebuildThumbs();
    void selectIndex(int index);
    void updateMeta();
    void showEmpty();

    Card *m_card;
    QStackedWidget *m_stack;
    QWidget *m_emptyPage;
    QWidget *m_busyPage;
    QWidget *m_resultPage;
    QLabel *m_preview;
    QImage m_image;
    QStringList m_paths;
    int m_index;
    bool m_hasResult;

    QWidget *m_thumbBar;
    QHBoxLayout *m_thumbLayout;
    QVector<QPushButton *> m_thumbButtons;
    ElidedLabel *m_meta;
    ElidedLabel *m_saved;
    QLabel *m_busyTitle;
    ProgressBar *m_bar;
    QLabel *m_percent;
    QLabel *m_stage;
};

#endif // OUTPUTPANE_H
