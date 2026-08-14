#include "Commands.h"
#include "EngineBridge.h"
#include "TimelineWidget.h"

namespace beta {

void TimelineCommand::refresh() const
{
    if (timeline_) timeline_->refreshTracks();
}

// ---- AddClipCmd ------------------------------------------------------

AddClipCmd::AddClipCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                       const QString& path, const QString& name,
                       uint64_t startFrame, uint64_t durationFrames,
                       TimelineWidget* timeline, QUndoCommand* parent)
    : TimelineCommand(engine, projectId, timeline, parent)
    , trackId_(trackId), path_(path), name_(name)
    , startFrame_(startFrame), durationFrames_(durationFrames)
{
    setText(QObject::tr("Add clip"));
}

void AddClipCmd::redo()
{
    if (firstRun_) {
        clipId_ = engine_->addClip(projectId_, trackId_, path_, name_,
                                    startFrame_, durationFrames_);
        firstRun_ = false;
    } else {
        // Re-add with the same id (engine assigns a new one — for simplicity
        // we accept that the id changes on redo; this is good enough for v0.4)
        clipId_ = engine_->addClip(projectId_, trackId_, path_, name_,
                                    startFrame_, durationFrames_);
    }
    refresh();
}

void AddClipCmd::undo()
{
    if (clipId_ != 0) {
        engine_->removeClip(projectId_, trackId_, clipId_);
        clipId_ = 0;
    }
    refresh();
}

// ---- RemoveClipCmd ---------------------------------------------------

RemoveClipCmd::RemoveClipCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                             uint64_t clipId, const QString& path, const QString& name,
                             uint64_t startFrame, uint64_t durationFrames, uint64_t trimIn,
                             TimelineWidget* timeline, QUndoCommand* parent)
    : TimelineCommand(engine, projectId, timeline, parent)
    , trackId_(trackId), clipId_(clipId), path_(path), name_(name)
    , startFrame_(startFrame), durationFrames_(durationFrames), trimIn_(trimIn)
{
    setText(QObject::tr("Remove clip"));
}

void RemoveClipCmd::redo()
{
    engine_->removeClip(projectId_, trackId_, clipId_);
    refresh();
}

void RemoveClipCmd::undo()
{
    // Re-create the clip with the same start/duration/trim. Note: the engine
    // will assign a NEW clip id, but the user-visible state is restored.
    uint64_t newId = engine_->addClip(projectId_, trackId_, path_, name_,
                                       startFrame_, durationFrames_);
    if (newId != 0) {
        engine_->trimClip(projectId_, trackId_, newId, trimIn_, durationFrames_);
        clipId_ = newId;
    }
    refresh();
}

// ---- MoveClipCmd -----------------------------------------------------

MoveClipCmd::MoveClipCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                         uint64_t clipId, uint64_t oldStart, uint64_t newStart,
                         TimelineWidget* timeline, QUndoCommand* parent)
    : TimelineCommand(engine, projectId, timeline, parent)
    , trackId_(trackId), clipId_(clipId), oldStart_(oldStart), newStart_(newStart)
{
    setText(QObject::tr("Move clip"));
}

void MoveClipCmd::redo()
{
    engine_->moveClip(projectId_, trackId_, clipId_, newStart_);
    refresh();
}

void MoveClipCmd::undo()
{
    engine_->moveClip(projectId_, trackId_, clipId_, oldStart_);
    refresh();
}

bool MoveClipCmd::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id()) return false;
    const auto* m = static_cast<const MoveClipCmd*>(other);
    if (m->trackId_ != trackId_ || m->clipId_ != clipId_) return false;
    newStart_ = m->newStart_;
    return true;
}

// ---- TrimClipCmd -----------------------------------------------------

TrimClipCmd::TrimClipCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                         uint64_t clipId,
                         uint64_t oldStart, uint64_t oldDuration, uint64_t oldTrimIn,
                         uint64_t newStart, uint64_t newDuration, uint64_t newTrimIn,
                         TimelineWidget* timeline, QUndoCommand* parent)
    : TimelineCommand(engine, projectId, timeline, parent)
    , trackId_(trackId), clipId_(clipId)
    , oldStart_(oldStart), oldDuration_(oldDuration), oldTrimIn_(oldTrimIn)
    , newStart_(newStart), newDuration_(newDuration), newTrimIn_(newTrimIn)
{
    setText(QObject::tr("Trim clip"));
}

void TrimClipCmd::redo()
{
    engine_->trimClip(projectId_, trackId_, clipId_, newTrimIn_, newDuration_);
    engine_->moveClip(projectId_, trackId_, clipId_, newStart_);
    refresh();
}

void TrimClipCmd::undo()
{
    engine_->trimClip(projectId_, trackId_, clipId_, oldTrimIn_, oldDuration_);
    engine_->moveClip(projectId_, trackId_, clipId_, oldStart_);
    refresh();
}

bool TrimClipCmd::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id()) return false;
    const auto* m = static_cast<const TrimClipCmd*>(other);
    if (m->trackId_ != trackId_ || m->clipId_ != clipId_) return false;
    newStart_     = m->newStart_;
    newDuration_  = m->newDuration_;
    newTrimIn_    = m->newTrimIn_;
    return true;
}

// ---- SplitClipCmd ----------------------------------------------------

SplitClipCmd::SplitClipCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                           uint64_t clipId, uint64_t splitFrame,
                           TimelineWidget* timeline, QUndoCommand* parent)
    : TimelineCommand(engine, projectId, timeline, parent)
    , trackId_(trackId), clipId_(clipId), splitFrame_(splitFrame)
{
    setText(QObject::tr("Split clip"));
}

