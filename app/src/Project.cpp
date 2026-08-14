#include "Project.h"
#include "Commands.h"
#include "EngineBridge.h"
#include "MediaProber.h"
#include "TimelineWidget.h"
#include "UndoHelper.h"

namespace beta {

Project::Project(EngineBridge* engine, MediaProber* prober, QObject* parent)
    : QObject(parent)
    , engine_(engine)
    , prober_(prober)
    , undoStack_(std::make_unique<QUndoStack>(this))
{
    projectId_ = engine_->createProject("Untitled Project");
    engine_->addTrack(projectId_, EngineBridge::Video, "Video 1");
    engine_->addTrack(projectId_, EngineBridge::Audio, "Audio 1");

    connect(undoStack_.get(), &QUndoStack::cleanChanged,
            this, [this](bool) { emit mediaBinChanged(); });
}

void Project::importMedia(const QString& path)
{
    if (mediaBin_.contains(path)) return;
    mediaBin_.append(path);
    if (prober_) {
        prober_->probeAsync(path);
        prober_->requestThumbnail(path, QSize(160, 90));
    }
    emit mediaBinChanged();
}

void Project::addClip(uint64_t trackId, const QString& path, const QString& name,
                      uint64_t startFrame, uint64_t durationFrames)
{
    // Wire the timeline's undo stack pointer through so TimelineFunctions
    // can push onto it.
    if (timeline_) timeline_->setUndoStack(undoStack_.get());
    TimelineFunctions::addClip(engine_, projectId_, trackId, path, name,
                                startFrame, durationFrames, timeline_);
}

void Project::moveClip(uint64_t trackId, uint64_t clipId,
                       uint64_t oldStart, uint64_t newStart)
{
    if (timeline_) timeline_->setUndoStack(undoStack_.get());
    TimelineFunctions::moveClip(engine_, projectId_, trackId, clipId,
                                 oldStart, newStart, timeline_);
}

void Project::trimClip(uint64_t trackId, uint64_t clipId,
                       uint64_t oldStart, uint64_t oldDuration, uint64_t oldTrimIn,
                       uint64_t newStart, uint64_t newDuration, uint64_t newTrimIn)
{
    if (timeline_) timeline_->setUndoStack(undoStack_.get());
    TimelineFunctions::trimClip(engine_, projectId_, trackId, clipId,
                                 oldStart, oldDuration, oldTrimIn,
                                 newStart, newDuration, newTrimIn, timeline_);
}

void Project::splitClip(uint64_t trackId, uint64_t clipId, uint64_t splitFrame)
{
    if (timeline_) timeline_->setUndoStack(undoStack_.get());
    TimelineFunctions::splitClip(engine_, projectId_, trackId, clipId,
                                  splitFrame, timeline_);
}

void Project::mergeClips(uint64_t trackId, uint64_t leftClipId, uint64_t rightClipId)
{
    if (timeline_) timeline_->setUndoStack(undoStack_.get());
    TimelineFunctions::mergeClips(engine_, projectId_, trackId,
                                   leftClipId, rightClipId, timeline_);
}

void Project::removeClip(uint64_t trackId, uint64_t clipId)
{
    if (timeline_) timeline_->setUndoStack(undoStack_.get());
    TimelineFunctions::removeClip(engine_, projectId_, trackId, clipId, timeline_);
}

void Project::setAdjust(uint64_t trackId, uint64_t clipId,
                        const EngineBridge::ClipAdjust& newAdjust)
{
    // Snapshot old adjust
    auto snap = engine_->snapshot(projectId_);
    EngineBridge::ClipAdjust oldAdjust;
    bool found = false;
    for (const auto& t : snap.tracks) {
        if (t.id == trackId) {
            for (const auto& c : t.clips) {
                if (c.id == clipId) {
                    oldAdjust = c.adjust;
                    found = true;
                    break;
                }
            }
        }
    }
    if (!found) return;
    if (timeline_) timeline_->setUndoStack(undoStack_.get());
    TimelineFunctions::setAdjust(engine_, projectId_, trackId, clipId,
                                  oldAdjust, newAdjust, timeline_);
}

void Project::setTrackState(uint64_t trackId, const EngineBridge::TrackState& newState)
{
    auto oldState = engine_->trackState(projectId_, trackId);
    if (timeline_) timeline_->setUndoStack(undoStack_.get());
    TimelineFunctions::setTrackState(engine_, projectId_, trackId,
                                      oldState, newState, timeline_);
}

} // namespace beta
