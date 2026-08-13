#include "MediaProber.h"

#include <QFileInfo>
#include <QImageReader>
#include <QMediaPlayer>
#include <QMediaMetaData>
#include <QSet>
#include <QVideoSink>
#include <QUrl>

namespace beta {

namespace {
const QSet<QString> VIDEO_EXTS = {
    "mp4","mov","mkv","avi","webm","m4v","wmv","flv","mpg","mpeg","ts"
};
const QSet<QString> AUDIO_EXTS = {
    "mp3","wav","aac","flac","ogg","m4a","opus","wma"
};
const QSet<QString> IMAGE_EXTS = {
    "png","jpg","jpeg","bmp","gif","webp","tiff","tga"
};
}

bool MediaProber::isImageFile(const QString& path) {
    return IMAGE_EXTS.contains(QFileInfo(path).suffix().toLower());
}
bool MediaProber::isVideoFile(const QString& path) {
    return VIDEO_EXTS.contains(QFileInfo(path).suffix().toLower());
}
bool MediaProber::isAudioFile(const QString& path) {
    return AUDIO_EXTS.contains(QFileInfo(path).suffix().toLower());
}

MediaProber::MediaProber(QObject* parent)
    : QObject(parent)
{
    player_ = new QMediaPlayer(this);
    sink_   = new QVideoSink(this);
    player_->setVideoOutput(sink_);

    connect(player_, &QMediaPlayer::durationChanged,
            this, &MediaProber::onDurationChanged);
    connect(player_, &QMediaPlayer::tracksChanged,
            this, &MediaProber::onTracksChanged);
    connect(player_, &QMediaPlayer::mediaStatusChanged,
            this, &MediaProber::onMediaStatusChanged);
}

void MediaProber::probeAsync(const QString& path)
{
    if (cache_.contains(path) && cache_[path].probed) {
        emit probed(path, cache_[path]);
        return;
    }
    if (isImageFile(path)) {
        QImageReader reader(path);
        QSize sz = reader.size();
        Info i;
        i.path = path;
        i.name = QFileInfo(path).fileName();
        i.kind = "image";
        i.width = sz.width();
        i.height = sz.height();
        i.durationMs = 0;       // images have no inherent duration
        i.fps = 30.0;
        i.probed = true;
        cache_[path] = i;
        emit probed(path, i);
        return;
    }
    if (pending_.contains(path)) return;
    pending_.enqueue(path);
    processNext();
}

MediaProber::Info MediaProber::info(const QString& path) const
{
    return cache_.value(path);
}

void MediaProber::processNext()
{
    if (probing_ || pending_.isEmpty()) return;
    probing_ = true;
    currentPath_ = pending_.dequeue();

    Info seed;
    seed.path = currentPath_;
    seed.name = QFileInfo(currentPath_).fileName();
    if (isVideoFile(currentPath_))      seed.kind = "video";
    else if (isAudioFile(currentPath_)) seed.kind = "audio";
    else                                seed.kind = "unknown";
    seed.fps = 30.0;
    cache_[currentPath_] = seed;

    player_->setSource(QUrl::fromLocalFile(currentPath_));
}

void MediaProber::onDurationChanged(qint64 dur)
{
    if (!probing_ || currentPath_.isEmpty()) return;
    if (dur > 0) {
        cache_[currentPath_].durationMs = dur;
    }
}

void MediaProber::onTracksChanged()
{
    if (!probing_ || currentPath_.isEmpty()) return;
    const auto videoTracks = player_->videoTracks();
    if (!videoTracks.isEmpty()) {
        QSize res = videoTracks.first().value(QMediaMetaData::Resolution).toSize();
        if (!res.isEmpty()) {
            cache_[currentPath_].width = res.width();
            cache_[currentPath_].height = res.height();
        }
    }
    const auto fpsVal = player_->metaData().value(QMediaMetaData::VideoFrameRate);
    if (fpsVal.isValid()) {
        bool ok = false;
        double f = fpsVal.toDouble(&ok);
        if (ok && f > 0) cache_[currentPath_].fps = f;
    }
}

void MediaProber::onMediaStatusChanged(int status)
{
    if (!probing_ || currentPath_.isEmpty()) return;
    auto st = static_cast<QMediaPlayer::MediaStatus>(status);
    if (st == QMediaPlayer::LoadedMedia ||
        st == QMediaPlayer::EndOfMedia  ||
        st == QMediaPlayer::InvalidMedia) {
        Info i = cache_[currentPath_];
        i.probed = true;
        cache_[currentPath_] = i;
        finishCurrent(i);
    }
}

void MediaProber::finishCurrent(const Info& info)
{
    const QString p = currentPath_;
    currentPath_.clear();
    probing_ = false;
    emit probed(p, info);
    processNext();
}

} // namespace beta
