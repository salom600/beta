#pragma once

#include <QWidget>

class QMediaPlayer;
class QVideoWidget;
class QLabel;
class QPushButton;
class QSlider;

namespace beta {

class MediaProber;

/// Center-top preview panel. Uses QtMultimedia to play back video /
/// audio, and shows a static thumbnail for image files.
///
/// The preview widget itself is also a drag source — clicking and
/// dragging on the video area starts a drag that drops the current
/// media onto a timeline track.
class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget* parent = nullptr);

    void setProber(MediaProber* prober);

    QString currentPath() const { return currentPath_; }
    QString currentName() const { return currentName_; }

public slots:
    void loadMedia(const QString& path, const QString& name);
    void togglePlayPause();
    void stop();

signals:
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private slots:
    void onPositionChanged(qint64 pos);
    void onDurationChanged(qint64 dur);
    void onSliderMoved(int value);
    void onPlayerStateChanged(int state);

private:
    void setupUi();
    void formatTimeLabel(qint64 ms);
    void paintImagePreview(const QString& path);

    QMediaPlayer* player_   = nullptr;
    QVideoWidget* video_    = nullptr;
    QLabel*       status_   = nullptr;
    QLabel*       timeLabel_ = nullptr;
    QSlider*      seeker_   = nullptr;
    QPushButton*  playBtn_  = nullptr;
    MediaProber*  prober_   = nullptr;

    QString currentPath_;
    QString currentName_;
    bool    isImage_ = false;

    // Drag state
    QPoint  dragStartPos_;
    bool    maybeDragging_ = false;
};

} // namespace beta
