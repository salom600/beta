#include "PreviewWidget.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaPlayer>
#include <QPushButton>
#include <QSlider>
#include <QStyle>
#include <QVideoWidget>
#include <QVBoxLayout>

namespace beta {

PreviewWidget::PreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void PreviewWidget::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    video_ = new QVideoWidget(this);
    video_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    video_->setMinimumHeight(280);
    video_->setStyleSheet("background: #000;");
    layout->addWidget(video_, 1);

    player_ = new QMediaPlayer(this);
    player_->setVideoOutput(video_);

    connect(player_, &QMediaPlayer::positionChanged,
            this, &PreviewWidget::onPositionChanged);
    connect(player_, &QMediaPlayer::durationChanged,
            this, &PreviewWidget::onDurationChanged);

    // Transport bar
    auto* bar = new QHBoxLayout;
    playBtn_ = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay), QString(), this);
    playBtn_->setToolTip(tr("Play / Pause"));
    connect(playBtn_, &QPushButton::clicked, this, &PreviewWidget::togglePlayPause);

    auto* stopBtn = new QPushButton(style()->standardIcon(QStyle::SP_MediaStop), QString(), this);
    stopBtn->setToolTip(tr("Stop"));
    connect(stopBtn, &QPushButton::clicked, this, &PreviewWidget::stop);

    seeker_ = new QSlider(Qt::Horizontal, this);
    seeker_->setRange(0, 0);
    connect(seeker_, &QSlider::sliderMoved, this, &PreviewWidget::onSliderMoved);

    status_ = new QLabel(tr("No media loaded"), this);
    status_->setStyleSheet("padding: 4px 8px; color: #888;");

    bar->addWidget(playBtn_);
    bar->addWidget(stopBtn);
    bar->addWidget(seeker_, 1);
    bar->addStretch();
    bar->addWidget(status_);
    layout->addLayout(bar);
}

void PreviewWidget::loadMedia(const QString& path, const QString& name)
{
    if (path.isEmpty()) return;
    QFileInfo info(path);
    QString suffix = info.suffix().toLower();

    if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
        suffix == "bmp" || suffix == "gif" || suffix == "webp") {
        // For images, the QVideoWidget can't render directly. We use
        // the player only for audio/video. For images we just show
        // status text — the timeline will render them as clips.
        player_->stop();
        status_->setText(tr("Image: %1").arg(name));
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
        playBtn_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    } else {
        player_->play();
        playBtn_->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    }
}

void PreviewWidget::stop()
{
    player_->stop();
    playBtn_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
}

void PreviewWidget::onPositionChanged(qint64 pos)
{
    seeker_->blockSignals(true);
    seeker_->setValue(static_cast<int>(pos));
    seeker_->blockSignals(false);
    emit positionChanged(pos);
}

void PreviewWidget::onDurationChanged(qint64 dur)
{
    seeker_->setRange(0, static_cast<int>(dur));
    emit durationChanged(dur);
}

void PreviewWidget::onSliderMoved(int value)
{
    player_->setPosition(static_cast<qint64>(value));
}

} // namespace beta
