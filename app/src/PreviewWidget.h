#pragma once

#include <QWidget>

class QMediaPlayer;
class QVideoWidget;
class QLabel;
class QPushButton;
class QSlider;

namespace beta {

/// Center-top preview panel. Uses QtMultimedia to play back video /
/// audio, and shows a static thumbnail for image files.
class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget* parent = nullptr);

public slots:
    void loadMedia(const QString& path, const QString& name);
    void togglePlayPause();
    void stop();

signals:
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);

private slots:
    void onPositionChanged(qint64 pos);
    void onDurationChanged(qint64 dur);
    void onSliderMoved(int value);

private:
    void setupUi();

    QMediaPlayer* player_   = nullptr;
    QVideoWidget* video_    = nullptr;
    QLabel*       status_   = nullptr;
    QSlider*      seeker_   = nullptr;
    QPushButton*  playBtn_  = nullptr;
};

} // namespace beta
