#pragma once

#include <QList>
#include <QMap>
#include <QWidget>
#include <cstdint>
#include <utility>
#include "EngineBridge.h"

namespace beta {

class MediaProber;

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
        struct Clip {
            uint64_t id;
            QString  name;
            QString  path;
            uint64_t startFrame;
            uint64_t durationFrames;
            uint64_t trimInFrames;
            EngineBridge::ClipAdjust adjust;
            int      mediaWidth  = 0;
            int      mediaHeight = 0;
            uint64_t mediaDurationFrames = 0;
        };
        QList<Clip> clips;
    };

    TimelineWidget(QWidget* parent, EngineBridge* engine, uint64_t projectId);

    void setProber(MediaProber* prober);

    int trackCount() const { return tracks_.size(); }
    void refreshTracks();
    void setProjectId(uint64_t id);

    /// Split the currently selected clip at the playhead.
    /// Returns true if a split actually happened.
    bool splitAtPlayhead();

    /// Delete the currently selected clip.
    bool deleteSelectedClip();

    /// Merge the currently selected clip with the next clip on the
    /// same track (if adjacent and from the same source).
    bool mergeSelectedWithNext();

    /// Returns the snapshot of tracks (used by exporter).
    QList<TrackRow> tracks() const { return tracks_; }

    /// Returns the playhead frame.
    uint64_t playheadFrame() const { return playheadFrame_; }
    void setPlayheadFrame(uint64_t f) { playheadFrame_ = f; update(); emit playheadMoved(f); }

signals:
    void clipSelected(const QString& name, const QString& path,
                      uint64_t start, uint64_t duration,
                      uint64_t trimIn,
                      const EngineBridge::ClipAdjust& adjust,
                      uint64_t trackId, uint64_t clipId);
    void playheadMoved(uint64_t frame);
    /// Emitted whenever the timeline contents change (clip add/move/
    /// trim/split/merge/delete) so external panels can refresh.
    void timelineChanged();

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

    /// Find the clip currently under the playhead on the selected track.
    /// Returns row + clip index, or {-1, -1} if none.
    std::pair<int, int> clipUnderPlayhead() const;

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

    // Selected clip (for split/cut/merge)
    int selectedRow_    = -1;
    int selectedClipIdx_ = -1;
};

} // namespace beta