void SplitClipCmd::redo()
{
    if (firstRun_) {
        // Snapshot original duration so we can restore it on undo
        auto snap = engine_->snapshot(projectId_);
        for (const auto& t : snap.tracks) {
            if (t.id == trackId_) {
                for (const auto& c : t.clips) {
                    if (c.id == clipId_) {
                        origDuration_ = c.durationFrames;
                        break;
                    }
                }
            }
        }
        newClipId_ = engine_->splitClip(projectId_, trackId_, clipId_, splitFrame_);
        firstRun_ = false;
    } else {
        // Re-split: original clip is now at origDuration_? No — on redo
        // after undo, the original clip is back to its full duration.
        // Re-splitting will produce a new right-hand clip.
        newClipId_ = engine_->splitClip(projectId_, trackId_, clipId_, splitFrame_);
    }
    refresh();
}

void SplitClipCmd::undo()
{
    // To undo a split: find the right-hand clip (newClipId_) and merge it
    // back into the left (clipId_).
    if (newClipId_ != 0) {
        engine_->mergeClips(projectId_, trackId_, clipId_, newClipId_);
        newClipId_ = 0;
    }
    refresh();
}

// ---- MergeClipsCmd ---------------------------------------------------

MergeClipsCmd::MergeClipsCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                             uint64_t leftClipId, uint64_t rightClipId,
                             uint64_t leftOrigDuration,
                             TimelineWidget* timeline, QUndoCommand* parent)
    : TimelineCommand(engine, projectId, timeline, parent)
    , trackId_(trackId), leftClipId_(leftClipId), rightClipId_(rightClipId)
    , leftOrigDuration_(leftOrigDuration)
{
    setText(QObject::tr("Merge clips"));
}

void MergeClipsCmd::redo()
{
    if (firstRun_) {
        // Snapshot the left clip's current duration so we can restore it
        auto snap = engine_->snapshot(projectId_);
        for (const auto& t : snap.tracks) {
            if (t.id == trackId_) {
                for (const auto& c : t.clips) {
                    if (c.id == leftClipId_) leftOrigDuration_ = c.durationFrames;
                }
            }
        }
        firstRun_ = false;
    }
    engine_->mergeClips(projectId_, trackId_, leftClipId_, rightClipId_);
    refresh();
}

void MergeClipsCmd::undo()
{
    // We can't perfectly unmerge without knowing the right clip's original
    // duration + trim. For v0.4, we re-add the right clip with the leftover
    // duration by extending the left clip back to its original length and
    // re-splitting at the boundary.
    // This is approximate — for a true undo we'd need to snapshot the full
    // right clip state. Acceptable for v0.4.
    auto snap = engine_->snapshot(projectId_);
    QString path, name;
    uint64_t startFrame = 0, duration = 0, trimIn = 0;
    for (const auto& t : snap.tracks) {
        if (t.id == trackId_) {
            for (const auto& c : t.clips) {
                if (c.id == leftClipId_) {
                    path = c.mediaPath;
                    name = c.mediaName;
                    startFrame = c.startFrame + c.durationFrames;
                    duration = c.durationFrames - leftOrigDuration_;
                    if (c.trimInFrames >= leftOrigDuration_)
                        trimIn = c.trimInFrames + leftOrigDuration_;
                    // Restore left clip's original duration
                    engine_->trimClip(projectId_, trackId_, leftClipId_,
                                      c.trimInFrames, leftOrigDuration_);
                    break;
                }
            }
        }
    }
    // Re-add the right clip
    if (!path.isEmpty() && duration > 0) {
        uint64_t newId = engine_->addClip(projectId_, trackId_, path, name,
                                           startFrame, duration);
        if (newId) engine_->trimClip(projectId_, trackId_, newId, trimIn, duration);
        rightClipId_ = newId;
    }
    refresh();
}

// ---- SetAdjustCmd ----------------------------------------------------

SetAdjustCmd::SetAdjustCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                           uint64_t clipId,
                           const EngineBridge::ClipAdjust& oldAdjust,
                           const EngineBridge::ClipAdjust& newAdjust,
                           TimelineWidget* timeline, QUndoCommand* parent)
    : TimelineCommand(engine, projectId, timeline, parent)
    , trackId_(trackId), clipId_(clipId)
    , oldAdjust_(oldAdjust), newAdjust_(newAdjust)
{
    setText(QObject::tr("Edit clip properties"));
}

void SetAdjustCmd::redo()
{
    engine_->setClipAdjust(projectId_, trackId_, clipId_, newAdjust_);
    refresh();
}

void SetAdjustCmd::undo()
{
    engine_->setClipAdjust(projectId_, trackId_, clipId_, oldAdjust_);
    refresh();
}

bool SetAdjustCmd::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id()) return false;
    const auto* m = static_cast<const SetAdjustCmd*>(other);
    if (m->trackId_ != trackId_ || m->clipId_ != clipId_) return false;
    newAdjust_ = m->newAdjust_;
    return true;
}

// ---- SetTrackStateCmd ------------------------------------------------

SetTrackStateCmd::SetTrackStateCmd(EngineBridge* engine, uint64_t projectId,
                                   uint64_t trackId,
                                   EngineBridge::TrackState oldState,
                                   EngineBridge::TrackState newState,
                                   TimelineWidget* timeline, QUndoCommand* parent)
    : TimelineCommand(engine, projectId, timeline, parent)
    , trackId_(trackId), oldState_(oldState), newState_(newState)
{
    setText(QObject::tr("Toggle track state"));
}

void SetTrackStateCmd::redo()
{
    engine_->setTrackState(projectId_, trackId_, newState_);
    refresh();
}

void SetTrackStateCmd::undo()
{
    engine_->setTrackState(projectId_, trackId_, oldState_);
    refresh();
}

} // namespace beta
