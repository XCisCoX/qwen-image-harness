#ifndef MOCKBACKEND_H
#define MOCKBACKEND_H

#include "Backend.h"

#include <QDateTime>
#include <QHash>
#include <QImage>
#include <QTimer>

/*!
 * A self-contained stand-in for the real service. It fakes the job lifecycle,
 * paints plausible result images from the prompt/seed (and from the input
 * image when there is one) and writes them to the output directory so that
 * every part of the UI - progress, results, discard, gallery - can be exercised
 * before the real backend is wired in.
 */
class MockBackend : public Backend
{
    Q_OBJECT

public:
    explicit MockBackend(QObject *parent = nullptr);
    ~MockBackend();

    void startServer() override;
    void stopServer() override;
    QString submit(const GenerationRequest &request) override;
    void cancel(const QString &jobId) override;

private slots:
    void onTick();

private:
    struct Job {
        QString id;
        GenerationRequest request;
        int step;
        int totalSteps;
        qint64 seed;
        QDateTime startedAt;
        QTimer *timer;

        Job()
            : step(0)
            , totalSteps(1)
            , seed(0)
            , timer(nullptr)
        {
        }
    };

    Job *findJob(const QString &jobId) const;
    void dropJob(Job *job);
    void decodeAndSave(const QString &jobId);
    QImage compose(const GenerationRequest &request, int index, qint64 seed) const;
    bool saveImage(const QImage &image, const GenerationRequest &request, int index,
                   qint64 seed, QString *pathOut, QString *errorOut) const;

    QHash<QString, Job *> m_jobs;
    int m_counter;
};

#endif // MOCKBACKEND_H
