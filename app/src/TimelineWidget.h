#pragma once

#include <QList>
#include <QMap>
#include <QWidget>
#include <cstdint>

namespace beta {

class EngineBridge;

/// Bottom panel: timeline with multiple tracks, a playback playhead,
/// per-track lock / eye / mute controls, and clip blocks rendered
/// manually with `paintEvent`.
class TimelineWidget : public QWidget {
    Q_OBJECT
public:
    struct TrackRow {
        uint64_t id;
        int      kind;          // 0=video, 1=audio, 2=image
        QString  name;
        bool     visible = true;
        bool     locked  = false;
        bool     muted   = false;
        // Local UI clip list (mirrors engine for fast paint)
        struct Clip {
            uint64_t id;
            QString  name;
            uint64_t startFrame;
            uint64_t durationFrames;
        };
        QList<Clip> clips;
    };

    TimelineWidget(QWidget* parent, EngineBridge* engine, uint64_t projectId);

    int trackCount() const { return tracks_.size(); }
    void refreshTracks();

    void setProjectId(uint64_t id);

signals:
    void clipSelected(const QString& name, const QString& path,
                      uint64_t start, uint64_t duration);

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private slots:
    void onPlayheadTimer();

private:
    void setupUi();
    void addClipToFirstTrack(const QString& path, const QString& name, int kindHint);
    int  rowAt(int y) const;
    int  frameAt(int x) const;
    int  xForFrame(uint64_t frame) const;
    QRect trackHeaderRect(int row) const;
    QRect trackBodyRect(int row) const;
    void  toggleVisible(uint64_t trackId);
    void  toggleMute(uint64_t trackId);
    void  toggleLock(uint64_t trackId);

    EngineBridge* engine_;
    uint64_t      projectId_;

    QList<TrackRow> tracks_;

    // Layout constants
    int headerWidth_   = 180;
    int rowHeight_     = 64;
    int pixelsPerFrame_ = 4;
    int rulerHeight_   = 28;

    // Playhead
    uint64_t playheadFrame_ = 0;
    bool     playing_       = false;
    class QTimer* playheadTimer_ = nullptr;

    // Drag state
    bool   draggingPlayhead_ = false;

    // friend MediaBrowser hookup
    friend class MainWindow;
};

} // namespace beta
