# Wiring the real Qwen-Image-2.1 backend

The UI never talks to HTTP, a socket or a process directly. Everything goes through the abstract
`Backend` class in `Src/Backend.h`, and exactly one function decides which implementation runs:

```cpp
// Src/BackendFactory.cpp
Backend *createBackend(QObject *parent)
{
    return new MockBackend(parent);   // <- replace with your backend
}
```

`MockBackend` is a complete, working implementation (job lifecycle, progress, cancellation,
PNG output, masking). Keeping it around while you build the real client lets you compare
behaviour side by side.

## The contract

Derive from `Backend` and implement four methods:

```cpp
void    startServer();                      // spawn / connect / load the model
void    stopServer();                       // shut it down
QString submit(const GenerationRequest &);  // queue one job, return a job id
void    cancel(const QString &jobId);       // stop a running job
```

Then emit the four signals:

| Signal | When | Notes |
| --- | --- | --- |
| `stateChanged(int)` | server state changes | values are `Backend::Stopped/Starting/Running/Stopping/Failed` |
| `progress(jobId, percent, stage)` | while a job runs | `percent` 0–100, `stage` is the short text shown in the status strip and output pane, e.g. `"Sampling 18/30"` |
| `jobFinished(GenerationResult)` | job completes | fill `images` with absolute file paths |
| `jobFailed(jobId, message)` | job cannot run | `message` is shown in a toast and the log |

Rules that matter:

* **Emit from the object's thread.** The backend lives in the GUI thread. If you poll or stream
  from a worker, marshal with a queued connection (`QMetaObject::invokeMethod` or a signal).
* **`submit()` returns an id even before work starts**, or an empty string if it cannot queue.
  The UI keeps that id and calls `cancel(id)` for the Cancel button.
* **Never block.** Do not sleep in `submit()`; set up your request and return.
* **Write results into the output directory** (`AppSettings::instance().outputDir()`), because
  that is what the gallery watches and what the "Saved to" line reports. Absolute paths anywhere
  else still display, but they will not appear in the gallery.

## The request

`GenerationRequest` (see `Src/Backend.h`) already carries everything the four workflows collect:

| Field | Meaning |
| --- | --- |
| `mode` | `Generate`, `Edit`, `FaceSwap`, `MaskInfill` |
| `prompt`, `negativePrompt` | trimmed user text |
| `size` | requested output size in pixels |
| `steps`, `cfg`, `batch`, `sampler` | sampler parameters |
| `seed` | fixed seed, or `-1` to let the backend pick |
| `images` | ordered absolute paths. **Index 0 is the primary image**, the rest are references. Between 0 and 16 entries. |
| `maskPath` | absolute path to a grayscale PNG (white = regenerate). Only set for `MaskInfill`, and only when the user painted or loaded a mask. Empty means "no mask". |

The mask is never part of `images`. In the mask workflow, `images[0]` is the picture being
masked and `images[1..]` are references.

Face swap is the same request as edit — the UI just relabels the controls. You can branch on
`mode` if the service exposes a dedicated endpoint, or ignore it entirely.

## Mask format

The UI writes the mask to `maskPath` right before `submit()`:

* PNG, same aspect as `images[0]` (the mask is stored at the primary image's resolution),
* grayscale: **white = area to regenerate**, black = keep,
* anti-aliased edges are preserved, so a soft brush gives soft transitions.

Read it as a single-channel image and threshold at ~127, or use it directly as an inpaint matte.

## Server lifecycle

`startServer()` / `stopServer()` are driven by the "Start server / Stop server" switch in the
sidebar. The mock fakes a short startup; the real implementation would launch the process or
call a health endpoint and flip `stateChanged`.

Settings already hold the fields for this (`command`, `arguments`, `workingDir`, `port`,
`autoStart`, `stopOnExit`) — read them with `AppSettings::instance().server()`. They are exposed
in the Settings page and persisted, but the bundled mock never spawns anything.

## Discarding

`Backend::discardImage(path)` defaults to deleting the image and its `.json` sidecar. Override it
if your service also keeps an index or a database entry that must be updated.

## Example skeleton

```cpp
class QwenHttpBackend : public Backend
{
    Q_OBJECT
public:
    explicit QwenHttpBackend(QObject *parent = nullptr) : Backend(parent) {}

    void startServer() override { setState(Running); }   // or spawn + poll health
    void stopServer() override  { setState(Stopped); }

    QString submit(const GenerationRequest &req) override
    {
        const QString id = nextId();
        // 1. POST /v1/jobs {prompt, size, steps, cfg, seed, images, mask}
        // 2. poll GET /v1/jobs/{id} on a QTimer
        //    emit progress(id, percent, stage);
        // 3. on completion download each result into outputDir()
        //    emit jobFinished(result);
        return id;
    }

    void cancel(const QString &id) override { /* POST /v1/jobs/{id}/cancel */ }
};
```

Add `QT += network` to `Src/QImageHarness.pro` when you use QtNetwork.

## Checklist before you consider it wired

- [ ] `submit()` returns quickly and never blocks the UI.
- [ ] Progress reaches 100 and the status strip returns to idle.
- [ ] Cancel ends the job and leaves no partial files in the output directory.
- [ ] Results land in the configured output directory and appear in Gallery immediately.
- [ ] `jobFailed` produces a readable message (it is shown verbatim).
- [ ] An empty mask and `images.size() == 1` behave sensibly for `MaskInfill`.
- [ ] Starting/stopping the server twice in a row does not crash or hang.
