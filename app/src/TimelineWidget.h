#pragma once

#include <QList>
#include <QMap>
#include <QWidget>
#include <cstdint>

namespace beta {

class EngineBridge;
class MediaProber;

/// Bottom panel: timeline with multiple tracks, a playback playhead,
/// per-track lock / eye / mute controls, and clip blocks rendered
/// manually with `paintEvent`.
///
/// Supports:
///   • Drop media from MediaBrowser onto a track to add a clip
///   • Click a clip's body to select it
///   • Drag a clip body to move it along the timeline
///   • Drag a clip's left/right edge (8 px) to trim
///   • Click on the ruler to scrub the playhead
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
        struct Clip {
            uint64_t id;
            QString  name;
            QString  path;
            uint64_t startFrame;
            uint64_t durationFrames;
            uint64_t trimInFrames;
            int      mediaWidth  = 0;
            int      mediaHeight = 0;
            uint64_t mediaDurationFrames = 0;
            double   volume   = 1.0;
            double   opacity  = 1.0;
            double   scale    = 1.0;
        };
        QList<Clip> clips;
    };

    TimelineWidget(QWidget* parent, EngineBridge* engine, uint64_t projectId);

    void setProber(MediaProber* prober);

    int trackCount() const { return tracks_.size(); }
    void refreshTracks();
    void setProjectId(uint64_t id);

    /// Returns the snapshot of tracks (used by exporter).
    QList<TrackRow> tracks() const { return tracks_; }

signals:
    void clipSelected(const QString& name, const QString& path,
                      uint64_t start, uint64_t duration,
                      uint64_t trimIn, double volume, double opacity, double scale,
                      uint64_t trackId, uint64_t clipId);
    void playheadMoved(uint64_t frame);

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private slots:
    void onPlayheadTimer();

private:
    void setupUi();
    void ensureScrollbarIfNeeded();
    int  rowAt(int y) const;
    int  frameAt(int x) const;
    int  xForFrame(uint64_t frame) const;
    QRect trackHeaderRect(int row) const;
    QRect trackBodyRect(int row) const;
    QRect clipRect(int row, const TrackRow::Clip& c) const;
    void  toggleVisible(uint64_t trackId);
    void  toggleMute(uint64_t trackId);
    void  toggleLock(uint64_t trackId);
    void  commitClipMove(int row, int clipIdx);
    void  commitClipTrim(int row, int clipIdx);

    enum class DragMode { None, ScrubPlayhead, MoveClip, TrimClipLeft, TrimClipRight };
    DragMode hitTest(const QPoint& pos, int* outRow, int* outClipIdx) const;

    EngineBridge* engine_;
    MediaProber*  prober_ = nullptr;
    uint64_t      projectId_;

    QList<TrackRow> tracks_;

    // Layout constants
    int headerWidth_   = 220;
    int rowHeight_     = 72;
    int rulerHeight_   = 32;
    int pixelsPerFrame_ = 4;
    int trimHandleWidth_ = 8;

    // Playhead
    uint64_t playheadFrame_ = 0;
    class QTimer* playheadTimer_ = nullptr;

    // Drag state
    DragMode  dragMode_   = DragMode::None;
    int       dragRow_    = -1;
    int       dragClipIdx_ = -1;
    int       dragStartFrame_ = 0;
    uint64_t  dragOrigStart_  = 0;
    uint64_t  dragOrigDuration_ = 0;
    uint64_t  dragOrigTrimIn_ = 0;
    QPoint    dragAnchor_;
    uint64_t  dragNewStart_  = 0;
    uint64_t  dragNewDuration_ = 0;
    uint64_t  dragNewTrimIn_ = 0;
};

} // namespace beta
