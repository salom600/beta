#include "MediaProber.h"

#include <QAudioDecoder>
#include <QAudioBuffer>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QMediaPlayer>
#include <QMediaMetaData>
#include <QPainter>
#include <QPen>
#include <QSet>
#include <QVideoFrame>
#include <QVideoSink>
#include <QUrl>
#include <QLinearGradient>
#include <cmath>

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
    probePlayer_ = new QMediaPlayer(this);
    probeSink_   = new QVideoSink(this);
    probePlayer_->setVideoOutput(probeSink_);

    connect(probePlayer_, &QMediaPlayer::durationChanged,
            this, &MediaProber::onDurationChanged);
    connect(probePlayer_, &QMediaPlayer::tracksChanged,
            this, &MediaProber::onTracksChanged);
    connect(probePlayer_, &QMediaPlayer::mediaStatusChanged,
            this, &MediaProber::onMediaStatusChanged);

    thumbPlayer_ = new QMediaPlayer(this);
    thumbSink_   = new QVideoSink(this);
    thumbPlayer_->setVideoOutput(thumbSink_);
    connect(thumbSink_, &QVideoSink::videoFrameChanged,
            this, &MediaProber::onVideoFrameChanged);

    audioDecoder_ = new QAudioDecoder(this);
    connect(audioDecoder_, &QAudioDecoder::bufferReady,
            this, [this]() {
        QAudioBuffer buf = audioDecoder_->read();
        if (!buf.isValid()) return;
        // Sum across channels, take abs peak per ~10ms chunk
        int channels = buf.format().channelCount();
        int frames = buf.frameCount();
        if (frames <= 0) return;
        float peak = 0.0f;
        const float *f = buf.constData<float>();
        const qint16 *s = buf.constData<qint16>();
        for (int i = 0; i < frames; ++i) {
            float v = 0.0f;
            if (buf.format().sampleFormat() == QAudioFormat::Float) {
                for (int c = 0; c < channels; ++c) v += f[i*channels + c];
            } else {
                for (int c = 0; c < channels; ++c) v += s[i*channels + c] / 32768.0f;
            }
            v = std::abs(v / channels);
            if (v > peak) peak = v;
        }
        audioPeaks_.append(peak);
    });
    connect(audioDecoder_, &QAudioDecoder::finished,
            this, [this]() {
        // Build waveform pixmap from audioPeaks_
        QPixmap pix(thumbCurrentSize_);
        pix.fill(QColor(20, 20, 24));
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        QLinearGradient grad(0, 0, 0, pix.height());
        grad.setColorAt(0, QColor(110, 200, 255));
        grad.setColorAt(1, QColor(60, 120, 200));
        p.setPen(Qt::NoPen);
        p.setBrush(grad);
        int midY = pix.height() / 2;
        int n = audioPeaks_.size();
        if (n > 1) {
            float stepX = float(pix.width()) / n;
            for (int i = 0; i < n; ++i) {
                float v = audioPeaks_[i];
                int h = std::max(1, int(v * pix.height() * 0.45f));
                int x = int(i * stepX);
                int w = std::max(1, int(stepX));
                p.drawRect(QRect(x, midY - h, w, h * 2));
            }
        } else {
            // No peaks — draw a flat line
            p.setPen(QPen(QColor(110, 200, 255), 2));
            p.drawLine(0, midY, pix.width(), midY);
        }
        p.end();
        finalizeThumbnail(thumbCurrentPath_, pix);
        audioPeaks_.clear();
        thumbWorking_ = false;
        processNextThumb();
    });
}

MediaProber::~MediaProber() = default;

void MediaProber::probeAsync(const QString& path)
{
    if (cache_.contains(path) && cache_[path].info.probed) {
        emit probed(path, cache_[path].info);
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
        i.fps = 30.0;
        i.probed = true;
        cache_[path].info = i;
        emit probed(path, i);
        return;
    }
    if (pending_.contains(path)) return;
    pending_.enqueue(path);
    processNextProbe();
}

MediaProber::Info MediaProber::info(const QString& path) const
{
    return cache_.value(path).info;
}

QPixmap MediaProber::thumbnail(const QString& path) const
{
    return cache_.value(path).thumb;
}

bool MediaProber::hasThumbnail(const QString& path) const
{
    return cache_.value(path).hasThumb;
}

void MediaProber::requestThumbnail(const QString& path, QSize size)
{
    if (cache_.contains(path) && cache_[path].hasThumb) {
        emit thumbnailReady(path, cache_[path].thumb);
        return;
    }
    for (const auto& r : thumbPending_) if (r.path == path) return;
    thumbPending_.enqueue({path, size});
    processNextThumb();
}

void MediaProber::processNextProbe()
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
    cache_[currentPath_].info = seed;

    probePlayer_->setSource(QUrl::fromLocalFile(currentPath_));
}

