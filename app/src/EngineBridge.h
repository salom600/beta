#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonDocument>
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

    struct ClipInfo {
        uint64_t id;
        QString  mediaPath;
        QString  mediaName;
        uint64_t startFrame;
        uint64_t durationFrames;
        uint64_t trimInFrames;
        float    volume;
        float    opacity;
        float    scale;
        int      mediaWidth;
        int      mediaHeight;
        uint64_t mediaDurationFrames;
    };

    struct TrackInfo {
        uint64_t id;
        int      kind;
        QString  name;
        TrackState state;
        QList<ClipInfo> clips;
    };

    struct ProjectSnapshot {
        QString name;
        uint32_t width;
        uint32_t height;
        double   fps;
        QList<TrackInfo> tracks;
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

    QList<uint64_t> trackIds(uint64_t projectId) const;

    int trackKind(uint64_t projectId, uint64_t trackId) const;
    QString trackName(uint64_t projectId, uint64_t trackId) const;
    TrackState trackState(uint64_t projectId, uint64_t trackId) const;
    bool setTrackState(uint64_t projectId, uint64_t trackId, const TrackState& state);

    uint64_t addClip(uint64_t projectId, uint64_t trackId,
                     const QString& mediaPath, const QString& mediaName,
                     uint64_t startFrame, uint64_t durationFrames);
    bool removeClip(uint64_t projectId, uint64_t trackId, uint64_t clipId);
    bool moveClip(uint64_t projectId, uint64_t trackId, uint64_t clipId, uint64_t newStartFrame);
    bool trimClip(uint64_t projectId, uint64_t trackId, uint64_t clipId,
                  uint64_t newTrimIn, uint64_t newDuration);
    bool setClipMediaInfo(uint64_t projectId, uint64_t trackId, uint64_t clipId,
                          uint32_t width, uint32_t height, uint64_t durationFrames);
    bool setClipProps(uint64_t projectId, uint64_t trackId, uint64_t clipId,
                      float volume, float opacity, float scale);

    /// Pull the entire project (tracks + clips + settings) as a single
    /// parsed snapshot. The engine serializes to JSON and we parse it
    /// with QJsonDocument. Useful for full UI refreshes and for export.
    ProjectSnapshot snapshot(uint64_t projectId) const;

    QString engineVersion() const;

private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};

} // namespace beta
