#include "Exporter.h"
#include "ExportDialog.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextStream>

#include <cmath>

namespace beta {

Exporter::Exporter(QObject* parent)
    : QObject(parent)
{
    process_ = new QProcess(this);
    connect(process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &Exporter::onStageFinished);
    connect(process_, &QProcess::readyReadStandardOutput,
            this, &Exporter::onStageOutput);
    connect(process_, &QProcess::readyReadStandardError,
            this, &Exporter::onStageOutput);

    ffmpegPath_ = findFFmpeg();
}

Exporter::~Exporter()
{
    if (process_ && process_->state() != QProcess::NotRunning) {
        process_->kill();
        process_->waitForFinished(2000);
    }
    delete tempDir_;
}

QString Exporter::findFFmpeg()
{
    // Allow override via env
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString envPath = env.value("BETA_FFMPEG");
    if (!envPath.isEmpty() && QFileInfo(envPath).isExecutable()) {
        return envPath;
    }
    return QStandardPaths::findExecutable("ffmpeg");
}

bool Exporter::isRunning() const
{
    return process_ && process_->state() != QProcess::NotRunning;
}

void Exporter::cancel()
{
    if (isRunning()) {
        process_->kill();
    }
}

void Exporter::start(const QList<QList<ClipItem>>& clipsByTrack,
                     const ExportSettings& settings,
                     double projectFps)
{
    if (isRunning()) {
        emit finished(false, tr("An export is already running."));
        return;
    }
    if (ffmpegPath_.isEmpty()) {
        emit finished(false, tr("FFmpeg not found. Install FFmpeg "
                                "(https://ffmpeg.org) and ensure it is on "
                                "your PATH, then try again."));
        return;
    }

    // Collect a single timeline: for now we walk tracks in order,
    // concatenating each track's clips. (A proper mixing pass would
    // overlay parallel tracks, but for v0.2 a sequential render is a
    // reasonable baseline.)
    QList<ClipItem> timeline;
    for (const auto& track : clipsByTrack) {
        for (const auto& c : track) {
            if (!c.visible && c.kind != "audio") continue;
            timeline.append(c);
        }
    }
    if (timeline.isEmpty()) {
        emit finished(false, tr("No clips to export. Add at least one "
                                "clip to the timeline."));
        return;
    }

    delete tempDir_;
    tempDir_ = new QTemporaryDir(QDir::tempPath() + "/beta-export-XXXXXX");
    if (!tempDir_->isValid()) {
        emit finished(false, tr("Could not create temp directory."));
        return;
    }

    double fps = projectFps > 0 ? projectFps : settings.fps;

    pendingStages_.clear();
    doneStages_ = 0;
    totalStages_ = 0;

    QStringList intermediateFiles;
    for (int i = 0; i < timeline.size(); ++i) {
        const auto& clip = timeline[i];
        QString intermediate = tempDir_->filePath(
            QString("clip_%1.mkv").arg(i, 4, 10, QChar('0')));
        intermediateFiles << intermediate;

        QStringList args;
        // Build FFmpeg args. For video: extract segment from trim_in to
        // trim_in+duration. For image: use loop.
        double startSec = clip.trimInFrames / fps;
        double durSec   = clip.durationFrames / fps;

        if (clip.kind == "image") {
            args << "-y"
                 << "-loop" << "1" << "-framerate" << QString::number(fps, 'f', 3)
                 << "-t" << QString::number(durSec, 'f', 3)
                 << "-i" << clip.path;
        } else if (clip.kind == "audio") {
            args << "-y"
                 << "-ss" << QString::number(startSec, 'f', 3)
                 << "-t"  << QString::number(durSec, 'f', 3)
                 << "-i" << clip.path;
        } else {
            args << "-y"
                 << "-ss" << QString::number(startSec, 'f', 3)
                 << "-t"  << QString::number(durSec, 'f', 3)
                 << "-i" << clip.path;
        }

        // Common encoding args
        if (clip.kind != "audio") {
            args << "-vf"
                 << QString("scale=%1:%2,format=yuv420p")
                        .arg(settings.width).arg(settings.height);
            args << "-r" << QString::number(fps, 'f', 3);
            args << "-c:v" << settings.videoCodec
                 << "-b:v" << QString("%1k").arg(settings.videoBitrate)
                 << "-pix_fmt" << "yuv420p";
        } else {
            args << "-vn";
        }

        if (settings.includeAudio && clip.kind != "image" && !clip.muted) {
            args << "-c:a" << settings.audioCodec
                 << "-b:a" << QString("%1k").arg(settings.audioBitrate)
                 << "-ar" << "48000" << "-ac" << "2";
        } else {
            args << "-an";
        }

        args << intermediate;
        pendingStages_ << args;
    }

    // Final concat stage
    QString concatList = tempDir_->filePath("concat.txt");
    {
        QFile f(concatList);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream s(&f);
            for (const QString& p : intermediateFiles) {
                s << "file '" << QDir::toNativeSeparators(p) << "'\n";
            }
        }
    }

    QStringList concatArgs;
    concatArgs << "-y" << "-f" << "concat" << "-safe" << "0"
               << "-i" << concatList
               << "-c" << "copy"
               << settings.outputPath;
    pendingStages_ << concatArgs;

    totalStages_ = pendingStages_.size();
    currentStageDescription_ = tr("Preparing");
    runNextStage();
}

void Exporter::runNextStage()
{
    if (pendingStages_.isEmpty()) {
        emit finished(true, tr("Export complete."));
        return;
    }
    currentArgs_ = pendingStages_.takeFirst();
    currentStageDescription_ = (doneStages_ + 1 < totalStages_)
        ? tr("Encoding clip %1 / %2").arg(doneStages_ + 1).arg(totalStages_ - 1)
        : tr("Concatenating final output");
    emit progress(static_cast<int>(doneStages_ * 100.0 / totalStages_),
                  currentStageDescription_);

    process_->setProgram(ffmpegPath_);
    process_->setArguments(currentArgs_);
    process_->start();
}

void Exporter::onStageFinished(int exitCode, QProcess::ExitStatus status)
{
    if (exitCode != 0 || status != QProcess::NormalExit) {
        QString err = process_->readAllStandardError();
        emit finished(false, tr("FFmpeg failed (exit %1): %2")
                          .arg(exitCode).arg(err.left(500)));
        return;
    }
    doneStages_++;
    emit progress(static_cast<int>(doneStages_ * 100.0 / totalStages_),
                  currentStageDescription_);
    runNextStage();
}

void Exporter::onStageOutput()
{
    QByteArray out = process_->readAllStandardOutput() +
                     process_->readAllStandardError();
    for (const QByteArray& line : out.split('\n')) {
        if (!line.trimmed().isEmpty()) {
            emit log(QString::fromLocal8Bit(line));
        }
    }
}

} // namespace beta
