#include "EngineBridge.h"
#include "engine.h"

#include <QList>
#include <QString>
#include <QByteArray>

namespace beta {

struct EngineBridge::Impl {
    EngineHandle handle = nullptr;
};

EngineBridge::EngineBridge(QObject* parent)
    : QObject(parent), d_(std::make_unique<Impl>())
{
    d_->handle = engine_new();
}

EngineBridge::~EngineBridge()
{
    if (d_->handle) {
        engine_free(d_->handle);
        d_->handle = nullptr;
    }
}

uint64_t EngineBridge::createProject(const QString& name)
{
    QByteArray n = name.toUtf8();
    return engine_create_project(d_->handle, n.constData());
}

bool EngineBridge::closeProject(uint64_t projectId)
{
    return engine_close_project(d_->handle, projectId) != 0;
}

uint64_t EngineBridge::addTrack(uint64_t projectId, TrackKind kind, const QString& name)
{
    QByteArray n = name.toUtf8();
    return engine_add_track(d_->handle, projectId, static_cast<int>(kind), n.constData());
}

bool EngineBridge::removeTrack(uint64_t projectId, uint64_t trackId)
{
    return engine_remove_track(d_->handle, projectId, trackId) != 0;
}

QList<uint64_t> EngineBridge::trackIds(uint64_t projectId) const
{
    QList<uint64_t> out;
    size_t n = engine_track_count(d_->handle, projectId);
    out.reserve(static_cast<int>(n));
    // We iterate ids 1..n because the engine assigns them monotonically
    // starting at 1. Any gaps (from removed tracks) are skipped.
    for (uint64_t i = 1; i <= n + 16; ++i) {
        char* nm = engine_track_name(d_->handle, projectId, i);
        if (nm) {
            out.append(i);
            engine_string_free(nm);
        }
        if (static_cast<size_t>(out.size()) >= n) break;
    }
    return out;
}

int EngineBridge::trackKind(uint64_t projectId, uint64_t trackId) const
{
    return engine_track_kind(d_->handle, projectId, trackId);
}

QString EngineBridge::trackName(uint64_t projectId, uint64_t trackId) const
{
    char* nm = engine_track_name(d_->handle, projectId, trackId);
    if (!nm) return {};
    QString s = QString::fromUtf8(nm);
    engine_string_free(nm);
    return s;
}

EngineBridge::TrackState
EngineBridge::trackState(uint64_t projectId, uint64_t trackId) const
{
    ::TrackState s = engine_track_state(d_->handle, projectId, trackId);
    return TrackState{ s.visible != 0, s.locked != 0, s.muted != 0 };
}

bool EngineBridge::setTrackState(uint64_t projectId, uint64_t trackId, const TrackState& s)
{
    ::TrackState raw;
    raw.visible = s.visible ? 1 : 0;
    raw.locked = s.locked ? 1 : 0;
    raw.muted = s.muted ? 1 : 0;
    return engine_set_track_state(d_->handle, projectId, trackId, raw) != 0;
}

uint64_t EngineBridge::addClip(uint64_t projectId, uint64_t trackId,
                                const QString& mediaPath, const QString& mediaName,
                                uint64_t startFrame, uint64_t durationFrames)
{
    QByteArray p = mediaPath.toUtf8();
    QByteArray n = mediaName.toUtf8();
    return engine_add_clip(d_->handle, projectId, trackId,
                           p.constData(), n.constData(),
                           startFrame, durationFrames);
}

bool EngineBridge::removeClip(uint64_t projectId, uint64_t trackId, uint64_t clipId)
{
    return engine_remove_clip(d_->handle, projectId, trackId, clipId) != 0;
}

QString EngineBridge::engineVersion() const
{
    const char* v = engine_version_static();
    return QString::fromUtf8(v);
}

} // namespace beta
