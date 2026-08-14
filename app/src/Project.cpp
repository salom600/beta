#include "Project.h"
#include "Commands.h"
#include "EngineBridge.h"
#include "MediaProber.h"
#include "TimelineWidget.h"

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
    auto* cmd = new AddClipCmd(engine_, projectId_, trackId, path, name,
                                startFrame, durationFrames, timeline_);
    undoStack_->push(cmd);
}

void Project::moveClip(uint64_t trackId, uint64_t clipId,
                       uint64_t oldStart, uint64_t newStart)
{
    auto* cmd = new MoveClipCmd(engine_, projectId_, trackId, clipId,
                                 oldStart, newStart, timeline_);
    undoStack_->push(cmd);
}

void Project::trimClip(uint64_t trackId, uint64_t clipId,
                       uint64_t oldStart, uint64_t oldDuration, uint64_t oldTrimIn,
                       uint64_t newStart, uint64_t newDuration, uint64_t newTrimIn)
{
    auto* cmd = new TrimClipCmd(engine_, projectId_, trackId, clipId,
                                 oldStart, oldDuration, oldTrimIn,
                                 newStart, newDuration, newTrimIn, timeline_);
    undoStack_->push(cmd);
}

void Project::splitClip(uint64_t trackId, uint64_t clipId, uint64_t splitFrame)
{
    auto* cmd = new SplitClipCmd(engine_, projectId_, trackId, clipId,
                                  splitFrame, timeline_);
    undoStack_->push(cmd);
}

void Project::mergeClips(uint64_t trackId, uint64_t leftClipId, uint64_t rightClipId)
{
    // Snapshot left's current duration before merging
    auto snap = engine_->snapshot(projectId_);
    uint64_t leftDur = 0;
    for (const auto& t : snap.tracks) {
        if (t.id == trackId) {
            for (const auto& c : t.clips) {
                if (c.id == leftClipId) leftDur = c.durationFrames;
            }
        }
    }
    auto* cmd = new MergeClipsCmd(engine_, projectId_, trackId,
                                   leftClipId, rightClipId, leftDur, timeline_);
    undoStack_->push(cmd);
}

void Project::removeClip(uint64_t trackId, uint64_t clipId)
{
    // Snapshot the clip's state before pushing the command
    auto snap = engine_->snapshot(projectId_);
    QString path, name;
    uint64_t startFrame = 0, duration = 0, trimIn = 0;
    bool found = false;
    for (const auto& t : snap.tracks) {
        if (t.id == trackId) {
            for (const auto& c : t.clips) {
                if (c.id == clipId) {
                    path = c.mediaPath;
                    name = c.mediaName;
                    startFrame = c.startFrame;
                    duration = c.durationFrames;
                    trimIn = c.trimInFrames;
                    found = true;
                    break;
                }
            }
        }
    }
    if (!found) return;
    auto* cmd = new RemoveClipCmd(engine_, projectId_, trackId, clipId,
                                   path, name, startFrame, duration, trimIn,
                                   timeline_);
    undoStack_->push(cmd);
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
    auto* cmd = new SetAdjustCmd(engine_, projectId_, trackId, clipId,
                                  oldAdjust, newAdjust, timeline_);
    undoStack_->push(cmd);
}

void Project::setTrackState(uint64_t trackId, const EngineBridge::TrackState& newState)
{
    auto oldState = engine_->trackState(projectId_, trackId);
    auto* cmd = new SetTrackStateCmd(engine_, projectId_, trackId,
                                      oldState, newState, timeline_);
    undoStack_->push(cmd);
}

} // namespace beta
