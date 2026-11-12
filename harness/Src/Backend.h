#ifndef BACKEND_H
#define BACKEND_H

#include <QMetaType>
#include <QObject>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector>

/*! The four workflows the UI exposes. */
enum class PageMode {
    Generate = 0,
    Edit,
    FaceSwap,
    MaskInfill
};

QString pageModeId(PageMode mode);
QString pageModeName(PageMode mode);
PageMode pageModeFromId(const QString &id);

/*!
 * Everything a backend needs to run one job.
 *
 * `images.first()` is the primary image (the bold first slot in the UI);
 * the remaining entries are references. The mask is deliberately separate
 * and never part of the image list.
 */
struct GenerationRequest {
    PageMode mode;
    QString prompt;
    QString negativePrompt;
    QSize size;
    int steps;
    double cfg;
    int batch;
    QString sampler;
    qint64 seed;          //!< -1 means "pick one"
    QStringList images;
    QString maskPath;     //!< only for MaskInfill

    GenerationRequest()
        : mode(PageMode::Generate)
        , size(1024, 1024)
        , steps(30)
        , cfg(4.0)
        , batch(1)
        , sampler("Euler")
        , seed(-1)
    {
    }

    bool hasMask() const { return !maskPath.isEmpty(); }
    bool hasPrimary() const { return !images.isEmpty(); }
};

struct GeneratedImage {
    QString path;
};

struct GenerationResult {
    QString jobId;
    PageMode mode;
    QVector<GeneratedImage> images;
    QString error;
    bool cancelled;

    GenerationResult()
        : mode(PageMode::Generate)
        , cancelled(false)
    {
    }
};

/*!
 * Abstract transport to Qwen-Image-2.1. The UI only ever talks to this class.
 *
 * To connect a real backend, implement these four virtuals and return your
 * instance from createBackend() in BackendFactory.cpp. See docs/BACKEND.md.
 */
class Backend : public QObject
{
    Q_OBJECT

public:
    enum State {
        Stopped = 0,
        Starting,
        Running,
        Stopping,
        Failed
    };

    explicit Backend(QObject *parent = nullptr);
    ~Backend();

    State state() const { return m_state; }

    virtual void startServer() = 0;
    virtual void stopServer() = 0;

    //! Returns a job id, or an empty string when the job could not be queued.
    virtual QString submit(const GenerationRequest &request) = 0;
    virtual void cancel(const QString &jobId) = 0;

    /*!
     * Removes a generated file from disk. The default implementation deletes
     * the image and its .json sidecar; override if your backend keeps a library
     * index that must be updated too.
     */
    virtual void discardImage(const QString &imagePath);

signals:
    void stateChanged(int state);
    void progress(const QString &jobId, int percent, const QString &stage);
    void jobFinished(const GenerationResult &result);
    void jobFailed(const QString &jobId, const QString &message);

protected:
    void setState(State state);

private:
    State m_state;
};

Q_DECLARE_METATYPE(GenerationRequest)
Q_DECLARE_METATYPE(GenerationResult)

#endif // BACKEND_H
