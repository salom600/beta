#pragma once

#include <QUndoCommand>
#include <QString>
#include <cstdint>
#include "EngineBridge.h"

namespace beta {

class TimelineWidget;

/// Base class for all timeline-editing commands. Stores the project /
/// track / clip ids so the command knows where to apply itself. The
/// `TimelineWidget*` is used to trigger a visual refresh after each
/// undo/redo.
class TimelineCommand : public QUndoCommand {
public:
    TimelineCommand(EngineBridge* engine, uint64_t projectId,
                    TimelineWidget* timeline,
                    QUndoCommand* parent = nullptr)
        : QUndoCommand(parent)
        , engine_(engine)
        , projectId_(projectId)
        , timeline_(timeline) {}

protected:
    EngineBridge*   engine_   = nullptr;
    uint64_t        projectId_ = 0;
    TimelineWidget* timeline_ = nullptr;

    void refresh() const;
};

class AddClipCmd : public TimelineCommand {
public:
    AddClipCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
               const QString& path, const QString& name,
               uint64_t startFrame, uint64_t durationFrames,
               TimelineWidget* timeline, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int  id() const override { return 1001; }
    uint64_t clipId() const { return clipId_; }

private:
    uint64_t trackId_;
    QString  path_;
    QString  name_;
    uint64_t startFrame_;
    uint64_t durationFrames_;
    uint64_t clipId_ = 0;
    bool     firstRun_ = true;
};

class RemoveClipCmd : public TimelineCommand {
public:
    RemoveClipCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                  uint64_t clipId, const QString& path, const QString& name,
                  uint64_t startFrame, uint64_t durationFrames, uint64_t trimIn,
                  TimelineWidget* timeline, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int  id() const override { return 1002; }

private:
    uint64_t trackId_;
    uint64_t clipId_;
    QString  path_;
    QString  name_;
    uint64_t startFrame_;
    uint64_t durationFrames_;
    uint64_t trimIn_;
};

class MoveClipCmd : public TimelineCommand {
public:
    MoveClipCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                uint64_t clipId, uint64_t oldStart, uint64_t newStart,
                TimelineWidget* timeline, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int  id() const override { return 1003; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    uint64_t trackId_;
    uint64_t clipId_;
    uint64_t oldStart_;
    uint64_t newStart_;
};

class TrimClipCmd : public TimelineCommand {
public:
    TrimClipCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                uint64_t clipId,
                uint64_t oldStart, uint64_t oldDuration, uint64_t oldTrimIn,
                uint64_t newStart, uint64_t newDuration, uint64_t newTrimIn,
                TimelineWidget* timeline, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int  id() const override { return 1004; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    uint64_t trackId_;
    uint64_t clipId_;
    uint64_t oldStart_, oldDuration_, oldTrimIn_;
    uint64_t newStart_, newDuration_, newTrimIn_;
};

class SplitClipCmd : public TimelineCommand {
public:
    SplitClipCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                 uint64_t clipId, uint64_t splitFrame,
                 TimelineWidget* timeline, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int  id() const override { return 1005; }

private:
    uint64_t trackId_;
    uint64_t clipId_;
    uint64_t splitFrame_;
    uint64_t newClipId_ = 0;
    uint64_t origDuration_ = 0;
    bool     firstRun_ = true;
};

class MergeClipsCmd : public TimelineCommand {
public:
    MergeClipsCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                  uint64_t leftClipId, uint64_t rightClipId,
                  uint64_t leftOrigDuration,
                  TimelineWidget* timeline, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int  id() const override { return 1006; }

private:
    uint64_t trackId_;
    uint64_t leftClipId_;
    uint64_t rightClipId_;
    uint64_t leftOrigDuration_;
    bool     firstRun_ = true;
};

class SetAdjustCmd : public TimelineCommand {
public:
    SetAdjustCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                 uint64_t clipId,
                 const EngineBridge::ClipAdjust& oldAdjust,
                 const EngineBridge::ClipAdjust& newAdjust,
                 TimelineWidget* timeline, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int  id() const override { return 1007; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    uint64_t trackId_;
    uint64_t clipId_;
    EngineBridge::ClipAdjust oldAdjust_;
    EngineBridge::ClipAdjust newAdjust_;
};

class SetTrackStateCmd : public TimelineCommand {
public:
    SetTrackStateCmd(EngineBridge* engine, uint64_t projectId, uint64_t trackId,
                     EngineBridge::TrackState oldState,
                     EngineBridge::TrackState newState,
                     TimelineWidget* timeline, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int  id() const override { return 1008; }

private:
    uint64_t trackId_;
    EngineBridge::TrackState oldState_;
    EngineBridge::TrackState newState_;
};

} // namespace beta
