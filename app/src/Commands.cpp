#include "Commands.h"
#include "EngineBridge.h"
#include "TimelineWidget.h"

#include <QUndoStack>

namespace beta {
namespace TimelineFunctions {

namespace {

/// Push a `FunctionalUndoCommand` onto the Project's undo stack.
/// `engine` doesn't own the stack — Project does — so we accept the
/// stack externally. For now we route through a static helper.
void pushUndo(TimelineWidget* timeline, const Fun& undo, const Fun& redo,
              const QString& text)
{
    // The timeline doesn't own a QUndoStack directly; the Project does.
    // We rely on the timeline having a reference to it via setProject().
    // For now, use a global accessor: TimelineWidget::undoStack().
    // If null, fall back to executing redo eagerly (no undo).
    if (timeline && timeline->undoStack()) {
        timeline->undoStack()->push(new FunctionalUndoCommand(undo, redo, text));
    } else {
        // No stack — just run redo
        redo();
    }
}

} // namespace

bool addClip(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
             const QString& path, const QString& name,
             uint64_t startFrame, uint64_t durationFrames,
             TimelineWidget* timeline)
{
    // Eagerly perform the add
    uint64_t clipId = engine->addClip(projectId, trackId, path, name,
                                       startFrame, durationFrames);
    if (clipId == 0) return false;

    // Build matching undo/redo lambdas
    Fun undo = [engine, projectId, trackId, clipId]() -> bool {
        engine->removeClip(projectId, trackId, clipId);
        return true;
    };
    Fun redo = [engine, projectId, trackId, path, name, startFrame, durationFrames]() -> bool {
        // Note: this will assign a NEW clip id, but the user-visible
        // state is restored. This is acceptable for v0.5.
        engine->addClip(projectId, trackId, path, name, startFrame, durationFrames);
        return true;
    };

    if (timeline) timeline->refreshTracks();
    pushUndo(timeline, undo, redo, QObject::tr("Add clip"));
    return true;
}

bool removeClip(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                uint64_t clipId, TimelineWidget* timeline)
{
    // Snapshot the clip's state before removing
    auto snap = engine->snapshot(projectId);
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
    if (!found) return false;

    // Eagerly remove
    engine->removeClip(projectId, trackId, clipId);

    Fun undo = [engine, projectId, trackId, path, name, startFrame, duration, trimIn]() -> bool {
        uint64_t newId = engine->addClip(projectId, trackId, path, name, startFrame, duration);
        if (newId != 0) {
            engine->trimClip(projectId, trackId, newId, trimIn, duration);
        }
        return true;
    };
    Fun redo = [engine, projectId, trackId, clipId]() -> bool {
        engine->removeClip(projectId, trackId, clipId);
        return true;
    };

    if (timeline) timeline->refreshTracks();
    pushUndo(timeline, undo, redo, QObject::tr("Remove clip"));
    return true;
}

bool moveClip(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
              uint64_t clipId, uint64_t oldStart, uint64_t newStart,
              TimelineWidget* timeline)
{
    if (oldStart == newStart) return false;

    // Eagerly move
    engine->moveClip(projectId, trackId, clipId, newStart);

    Fun undo = [engine, projectId, trackId, clipId, oldStart]() -> bool {
        engine->moveClip(projectId, trackId, clipId, oldStart);
        return true;
    };
    Fun redo = [engine, projectId, trackId, clipId, newStart]() -> bool {
        engine->moveClip(projectId, trackId, clipId, newStart);
        return true;
    };

    if (timeline) timeline->refreshTracks();
    pushUndo(timeline, undo, redo, QObject::tr("Move clip"));
    return true;
}

bool trimClip(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
              uint64_t clipId,
              uint64_t oldStart, uint64_t oldDuration, uint64_t oldTrimIn,
              uint64_t newStart, uint64_t newDuration, uint64_t newTrimIn,
              TimelineWidget* timeline)
{
    // Eagerly trim + move
    engine->trimClip(projectId, trackId, clipId, newTrimIn, newDuration);
    engine->moveClip(projectId, trackId, clipId, newStart);

    Fun undo = [engine, projectId, trackId, clipId,
                oldStart, oldDuration, oldTrimIn]() -> bool {
        engine->trimClip(projectId, trackId, clipId, oldTrimIn, oldDuration);
        engine->moveClip(projectId, trackId, clipId, oldStart);
        return true;
    };
    Fun redo = [engine, projectId, trackId, clipId,
                newStart, newDuration, newTrimIn]() -> bool {
        engine->trimClip(projectId, trackId, clipId, newTrimIn, newDuration);
        engine->moveClip(projectId, trackId, clipId, newStart);
        return true;
    };

    if (timeline) timeline->refreshTracks();
    pushUndo(timeline, undo, redo, QObject::tr("Trim clip"));
    return true;
}

bool splitClip(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
               uint64_t clipId, uint64_t splitFrame,
               TimelineWidget* timeline)
{
    // Snapshot original duration
    auto snap = engine->snapshot(projectId);
    uint64_t origDuration = 0;
    bool found = false;
    for (const auto& t : snap.tracks) {
        if (t.id == trackId) {
            for (const auto& c : t.clips) {
                if (c.id == clipId) {
                    origDuration = c.durationFrames;
                    found = true;
                    break;
                }
            }
        }
    }
    if (!found) return false;

    // Eagerly split
    uint64_t newClipId = engine->splitClip(projectId, trackId, clipId, splitFrame);
    if (newClipId == 0) return false;

    // Undo = merge the new clip back into the original
    Fun undo = [engine, projectId, trackId, clipId, newClipId]() -> bool {
        engine->mergeClips(projectId, trackId, clipId, newClipId);
        return true;
    };
    // Redo = re-split (will produce a new right-hand clip id)
    Fun redo = [engine, projectId, trackId, clipId, splitFrame]() -> bool {
        engine->splitClip(projectId, trackId, clipId, splitFrame);
        return true;
    };

    if (timeline) timeline->refreshTracks();
    pushUndo(timeline, undo, redo, QObject::tr("Split clip"));
    return true;
}

bool mergeClips(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                uint64_t leftClipId, uint64_t rightClipId,
                TimelineWidget* timeline)
{
    // Snapshot left's duration before merging
    auto snap = engine->snapshot(projectId);
    uint64_t leftOrigDuration = 0;
    QString rightPath, rightName;
    uint64_t rightStart = 0, rightDuration = 0, rightTrimIn = 0;
    bool found = false;
    for (const auto& t : snap.tracks) {
        if (t.id == trackId) {
            for (const auto& c : t.clips) {
                if (c.id == leftClipId) leftOrigDuration = c.durationFrames;
                if (c.id == rightClipId) {
                    rightPath = c.mediaPath;
                    rightName = c.mediaName;
                    rightStart = c.startFrame;
                    rightDuration = c.durationFrames;
                    rightTrimIn = c.trimInFrames;
                    found = true;
                }
            }
        }
    }
    if (!found) return false;

    // Eagerly merge
    engine->mergeClips(projectId, trackId, leftClipId, rightClipId);

    // Undo = re-add the right clip and restore left's original duration
    Fun undo = [engine, projectId, trackId, leftClipId,
                leftOrigDuration, rightPath, rightName,
                rightStart, rightDuration, rightTrimIn]() -> bool {
        // Restore left's original duration
        engine->trimClip(projectId, trackId, leftClipId, 0, leftOrigDuration);
        // Re-add the right clip
        uint64_t newId = engine->addClip(projectId, trackId, rightPath, rightName,
                                          rightStart, rightDuration);
        if (newId != 0) {
            engine->trimClip(projectId, trackId, newId, rightTrimIn, rightDuration);
        }
        return true;
    };
    Fun redo = [engine, projectId, trackId, leftClipId, rightClipId]() -> bool {
        engine->mergeClips(projectId, trackId, leftClipId, rightClipId);
        return true;
    };

    if (timeline) timeline->refreshTracks();
    pushUndo(timeline, undo, redo, QObject::tr("Merge clips"));
    return true;
}

bool setAdjust(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
               uint64_t clipId,
               const EngineBridge::ClipAdjust& oldAdjust,
               const EngineBridge::ClipAdjust& newAdjust,
               TimelineWidget* timeline)
{
    // Eagerly apply
    engine->setClipAdjust(projectId, trackId, clipId, newAdjust);

    Fun undo = [engine, projectId, trackId, clipId, oldAdjust]() -> bool {
        engine->setClipAdjust(projectId, trackId, clipId, oldAdjust);
        return true;
    };
    Fun redo = [engine, projectId, trackId, clipId, newAdjust]() -> bool {
        engine->setClipAdjust(projectId, trackId, clipId, newAdjust);
        return true;
    };

    if (timeline) timeline->refreshTracks();
    pushUndo(timeline, undo, redo, QObject::tr("Edit clip properties"));
    return true;
}

bool setTrackState(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                   const EngineBridge::TrackState& oldState,
                   const EngineBridge::TrackState& newState,
                   TimelineWidget* timeline)
{
    // Eagerly apply
    engine->setTrackState(projectId, trackId, newState);

    Fun undo = [engine, projectId, trackId, oldState]() -> bool {
        engine->setTrackState(projectId, trackId, oldState);
        return true;
    };
    Fun redo = [engine, projectId, trackId, newState]() -> bool {
        engine->setTrackState(projectId, trackId, newState);
        return true;
    };

    if (timeline) timeline->refreshTracks();
    pushUndo(timeline, undo, redo, QObject::tr("Toggle track state"));
    return true;
}

} // namespace TimelineFunctions
} // namespace beta
