#pragma once

#include <QObject>
#include <QUndoStack>
#include <memory>
#include <cstdint>
#include "EngineBridge.h"

namespace beta {

class TimelineWidget;
class MediaProber;

/// Single source of truth for an open project. Owns:
///   • the engine project id
///   • the undo stack (every edit goes through here)
///   • the media bin (list of imported paths + probed info)
class Project : public QObject {
    Q_OBJECT
public:
    explicit Project(EngineBridge* engine, MediaProber* prober,
                     QObject* parent = nullptr);

    uint64_t id() const { return projectId_; }
    QUndoStack* undoStack() const { return undoStack_.get(); }
    EngineBridge* engine() const { return engine_; }
    MediaProber* prober() const { return prober_; }

    /// Bind a TimelineWidget so commands can refresh it after each
    /// undo/redo.
    void setTimeline(TimelineWidget* t) { timeline_ = t; }
    TimelineWidget* timeline() const { return timeline_; }

    double fps() const { return 30.0; }
    int width() const  { return 1920; }
    int height() const { return 1080; }

    /// Import media into the bin (no clip is added to the timeline yet).
    void importMedia(const QString& path);
    const QList<QString>& mediaBin() const { return mediaBin_; }

    /// Convenience: push an AddClip command onto the undo stack.
    void addClip(uint64_t trackId, const QString& path, const QString& name,
                 uint64_t startFrame, uint64_t durationFrames);

    /// Convenience: push a MoveClip command.
    void moveClip(uint64_t trackId, uint64_t clipId,
                  uint64_t oldStart, uint64_t newStart);

    /// Convenience: push a TrimClip command.
    void trimClip(uint64_t trackId, uint64_t clipId,
                  uint64_t oldStart, uint64_t oldDuration, uint64_t oldTrimIn,
                  uint64_t newStart, uint64_t newDuration, uint64_t newTrimIn);

    /// Convenience: push a SplitClip command.
    void splitClip(uint64_t trackId, uint64_t clipId, uint64_t splitFrame);

    /// Convenience: push a MergeClips command.
    void mergeClips(uint64_t trackId, uint64_t leftClipId, uint64_t rightClipId);

    /// Convenience: push a RemoveClip command (snapshots current state).
    void removeClip(uint64_t trackId, uint64_t clipId);

    /// Convenience: push a SetAdjust command (snapshots old adjust).
    void setAdjust(uint64_t trackId, uint64_t clipId,
                   const EngineBridge::ClipAdjust& newAdjust);

    /// Convenience: push a SetTrackState command.
    void setTrackState(uint64_t trackId,
                       const EngineBridge::TrackState& newState);

signals:
    void mediaBinChanged();

private:
    EngineBridge*   engine_;
    MediaProber*    prober_;
    uint64_t        projectId_ = 0;
    std::unique_ptr<QUndoStack> undoStack_;
    TimelineWidget* timeline_ = nullptr;
    QList<QString>  mediaBin_;
};

} // namespace beta
