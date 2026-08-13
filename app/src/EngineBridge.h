#pragma once

#include <QObject>
#include <QString>
#include <cstdint>
#include <memory>

// Forward-declare opaque engine struct from C ABI
struct Engine;

namespace beta {

/// RAII wrapper around the C engine ABI. Owns the engine handle and
/// exposes high-level operations to the rest of the UI.
class EngineBridge : public QObject {
    Q_OBJECT
public:
    enum TrackKind { Video = 0, Audio = 1, Image = 2 };

    struct TrackState {
        bool visible = true;
        bool locked = false;
        bool muted = false;
    };

    explicit EngineBridge(QObject* parent = nullptr);
    ~EngineBridge() override;

    EngineBridge(const EngineBridge&) = delete;
    EngineBridge& operator=(const EngineBridge&) = delete;

    /// Create a new project; returns the project id (>0 on success).
    uint64_t createProject(const QString& name);
    bool closeProject(uint64_t projectId);

    uint64_t addTrack(uint64_t projectId, TrackKind kind, const QString& name);
    bool removeTrack(uint64_t projectId, uint64_t trackId);

    /// Returns the ids of tracks in creation order.
    QList<uint64_t> trackIds(uint64_t projectId) const;

    int trackKind(uint64_t projectId, uint64_t trackId) const;
    QString trackName(uint64_t projectId, uint64_t trackId) const;
    TrackState trackState(uint64_t projectId, uint64_t trackId) const;
    bool setTrackState(uint64_t projectId, uint64_t trackId, const TrackState& state);

    uint64_t addClip(uint64_t projectId, uint64_t trackId,
                     const QString& mediaPath, const QString& mediaName,
                     uint64_t startFrame, uint64_t durationFrames);
    bool removeClip(uint64_t projectId, uint64_t trackId, uint64_t clipId);

    QString engineVersion() const;

private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};

} // namespace beta
