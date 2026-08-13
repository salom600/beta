#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QHash>
#include <cstdint>

class QProcess;
class QTemporaryDir;

namespace beta {

class EngineBridge;

/// Drives FFmpeg (as a subprocess) to render the project's timeline
/// to a video file. The strategy is:
///
///  1. Collect every clip on every track in timeline order.
///  2. For each clip, re-encode it to a normalized intermediate file
///     (same resolution, fps, codec, audio sample-rate) using FFmpeg.
///     For images, build a video of the requested duration.
///  3. Concatenate the intermediates with FFmpeg's concat demuxer
///     into the final output file.
///
/// Requires `ffmpeg` to be on PATH (or `ffmpegPath` set explicitly).
class Exporter : public QObject {
    Q_OBJECT
public:
    struct ClipItem {
        QString  path;
        QString  name;
        QString  kind;          // "video" | "audio" | "image"
        uint64_t trimInFrames  = 0;
        uint64_t durationFrames = 0;
        double   fps = 30.0;
        int      width = 0;
        int      height = 0;
        double   volume = 1.0;
        double   opacity = 1.0;
        double   scale = 1.0;
        bool     muted = false;
        bool     visible = true;
    };

    explicit Exporter(QObject* parent = nullptr);
    ~Exporter() override;

    /// Locate ffmpeg on PATH (returns empty string if not found).
    static QString findFFmpeg();

    void setFFmpegPath(const QString& path) { ffmpegPath_ = path; }
    QString ffmpegPath() const { return ffmpegPath_; }

    /// Start exporting the given clips with the given settings.
    /// `clipsByTrack` is one list per track (parallel tracks are
    /// currently mixed down to a single sequential timeline).
    void start(const QList<QList<ClipItem>>& clipsByTrack,
               const struct ExportSettings& settings,
               double projectFps);

    bool isRunning() const;

public slots:
    void cancel();

signals:
    void progress(int percent, const QString& stage);
    void finished(bool success, const QString& message);
    void log(const QString& line);

private slots:
    void onStageFinished(int exitCode, QProcess::ExitStatus status);
    void onStageOutput();

private:
    void advanceStage();
    void runNextStage();
    QString buildNormalizeArgs(const ClipItem& clip,
                                const QString& intermediatePath,
                                const ExportSettings& s,
                                double fps) const;
    QString buildConcatArgs(const QStringList& intermediateFiles,
                             const QString& outputPath,
                             const ExportSettings& s) const;

    QString ffmpegPath_;
    QProcess* process_ = nullptr;
    QTemporaryDir* tempDir_ = nullptr;
    QList<QStringList> pendingStages_;
    QStringList currentArgs_;
    int totalStages_ = 0;
    int doneStages_ = 0;
    QString currentStageDescription_;
};

} // namespace beta
