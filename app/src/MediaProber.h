#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QQueue>
#include <QPixmap>
#include <QVector>

class QMediaPlayer;
class QVideoSink;
class QAudioDecoder;

namespace beta {

/// Probes media files asynchronously via QtMultimedia and caches the
/// results. For images we use the synchronous QImageReader; for audio
/// and video we use a single QMediaPlayer and queue requests.
///
/// Also produces thumbnails:
///   • video  -> first decoded frame (grabbed via QVideoSink)
///   • image  -> scaled QImageReader thumbnail
///   • audio  -> waveform QPixmap computed from decoded PCM peaks
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
    ~MediaProber() override;

    void probeAsync(const QString& path);
    void requestThumbnail(const QString& path, QSize size = QSize(160, 90));

    Info info(const QString& path) const;
    QPixmap thumbnail(const QString& path) const;
    bool hasThumbnail(const QString& path) const;

    static bool isImageFile(const QString& path);
    static bool isVideoFile(const QString& path);
    static bool isAudioFile(const QString& path);

signals:
    void probed(const QString& path, const Info& info);
    void thumbnailReady(const QString& path, const QPixmap& thumb);

private slots:
    void onDurationChanged(qint64 dur);
    void onTracksChanged();
    void onMediaStatusChanged(int status);
    void onVideoFrameChanged(const class QVideoFrame &frame);

private:
    void processNextProbe();
    void finishCurrentProbe(const Info& info);
    void processNextThumb();
    void makeImageThumbnail(const QString& path, QSize size);
    void makeVideoThumbnail(const QString& path, QSize size);
    void makeAudioWaveform(const QString& path, QSize size);
    void finalizeThumbnail(const QString& path, const QPixmap& pix);

    struct CachedInfo {
        Info info;
        QPixmap thumb;
        bool hasThumb = false;
    };
    QHash<QString, CachedInfo> cache_;

    // Probe queue
    QQueue<QString> pending_;
    QString currentPath_;
    bool probing_ = false;
    QMediaPlayer* probePlayer_ = nullptr;
    QVideoSink*   probeSink_   = nullptr;

    // Thumbnail queue
    struct ThumbReq { QString path; QSize size; };
    QQueue<ThumbReq> thumbPending_;
    bool thumbWorking_ = false;
    QString thumbCurrentPath_;
    QSize    thumbCurrentSize_;

    // Separate player for video thumbnail grabbing
    QMediaPlayer* thumbPlayer_ = nullptr;
    QVideoSink*   thumbSink_   = nullptr;

    // Audio decoder for waveforms
    QAudioDecoder* audioDecoder_ = nullptr;
    QVector<float> audioPeaks_;
};

} // namespace beta
