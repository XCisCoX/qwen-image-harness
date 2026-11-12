#ifndef COMFYBACKEND_H
#define COMFYBACKEND_H

#include <QElapsedTimer>
#include <QImage>
#include <QRect>
#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QTimer>

#include "Backend.h"

class QJsonObject;
class QNetworkReply;

/*!
 * Real transport for Qwen-Image-2.1: spawns (or attaches to) ComfyUI, uploads the
 * primary image / references / mask, queues a graph built from the request, polls
 * the history endpoint and writes results into the configured output directory.
 *
 * Everything is synchronous HTTP against a loopback server (millisecond latency),
 * so a local event loop per call keeps the state machine readable without any
 * worker threads. Nothing sleeps.
 */
class ComfyBackend : public Backend
{
    Q_OBJECT

public:
    explicit ComfyBackend(QObject *parent = nullptr);
    ~ComfyBackend() override;

    void startServer() override;
    void stopServer() override;
    QString submit(const GenerationRequest &request) override;
    void cancel(const QString &jobId) override;

private slots:
    void onProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void pollServer();
    void pollJobs();

private:
    struct Job {
        QString id;
        QString promptId;
        GenerationRequest req;
        qint64 seed;
        int steps;
        QString modelFile;
        QString teFile;
        QStringList uploaded;   // file names inside ComfyUI's input directory
        QString maskName;
        // Mask infill works on a crop so the model sees the structure around the
        // hole; the crop is blended back locally when the result arrives.
        QString originalPath;   // full-size primary image
        QString cropImagePath;  // upscaled crop sent to ComfyUI
        QString cropMaskPath;   // mask cropped and scaled to the same frame
        QRect cropRect;         // crop position in the original
        QSize workSize;         // crop after rescaling (what the model sees)
        QImage featherMask;     // cropped mask at original scale, used for blending
        QElapsedTimer clock;
        bool cancelled;
        int percent;
        QString stage;

        Job() : seed(-1), steps(30), cancelled(false), percent(0) {}
    };

    // --- http helpers (local event loop, bounded by a timeout) ---------------
    QString baseUrl() const;
    bool httpGet(const QString &path, QJsonObject *json, QString *error, int timeoutMs = 15000);
    bool httpPostJson(const QString &path, const QJsonObject &body, QJsonObject *json,
                      QString *error, int timeoutMs = 30000);
    bool httpPostEmpty(const QString &path, QString *error);
    bool httpUpload(const QString &filePath, const QString &field, QString *storedName, QString *error);
    bool httpDownload(const QString &query, const QString &destPath, QString *error);

    // --- job pipeline --------------------------------------------------------
    void beginUploads(Job *job);
    bool prepareMaskedCrop(Job *job, QString *error);
    bool blendMaskedResult(Job *job, const QString &downloadedPath, QString *error);
    void queueJob(Job *job);
    void completeJob(Job *job, const QJsonObject &entry);
    void failJob(Job *job, const QString &message);
    void cancelJob(Job *job);
    QJsonObject buildGraph(const Job &job) const;
    void writeSidecar(const Job &job, const QString &imagePath) const;

    QString resolveModel(int steps) const;
    QString resolveTextEncoder() const;
    QString mapSampler(const QString &uiName) const;
    void loadServerOptions();
    void log(const QString &text);
    void emitState(State state);

    QNetworkAccessManager *m_net;
    QProcess *m_proc;
    QTimer *m_health;
    QTimer *m_poll;
    int m_port;
    int m_healthTicks;
    bool m_owned;              //!< true when this backend spawned the process
    QHash<QString, Job *> m_jobs;        //!< job id -> job
    QHash<QString, QString> m_byPrompt;  //!< prompt id -> job id
    int m_step;
    int m_total;
    QString m_activeJob;
};

#endif // COMFYBACKEND_H
