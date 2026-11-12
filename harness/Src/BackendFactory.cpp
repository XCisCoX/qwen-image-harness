#include "BackendFactory.h"

#include "ComfyBackend.h"

/*!
 * ===========================================================================
 *  SOLDERING PAD - connect Qwen-Image-2.1 here
 * ===========================================================================
 *
 *  The whole UI talks to the abstract Backend class in Backend.h and never to
 *  anything concrete. To go live:
 *
 *    1. Add a class deriving from Backend (for example QwenHttpBackend) with:
 *
 *         void    startServer()                       // spawn/launch/attach
 *         void    stopServer()                        // shut it down
 *         QString submit(const GenerationRequest &)   // queue a job
 *         void    cancel(const QString &jobId)        // stop a job
 *
 *    2. Map your transport onto the existing signals:
 *
 *         stateChanged(int)                       Backend::Stopped..Failed
 *         progress(jobId, percent, stage)         0..100 + short stage text
 *         jobFinished(GenerationResult)           result.images[].path
 *         jobFailed(jobId, message)               human readable error
 *
 *    3. Return an instance of it below.
 *
 *  `GenerationRequest` already carries everything the four workflows need:
 *  prompt, negative prompt, size, steps, cfg, batch, sampler, seed, the ordered
 *  image list (index 0 = primary) and the mask path. Paths are local files;
 *  the UI has already written the mask to disk for you.
 *
 *  Nothing else in the UI needs to change. The mock below exercises every code
 *  path (progress, cancellation, saving, discard, gallery refresh) so you can
 *  develop the real client against a live UI.
 * ===========================================================================
 */
Backend *createBackend(QObject *parent)
{
    return new ComfyBackend(parent);
}
