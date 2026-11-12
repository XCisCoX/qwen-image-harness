#include "MockBackend.h"

#include "Log.h"
#include "Settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QStringList>

namespace {

QImage loadScaled(const QString &path, const QSize &target)
{
    if (path.isEmpty())
        return QImage();

    QImageReader reader(path);
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    reader.setAutoTransform(true);
#endif
    QImage img = reader.read();
    if (img.isNull())
        return img;

    img = img.scaled(target, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const int x = qMax(0, (img.width() - target.width()) / 2);
    const int y = qMax(0, (img.height() - target.height()) / 2);
    return img.copy(x, y, target.width(), target.height());
}

QColor tinted(const QColor &c, int alpha)
{
    return QColor(c.red(), c.green(), c.blue(), alpha);
}

} // namespace

MockBackend::MockBackend(QObject *parent)
    : Backend(parent)
    , m_counter(0)
{
}

MockBackend::~MockBackend()
{
    const QStringList ids = m_jobs.keys();
    for (int i = 0; i < ids.size(); ++i) {
        Job *j = m_jobs.value(ids.at(i));
        if (j) {
            if (j->timer)
                j->timer->stop();
            delete j->timer;
            delete j;
        }
    }
    m_jobs.clear();
}

void MockBackend::startServer()
{
    if (state() == Running || state() == Starting)
        return;

    setState(Starting);
    Logging::backend("Launching mock backend (no network transport)");
    Logging::backend("Loading Qwen-Image-2.1 weights (simulated)");

    QTimer::singleShot(900, this, [this]() {
        if (this->state() != Starting)
            return;
        const ServerOptions &srv = AppSettings::instance().server();
        Logging::backend(QString("Mock service ready on port %1").arg(srv.port));
        Logging::success("Backend is running");
        setState(Running);
    });
}

void MockBackend::stopServer()
{
    if (state() == Stopped)
        return;

    const QStringList ids = m_jobs.keys();
    for (int i = 0; i < ids.size(); ++i)
        cancel(ids.at(i));

    setState(Stopping);
    Logging::backend("Stopping mock backend");
    setState(Stopped);
    Logging::warn("Backend stopped");
}

MockBackend::Job *MockBackend::findJob(const QString &jobId) const
{
    return m_jobs.value(jobId, nullptr);
}

void MockBackend::dropJob(Job *job)
{
    if (!job)
        return;
    m_jobs.remove(job->id);
    if (job->timer) {
        job->timer->stop();
        job->timer->deleteLater();
    }
    job->timer = nullptr;
    delete job;
}

QString MockBackend::submit(const GenerationRequest &request)
{
    if (state() != Running) {
        Logging::error("Cannot queue a job: backend is not running");
        return QString();
    }

    Job *job = new Job();
    job->id = QString("job-%1").arg(++m_counter);
    job->request = request;
    job->totalSteps = qMax(4, request.steps);
    job->seed = request.seed >= 0
                    ? request.seed
                    : (QDateTime::currentMSecsSinceEpoch() / 7) % 1000000000LL + m_counter;
    job->startedAt = QDateTime::currentDateTime();

    job->timer = new QTimer(this);
    job->timer->setInterval(55);
    connect(job->timer, &QTimer::timeout, this, &MockBackend::onTick);

    m_jobs.insert(job->id, job);

    Logging::backend(QString("%1 queued %2 - %3x%4, %5 steps, batch %6")
                         .arg(job->id)
                         .arg(pageModeName(request.mode))
                         .arg(request.size.width())
                         .arg(request.size.height())
                         .arg(request.steps)
                         .arg(request.batch));
    if (!request.images.isEmpty()) {
        Logging::backend(QString("%1 input images: %2, mask: %3")
                             .arg(job->id)
                             .arg(request.images.size())
                             .arg(request.hasMask() ? "yes" : "no"));
    }

    emit progress(job->id, 2, "Queued");
    job->timer->start();
    return job->id;
}

void MockBackend::cancel(const QString &jobId)
{
    Job *job = findJob(jobId);
    if (!job)
        return;

    GenerationResult result;
    result.jobId = job->id;
    result.mode = job->request.mode;
    result.cancelled = true;

    dropJob(job);
    Logging::warn(QString("%1 cancelled").arg(jobId));
    emit jobFinished(result);
}

void MockBackend::onTick()
{
    QTimer *timer = qobject_cast<QTimer *>(sender());
    if (!timer)
        return;

    Job *job = nullptr;
    QHash<QString, Job *>::iterator it = m_jobs.begin();
    for (; it != m_jobs.end(); ++it) {
        if (it.value()->timer == timer) {
            job = it.value();
            break;
        }
    }
    if (!job)
        return;

    job->step += 1;

    const int pct = 6 + int(72.0 * double(job->step) / double(qMax(1, job->totalSteps)));
    emit progress(job->id, qMin(pct, 80),
                  QString("Sampling %1/%2").arg(qMin(job->step, job->totalSteps)).arg(job->totalSteps));

    if (job->step >= job->totalSteps) {
        job->timer->stop();
        emit progress(job->id, 86, "Decoding latents");
        const QString id = job->id;
        QTimer::singleShot(140, this, [this, id]() { decodeAndSave(id); });
    }
}

void MockBackend::decodeAndSave(const QString &jobId)
{
    Job *job = findJob(jobId);
    if (!job)
        return;

    const GenerationRequest req = job->request;
    const qint64 seed = job->seed;

    emit progress(jobId, 92, "Decoding latents");

    GenerationResult result;
    result.jobId = jobId;
    result.mode = req.mode;

    const int count = qMax(1, req.batch);
    for (int i = 0; i < count; ++i) {
        const QImage img = compose(req, i, seed + i);
        QString path;
        QString error;
        if (!saveImage(img, req, i, seed + i, &path, &error)) {
            result.error = error;
            dropJob(job);
            Logging::error(QString("%1 failed: %2").arg(jobId).arg(error));
            emit progress(jobId, 0, "Failed");
            emit jobFailed(jobId, error);
            return;
        }
        GeneratedImage gi;
        gi.path = path;
        result.images.append(gi);
        emit progress(jobId, 92 + int(7.0 * (i + 1) / count), "Writing results");
    }

    const qint64 elapsed = job->startedAt.msecsTo(QDateTime::currentDateTime());
    dropJob(job);

    emit progress(jobId, 100, "Done");
    Logging::success(QString("%1 finished in %2s - %3 image(s)")
                         .arg(jobId)
                         .arg(QString::number(elapsed / 1000.0, 'f', 1))
                         .arg(result.images.size()));
    emit jobFinished(result);
}

QImage MockBackend::compose(const GenerationRequest &req, int index, qint64 seed) const
{
    Q_UNUSED(index)
    const QSize size = req.size.isValid() ? req.size : QSize(1024, 1024);
    const int w = qBound(64, size.width(), 2048);
    const int h = qBound(64, size.height(), 2048);
    const QSize out(w, h);

    const quint32 hash = qHash(req.prompt + "|" + req.negativePrompt + "|" + QString::number(seed));
    quint32 state = hash | 1u;
    const auto rnd = [&state]() -> quint32 {
        state = state * 1664525u + 1013904223u;
        return (state >> 8) & 0xFFFFFF;
    };

    const int hue = int(hash % 360u);
    const QColor c1 = QColor::fromHsv(hue, 140, 46);
    const QColor c2 = QColor::fromHsv((hue + 38) % 360, 165, 105);
    const QColor c3 = QColor::fromHsv((hue + 305) % 360, 190, 210);

    QImage base;
    if (!req.images.isEmpty())
        base = loadScaled(req.images.first(), out);

    QImage canvas(out, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::black);

    QPainter p(&canvas);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (!base.isNull()) {
        // Blur the input so the result reads as a variation of it.
        const QImage blur = base.scaled(qMax(2, w / 12), qMax(2, h / 12), Qt::IgnoreAspectRatio,
                                        Qt::SmoothTransformation)
                                .scaled(out, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        p.drawImage(0, 0, blur);
        p.fillRect(canvas.rect(), tinted(c1, 95));
    } else {
        QLinearGradient g(0, 0, w, h);
        g.setColorAt(0.0, c1);
        g.setColorAt(0.55, c2);
        g.setColorAt(1.0, QColor(7, 8, 11));
        p.fillRect(canvas.rect(), g);
    }

    // Soft colour fields.
    for (int i = 0; i < 9; ++i) {
        const qreal cx = (rnd() % 1000) / 1000.0 * w;
        const qreal cy = (rnd() % 1000) / 1000.0 * h;
        const qreal radius = (0.14 + (rnd() % 1000) / 1000.0 * 0.36) * qMax(w, h);
        QColor col = QColor::fromHsv((hue + int(rnd() % 130)) % 360,
                                     150 + int(rnd() % 90),
                                     120 + int(rnd() % 120),
                                     120 + int(rnd() % 70));
        QRadialGradient rg(cx, cy, radius);
        rg.setColorAt(0.0, col);
        col.setAlpha(0);
        rg.setColorAt(1.0, col);
        p.setPen(Qt::NoPen);
        p.setBrush(rg);
        p.drawEllipse(QPointF(cx, cy), radius, radius);
    }

    // A couple of sweeping strokes for structure.
    p.setBrush(Qt::NoBrush);
    for (int i = 0; i < 3; ++i) {
        QPainterPath path;
        const qreal y0 = (rnd() % 1000) / 1000.0 * h;
        path.moveTo(-10, y0);
        path.cubicTo(w * 0.3, (rnd() % 1000) / 1000.0 * h,
                     w * 0.7, (rnd() % 1000) / 1000.0 * h,
                     w + 10, (rnd() % 1000) / 1000.0 * h);
        QColor sc = c3;
        sc.setAlpha(40 + int(rnd() % 50));
        QPen pen(sc, 1.0 + (rnd() % 100) / 40.0);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.drawPath(path);
    }

    // Grain.
    p.setPen(Qt::NoPen);
    const int grains = qMin(2600, (w * h) / 900);
    for (int i = 0; i < grains; ++i) {
        const int x = int(rnd() % quint32(w));
        const int y = int(rnd() % quint32(h));
        p.fillRect(QRect(x, y, 1, 1), QColor(255, 255, 255, 8 + int(rnd() % 16)));
    }

    // Vignette.
    QRadialGradient vig(QPointF(w / 2.0, h / 2.0), qMax(w, h) * 0.72);
    vig.setColorAt(0.55, QColor(0, 0, 0, 0));
    vig.setColorAt(1.0, QColor(0, 0, 0, 150));
    p.fillRect(canvas.rect(), vig);

    if (req.mode == PageMode::MaskInfill && !req.maskPath.isEmpty()) {
        const QImage mask = loadScaled(req.maskPath, out);
        if (!mask.isNull()) {
            const QImage maskArgb = mask.convertToFormat(QImage::Format_ARGB32);
            QImage alpha(out, QImage::Format_ARGB32);
            alpha.fill(Qt::transparent);
            for (int y = 0; y < h; ++y) {
                const QRgb *src = reinterpret_cast<const QRgb *>(maskArgb.constScanLine(y));
                QRgb *dst = reinterpret_cast<QRgb *>(alpha.scanLine(y));
                for (int x = 0; x < w; ++x)
                    dst[x] = qRgba(0, 0, 0, qGray(src[x]));
            }
            // Feather the edge so the fill blends instead of cutting hard.
            alpha = alpha.scaled(qMax(2, w / 5), qMax(2, h / 5), Qt::IgnoreAspectRatio,
                                 Qt::SmoothTransformation)
                        .scaled(out, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

            p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            p.drawImage(0, 0, alpha);
            p.setCompositionMode(QPainter::CompositionMode_SourceOver);

            QImage result = base.isNull() ? QImage(out, QImage::Format_ARGB32_Premultiplied)
                                          : base.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            if (result.isNull())
                result = QImage(out, QImage::Format_ARGB32_Premultiplied);
            if (base.isNull()) {
                result.fill(QColor(10, 11, 14));
            }
            QPainter rp(&result);
            rp.drawImage(0, 0, canvas);
            rp.end();
            p.end();
            return result;
        }
    }

    if (!base.isNull() && req.mode != PageMode::Generate) {
        QImage result = base.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        QPainter rp(&result);
        rp.setOpacity(0.82);
        rp.drawImage(0, 0, canvas);
        rp.end();
        p.end();
        return result;
    }

    p.end();
    return canvas;
}

bool MockBackend::saveImage(const QImage &image, const GenerationRequest &req, int index,
                            qint64 seed, QString *pathOut, QString *errorOut) const
{
    const AppSettings &settings = AppSettings::instance();
    const QString dir = settings.outputDir();
    if (dir.isEmpty()) {
        if (errorOut)
            *errorOut = "Output directory is not configured";
        return false;
    }
    if (!QDir().mkpath(dir)) {
        if (errorOut)
            *errorOut = QString("Cannot create output directory: %1").arg(QDir::toNativeSeparators(dir));
        return false;
    }

    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString name = QString("qwen_%1_%2_%3_%4")
                       .arg(pageModeId(req.mode))
                       .arg(stamp)
                       .arg(seed)
                       .arg(index + 1);
    QString path = dir + "/" + name + ".png";
    int suffix = 1;
    while (QFileInfo(path).exists()) {
        path = dir + "/" + name + QString("_%1").arg(suffix++) + ".png";
    }

    if (!image.save(path, "PNG")) {
        if (errorOut)
            *errorOut = QString("Cannot write %1").arg(QDir::toNativeSeparators(path));
        return false;
    }

    if (settings.data().sidecar) {
        QJsonObject meta;
        meta.insert("mode", pageModeId(req.mode));
        meta.insert("prompt", req.prompt);
        meta.insert("negative_prompt", req.negativePrompt);
        meta.insert("width", req.size.width());
        meta.insert("height", req.size.height());
        meta.insert("steps", req.steps);
        meta.insert("cfg", req.cfg);
        meta.insert("sampler", req.sampler);
        meta.insert("seed", double(seed));
        meta.insert("batch_index", index + 1);
        meta.insert("batch_size", req.batch);
        meta.insert("created", QDateTime::currentDateTime().toString(Qt::ISODate));
        QJsonArray inputs;
        for (int i = 0; i < req.images.size(); ++i)
            inputs.append(req.images.at(i));
        meta.insert("input_images", inputs);
        if (!req.maskPath.isEmpty())
            meta.insert("mask", req.maskPath);

        const QString sidecar = QFileInfo(path).absolutePath() + "/"
                                + QFileInfo(path).completeBaseName() + ".json";
        QFile f(sidecar);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            f.write(QJsonDocument(meta).toJson(QJsonDocument::Indented));
            f.close();
        }
    }

    Logging::backend(QString("Saved %1").arg(QFileInfo(path).fileName()));
    if (pathOut)
        *pathOut = path;
    return true;
}
