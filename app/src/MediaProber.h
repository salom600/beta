#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QQueue>

class QMediaPlayer;
class QVideoSink;

namespace beta {

/// Probes media files asynchronously via QtMultimedia and caches the
/// results. For images we use the synchronous QImageReader; for audio
/// and video we use a single QMediaPlayer and queue requests.
class MediaProber : public QObject {
    Q_OBJECT
public:
    struct Info {
        QString path;
        QString name;
        QString kind;        // "video" / "audio" / "image" / "unknown"
        qint64 durationMs = 0;
        int width = 0;
        int height = 0;
        double fps = 30.0;
        bool probed = false;
    };

    explicit MediaProber(QObject* parent = nullptr);

    /// Kick off an async probe. `probed()` is emitted when the result is
    /// ready (or immediately for images, which are synchronous).
    void probeAsync(const QString& path);

    /// Returns cached info for `path` (or a default-constructed Info if
    /// not yet probed).
    Info info(const QString& path) const;

    static bool isImageFile(const QString& path);
    static bool isVideoFile(const QString& path);
    static bool isAudioFile(const QString& path);

signals:
    void probed(const QString& path, const Info& info);

private slots:
    void onDurationChanged(qint64 dur);
    void onTracksChanged();
    void onMediaStatusChanged(int status);

private:
    void processNext();
    void finishCurrent(const Info& info);

    QHash<QString, Info> cache_;
    QQueue<QString> pending_;
    QString currentPath_;
    bool probing_ = false;

    QMediaPlayer* player_ = nullptr;
    QVideoSink*   sink_   = nullptr;
};

} // namespace beta
