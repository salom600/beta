#pragma once

#include <QUndoCommand>
#include <QString>
#include <cstdint>
#include <functional>
#include "EngineBridge.h"
#include "UndoHelper.h"

namespace beta {

class TimelineWidget;

/// v0.5: All timeline commands now use the `FunctionalUndoCommand`
/// pattern (one generic QUndoCommand subclass that wraps two `Fun`
/// lambdas). This is the Kdenlive architecture — see
/// `undohelper.hpp` and `timelinefunctions.cpp` in Kdenlive source.
///
/// Each `TimelineFunctions::*` helper performs the mutation eagerly
/// and accumulates matching undo/redo lambdas; the helper then pushes
/// a single `FunctionalUndoCommand` carrying both. Failure rolls back
/// via the partial undo lambda.
namespace TimelineFunctions {

/// Add a clip to a track. Pushes one undoable command.
bool addClip(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
             const QString& path, const QString& name,
             uint64_t startFrame, uint64_t durationFrames,
             TimelineWidget* timeline);

/// Remove a clip. Pushes one undoable command.
bool removeClip(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                uint64_t clipId, TimelineWidget* timeline);

/// Move a clip to a new start frame. Pushes one undoable command
/// (with mergeWith so consecutive drags collapse).
bool moveClip(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
              uint64_t clipId, uint64_t oldStart, uint64_t newStart,
              TimelineWidget* timeline);

/// Trim a clip (left or right edge). Pushes one undoable command.
bool trimClip(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
              uint64_t clipId,
              uint64_t oldStart, uint64_t oldDuration, uint64_t oldTrimIn,
              uint64_t newStart, uint64_t newDuration, uint64_t newTrimIn,
              TimelineWidget* timeline);

/// Split a clip at `splitFrame` into two clips. Pushes one undoable
/// command (composed of: snapshot original duration, split, capture
/// new clip id).
bool splitClip(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
               uint64_t clipId, uint64_t splitFrame,
               TimelineWidget* timeline);

/// Merge two adjacent clips with the same source media. Pushes one
/// undoable command.
bool mergeClips(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                uint64_t leftClipId, uint64_t rightClipId,
                TimelineWidget* timeline);

/// Set the full clip adjust block (color/transform/speed/fade/volume/opacity).
/// Pushes one undoable command.
bool setAdjust(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
               uint64_t clipId,
               const EngineBridge::ClipAdjust& oldAdjust,
               const EngineBridge::ClipAdjust& newAdjust,
               TimelineWidget* timeline);

/// Toggle a track's visible/locked/muted state. Pushes one undoable command.
bool setTrackState(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                   const EngineBridge::TrackState& oldState,
                   const EngineBridge::TrackState& newState,
                   TimelineWidget* timeline);

} // namespace TimelineFunctions

} // namespace beta
