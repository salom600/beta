#pragma once

#include <QList>
#include <QWidget>
#include <cstdint>
#include <utility>
#include <memory>
#include "EngineBridge.h"
#include "Tool.h"
#include "SnapModel.h"

class QUndoStack;

namespace beta {

class MediaProber;
class Project;

/// Bottom panel: timeline with multiple tracks, playback playhead,
/// per-track lock/eye/mute controls, and clip blocks rendered with
/// `paintEvent`.
///
/// v0.4 redesign:
///   • Tool-based interaction (Select / Razor / Spacer / Hand)
///   • Snapping (playhead, clip edges, ruler marks)
///   • Clip thumbnails for video, waveform overlay for audio
///   • Routes all edits through Project's undo stack
class TimelineWidget : public QWidget {
    Q_OBJECT
public:
    struct TrackRow {
        uint64_t id;
        int      kind;
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
    void setProject(Project* project);

    int trackCount() const { return tracks_.size(); }
    void refreshTracks();
    void setProjectId(uint64_t id);

    bool splitAtPlayhead();
    bool deleteSelectedClip();
    bool mergeSelectedWithNext();

    /// Snap a frame value to the nearest of: playhead, any clip edge,
    /// or ruler second mark — within `threshold` frames.
    uint64_t snapFrame(uint64_t frame, int threshold = 4) const;

    /// Move playhead to the start of the next cut after the current
    /// playhead position.
    void jumpToNextCut();
    void jumpToPrevCut();

    QList<TrackRow> tracks() const { return tracks_; }
    uint64_t playheadFrame() const { return playheadFrame_; }
    void setPlayheadFrame(uint64_t f) { playheadFrame_ = f; update(); emit playheadMoved(f); }

    void setTool(Tool::Kind t) { tool_ = t; update(); }
    Tool::Kind tool() const { return tool_; }

    void setZoom(int pixelsPerFrame);
    int  zoom() const { return pixelsPerFrame_; }

    int  totalHeight() const;

    /// Exposed so Commands.cpp can push FunctionalUndoCommand onto the
    /// Project's undo stack.
    class QUndoStack* undoStack() const { return undoStack_; }
    void setUndoStack(QUndoStack* s) { undoStack_ = s; }

signals:
    void clipSelected(const QString& name, const QString& path,
                      uint64_t start, uint64_t duration,
                      uint64_t trimIn,
                      const EngineBridge::ClipAdjust& adjust,
                      uint64_t trackId, uint64_t clipId);
    void playheadMoved(uint64_t frame);
    void timelineChanged();
    void toolChanged(Tool::Kind t);

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

    /// Find the clip at the given point. Returns {row, clipIdx} or {-1,-1}.
    std::pair<int, int> clipAt(const QPoint& pos) const;

    /// Find the clip currently under the playhead on any track.
    std::pair<int, int> clipUnderPlayhead() const;

    enum class DragMode { None, ScrubPlayhead, MoveClip, TrimClipLeft, TrimClipRight, Pan };
    DragMode hitTest(const QPoint& pos, int* outRow, int* outClipIdx) const;

    void handleSelectPress(const QPoint& pos);
    void handleRazorPress(const QPoint& pos);
    void handleSpacerPress(const QPoint& pos);
    void handleHandPress(const QPoint& pos);

    EngineBridge* engine_;
    MediaProber*  prober_ = nullptr;
    Project*      project_ = nullptr;
    uint64_t      projectId_;

    QList<TrackRow> tracks_;

    // Layout constants
    int headerWidth_   = 220;
    int rowHeight_     = 72;
    int rulerHeight_   = 32;
    int pixelsPerFrame_ = 4;
    int trimHandleWidth_ = 8;
    int scrollY_ = 0;

    // Playhead
    uint64_t playheadFrame_ = 0;
    class QTimer* playheadTimer_ = nullptr;

    // Tool
    Tool::Kind tool_ = Tool::SelectTool;

    // Drag state
    DragMode  dragMode_   = DragMode::None;
    int       dragRow_    = -1;
    int       dragClipIdx_ = -1;
    int       dragStartFrame_ = 0;
    uint64_t  dragOrigStart_  = 0;
    uint64_t  dragOrigDuration_ = 0;
    uint64_t  dragOrigTrimIn_ = 0;
    QPoint    dragAnchor_;
    QPoint    panAnchor_;
    uint64_t  dragNewStart_  = 0;
    uint64_t  dragNewDuration_ = 0;
    uint64_t  dragNewTrimIn_ = 0;
    bool      dragCommitted_ = false;

    // Selected clip
    int selectedRow_    = -1;
    int selectedClipIdx_ = -1;

    // Snap model (refcounted snap points)
    std::unique_ptr<SnapModel> snapModel_;

    // Undo stack (owned by Project; weak ref)
    QUndoStack* undoStack_ = nullptr;
};

} // namespace beta
