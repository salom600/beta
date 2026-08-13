#include "PreviewWidget.h"
#include "MediaProber.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaPlayer>
#include <QPushButton>
#include <QSlider>
#include <QStyle>
#include <QVideoWidget>
#include <QVBoxLayout>
#include <QFrame>

namespace beta {

PreviewWidget::PreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void PreviewWidget::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // Video frame with rounded border
    auto* frame = new QFrame(this);
    frame->setObjectName("panel");
    frame->setStyleSheet(
        "#panel { background: #000; border-radius: 6px; }");
    auto* frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);

    video_ = new QVideoWidget(frame);
    video_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    video_->setMinimumHeight(300);
    frameLayout->addWidget(video_);
    layout->addWidget(frame, 1);

    player_ = new QMediaPlayer(this);
    player_->setVideoOutput(video_);

    connect(player_, &QMediaPlayer::positionChanged,
            this, &PreviewWidget::onPositionChanged);
    connect(player_, &QMediaPlayer::durationChanged,
            this, &PreviewWidget::onDurationChanged);
    connect(player_, &QMediaPlayer::playbackStateChanged,
            this, &PreviewWidget::onPlayerStateChanged);

    // Transport bar
    auto* bar = new QHBoxLayout;
    bar->setSpacing(8);

    playBtn_ = new QPushButton(QIcon(":/icons/play.svg"), QString(), this);
    playBtn_->setToolTip(tr("Play / Pause (Space)"));
    playBtn_->setFixedSize(36, 36);
    playBtn_->setIconSize(QSize(18, 18));
    connect(playBtn_, &QPushButton::clicked, this, &PreviewWidget::togglePlayPause);

    auto* stopBtn = new QPushButton(QIcon(":/icons/stop.svg"), QString(), this);
    stopBtn->setToolTip(tr("Stop"));
    stopBtn->setFixedSize(36, 36);
    stopBtn->setIconSize(QSize(18, 18));
    connect(stopBtn, &QPushButton::clicked, this, &PreviewWidget::stop);

    seeker_ = new QSlider(Qt::Horizontal, this);
    seeker_->setRange(0, 0);
    connect(seeker_, &QSlider::sliderMoved, this, &PreviewWidget::onSliderMoved);

    timeLabel_ = new QLabel("00:00 / 00:00", this);
    timeLabel_->setObjectName("hintLabel");
    timeLabel_->setMinimumWidth(120);
    timeLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    bar->addWidget(playBtn_);
    bar->addWidget(stopBtn);
    bar->addWidget(seeker_, 1);
    bar->addWidget(timeLabel_);
    layout->addLayout(bar);

    status_ = new QLabel(tr("No media loaded"), this);
    status_->setObjectName("hintLabel");
    status_->setContentsMargins(2, 0, 0, 0);
    layout->addWidget(status_);
}

void PreviewWidget::setProber(MediaProber* prober) { prober_ = prober; }

void PreviewWidget::loadMedia(const QString& path, const QString& name)
{
    if (path.isEmpty()) return;
    QFileInfo info(path);
    QString suffix = info.suffix().toLower();

    if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
        suffix == "bmp" || suffix == "gif" || suffix == "webp") {
        player_->stop();
        status_->setText(tr("Image: %1").arg(name));
        formatTimeLabel(0);
        return;
    }

    player_->setSource(QUrl::fromLocalFile(path));
    status_->setText(tr("Loaded: %1").arg(name));
    player_->play();
}

void PreviewWidget::togglePlayPause()
{
    if (player_->playbackState() == QMediaPlayer::PlayingState) {
        player_->pause();
    } else {
        player_->play();
    }
}

void PreviewWidget::stop()
{
    player_->stop();
}

void PreviewWidget::onPositionChanged(qint64 pos)
{
    seeker_->blockSignals(true);
    seeker_->setValue(static_cast<int>(pos));
    seeker_->blockSignals(false);
    formatTimeLabel(pos);
    emit positionChanged(pos);
}

void PreviewWidget::onDurationChanged(qint64 dur)
{
    seeker_->setRange(0, static_cast<int>(dur));
    formatTimeLabel(player_->position());
    emit durationChanged(dur);
}

void PreviewWidget::onSliderMoved(int value)
{
    player_->setPosition(static_cast<qint64>(value));
}

void PreviewWidget::onPlayerStateChanged(int state)
{
    auto s = static_cast<QMediaPlayer::PlaybackState>(state);
    if (s == QMediaPlayer::PlayingState) {
        playBtn_->setIcon(QIcon(":/icons/pause.svg"));
    } else {
        playBtn_->setIcon(QIcon(":/icons/play.svg"));
    }
}

void PreviewWidget::formatTimeLabel(qint64 pos)
{
    qint64 dur = player_->duration();
    auto fmt = [](qint64 ms) {
        qint64 s = ms / 1000;
        return QString::asprintf("%02lld:%02lld", s / 60, s % 60);
    };
    timeLabel_->setText(QString("%1 / %2").arg(fmt(pos), fmt(dur)));
}

} // namespace beta
