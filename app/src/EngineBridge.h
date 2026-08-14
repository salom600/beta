#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonDocument>
#include <cstdint>
#include <memory>

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

    struct ClipAdjust {
        float brightness = 0.0f;
        float contrast   = 0.0f;
        float saturation = 0.0f;
        float hue        = 0.0f;
        float posX       = 0.0f;
        float posY       = 0.0f;
        float scale      = 1.0f;
        float rotation   = 0.0f;
        float speed      = 1.0f;
        uint64_t fadeIn  = 0;
        uint64_t fadeOut = 0;
        float volume     = 1.0f;
        float opacity    = 1.0f;
    };

    struct ClipInfo {
        uint64_t id;
        QString  mediaPath;
        QString  mediaName;
        uint64_t startFrame;
        uint64_t durationFrames;
        uint64_t trimInFrames;
        ClipAdjust adjust;
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
    bool setClipAdjust(uint64_t projectId, uint64_t trackId, uint64_t clipId,
                       const ClipAdjust& a);

    /// Split a clip at the given absolute timeline frame. Returns the
    /// new (right) clip id, or 0 on failure.
    uint64_t splitClip(uint64_t projectId, uint64_t trackId, uint64_t clipId, uint64_t splitFrame);

    /// Merge two adjacent clips on the same track (left + right).
    bool mergeClips(uint64_t projectId, uint64_t trackId,
                    uint64_t leftClipId, uint64_t rightClipId);

    ProjectSnapshot snapshot(uint64_t projectId) const;

    QString engineVersion() const;

private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};

} // namespace beta