void MediaProber::onDurationChanged(qint64 dur)
{
    if (!probing_ || currentPath_.isEmpty()) return;
    if (dur > 0) cache_[currentPath_].info.durationMs = dur;
}

void MediaProber::onTracksChanged()
{
    if (!probing_ || currentPath_.isEmpty()) return;
    const auto videoTracks = probePlayer_->videoTracks();
    if (!videoTracks.isEmpty()) {
        QSize res = videoTracks.first().value(QMediaMetaData::Resolution).toSize();
        if (!res.isEmpty()) {
            cache_[currentPath_].info.width = res.width();
            cache_[currentPath_].info.height = res.height();
        }
    }
    const auto fpsVal = probePlayer_->metaData().value(QMediaMetaData::VideoFrameRate);
    if (fpsVal.isValid()) {
        bool ok = false;
        double f = fpsVal.toDouble(&ok);
        if (ok && f > 0) cache_[currentPath_].info.fps = f;
    }
}

void MediaProber::onMediaStatusChanged(int status)
{
    if (!probing_ || currentPath_.isEmpty()) return;
    auto st = static_cast<QMediaPlayer::MediaStatus>(status);
    if (st == QMediaPlayer::LoadedMedia ||
        st == QMediaPlayer::EndOfMedia  ||
        st == QMediaPlayer::InvalidMedia) {
        Info i = cache_[currentPath_].info;
        i.probed = true;
        cache_[currentPath_].info = i;
        finishCurrentProbe(i);
    }
}

void MediaProber::finishCurrentProbe(const Info& info)
{
    const QString p = currentPath_;
    currentPath_.clear();
    probing_ = false;
    emit probed(p, info);
    processNextProbe();
}

void MediaProber::processNextThumb()
{
    if (thumbWorking_ || thumbPending_.isEmpty()) return;
    thumbWorking_ = true;
    ThumbReq req = thumbPending_.dequeue();
    thumbCurrentPath_ = req.path;
    thumbCurrentSize_ = req.size;

    if (isImageFile(req.path)) {
        makeImageThumbnail(req.path, req.size);
    } else if (isVideoFile(req.path)) {
        makeVideoThumbnail(req.path, req.size);
    } else if (isAudioFile(req.path)) {
        makeAudioWaveform(req.path, req.size);
    } else {
        // Unknown — produce a placeholder
        QPixmap pix(req.size);
        pix.fill(QColor(40, 40, 44));
        finalizeThumbnail(req.path, pix);
        thumbWorking_ = false;
        processNextThumb();
    }
}

void MediaProber::makeImageThumbnail(const QString& path, QSize size)
{
    QImageReader reader(path);
    reader.setScaledSize(size);
    QImage img = reader.read();
    QPixmap pix;
    if (!img.isNull()) {
        pix = QPixmap::fromImage(img);
    } else {
        pix = QPixmap(size);
        pix.fill(QColor(40, 40, 44));
    }
    finalizeThumbnail(path, pix);
    thumbWorking_ = false;
    processNextThumb();
}

void MediaProber::makeVideoThumbnail(const QString& path, QSize /*size*/)
{
    // Load the file, wait for first frame, capture, stop.
    thumbPlayer_->setSource(QUrl::fromLocalFile(path));
    thumbPlayer_->play();
    // The videoFrameChanged slot will catch the first frame and
    // finalize the thumbnail.
}

void MediaProber::makeAudioWaveform(const QString& path, QSize /*size*/)
{
    audioPeaks_.clear();
    audioDecoder_->setSource(QUrl::fromLocalFile(path));
    audioDecoder_->start();
}

void MediaProber::onVideoFrameChanged(const QVideoFrame& frame)
{
    if (!thumbWorking_ || thumbCurrentPath_.isEmpty()) return;
    if (!frame.isValid()) return;

    QImage img = frame.toImage();
    if (img.isNull()) return;

    // Scale to requested size
    QImage scaled = img.scaled(thumbCurrentSize_,
                                Qt::KeepAspectRatioByExpanding,
                                Qt::SmoothTransformation);
    // Crop to exact size
    int x = (scaled.width()  - thumbCurrentSize_.width())  / 2;
    int y = (scaled.height() - thumbCurrentSize_.height()) / 2;
    QImage cropped = scaled.copy(x, y, thumbCurrentSize_.width(), thumbCurrentSize_.height());

    QPixmap pix = QPixmap::fromImage(cropped);
    thumbPlayer_->stop();
    finalizeThumbnail(thumbCurrentPath_, pix);
    thumbWorking_ = false;
    processNextThumb();
}

void MediaProber::finalizeThumbnail(const QString& path, const QPixmap& pix)
{
    cache_[path].thumb = pix;
    cache_[path].hasThumb = true;
    emit thumbnailReady(path, pix);
}

} // namespace beta
