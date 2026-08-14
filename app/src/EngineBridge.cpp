#include "EngineBridge.h"
#include "engine.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QString>

namespace beta {

namespace {

EngineBridge::ClipAdjust parseAdjust(const QJsonObject& o)
{
    EngineBridge::ClipAdjust a;
    a.brightness = static_cast<float>(o.value("brightness").toDouble(0.0));
    a.contrast   = static_cast<float>(o.value("contrast").toDouble(0.0));
    a.saturation = static_cast<float>(o.value("saturation").toDouble(0.0));
    a.hue        = static_cast<float>(o.value("hue").toDouble(0.0));
    a.posX       = static_cast<float>(o.value("pos_x").toDouble(0.0));
    a.posY       = static_cast<float>(o.value("pos_y").toDouble(0.0));
    a.scale      = static_cast<float>(o.value("scale").toDouble(1.0));
    a.rotation   = static_cast<float>(o.value("rotation").toDouble(0.0));
    a.speed      = static_cast<float>(o.value("speed").toDouble(1.0));
    a.fadeIn     = static_cast<uint64_t>(o.value("fade_in").toVariant().toULongLong());
    a.fadeOut    = static_cast<uint64_t>(o.value("fade_out").toVariant().toULongLong());
    a.volume     = static_cast<float>(o.value("volume").toDouble(1.0));
    a.opacity    = static_cast<float>(o.value("opacity").toDouble(1.0));
    return a;
}

::ClipAdjust toC(const EngineBridge::ClipAdjust& a)
{
    ::ClipAdjust c;
    c.brightness = a.brightness;
    c.contrast   = a.contrast;
    c.saturation = a.saturation;
    c.hue        = a.hue;
    c.pos_x      = a.posX;
    c.pos_y      = a.posY;
    c.scale      = a.scale;
    c.rotation   = a.rotation;
    c.speed      = a.speed;
    c.fade_in    = a.fadeIn;
    c.fade_out   = a.fadeOut;
    c.volume     = a.volume;
    c.opacity    = a.opacity;
    return c;
}

} // namespace

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
    ProjectSnapshot s = snapshot(projectId);
    out.reserve(s.tracks.size());
    for (const auto& t : s.tracks) out.append(t.id);
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

bool EngineBridge::moveClip(uint64_t projectId, uint64_t trackId, uint64_t clipId,
                             uint64_t newStartFrame)
{
    return engine_move_clip(d_->handle, projectId, trackId, clipId, newStartFrame) != 0;
}

bool EngineBridge::trimClip(uint64_t projectId, uint64_t trackId, uint64_t clipId,
                             uint64_t newTrimIn, uint64_t newDuration)
{
    return engine_trim_clip(d_->handle, projectId, trackId, clipId,
                             newTrimIn, newDuration) != 0;
}

bool EngineBridge::setClipMediaInfo(uint64_t projectId, uint64_t trackId, uint64_t clipId,
                                     uint32_t width, uint32_t height, uint64_t durationFrames)
{
    return engine_set_clip_media_info(d_->handle, projectId, trackId, clipId,
                                       width, height, durationFrames) != 0;
}

bool EngineBridge::setClipProps(uint64_t projectId, uint64_t trackId, uint64_t clipId,
                                 float volume, float opacity, float scale)
{
    return engine_set_clip_props(d_->handle, projectId, trackId, clipId,
                                  volume, opacity, scale) != 0;
}

bool EngineBridge::setClipAdjust(uint64_t projectId, uint64_t trackId, uint64_t clipId,
                                  const ClipAdjust& a)
{
    return engine_set_clip_adjust(d_->handle, projectId, trackId, clipId, toC(a)) != 0;
}

uint64_t EngineBridge::splitClip(uint64_t projectId, uint64_t trackId, uint64_t clipId,
                                  uint64_t splitFrame)
{
    return engine_split_clip(d_->handle, projectId, trackId, clipId, splitFrame);
}

bool EngineBridge::mergeClips(uint64_t projectId, uint64_t trackId,
                               uint64_t leftClipId, uint64_t rightClipId)
{
    return engine_merge_clips(d_->handle, projectId, trackId,
                               leftClipId, rightClipId) != 0;
}

EngineBridge::ProjectSnapshot EngineBridge::snapshot(uint64_t projectId) const
{
    ProjectSnapshot snap;
    char* json = engine_serialize_project(d_->handle, projectId);
    if (!json) return snap;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(
        QByteArray(json), &err);
    engine_string_free(json);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return snap;

    QJsonObject root = doc.object();
    snap.name   = root.value("name").toString();
    snap.width  = static_cast<uint32_t>(root.value("width").toInt(1920));
    snap.height = static_cast<uint32_t>(root.value("height").toInt(1080));
    snap.fps    = root.value("fps").toDouble(30.0);

    const QJsonArray tracks = root.value("tracks").toArray();
    for (const QJsonValue& tv : tracks) {
        QJsonObject to = tv.toObject();
        TrackInfo t;
        t.id   = static_cast<uint64_t>(to.value("id").toVariant().toULongLong());
        t.name = to.value("name").toString();
        QString kindStr = to.value("kind").toString();
        if      (kindStr == "video") t.kind = 0;
        else if (kindStr == "audio") t.kind = 1;
        else if (kindStr == "image") t.kind = 2;
        else                          t.kind = 0;

        QJsonObject so = to.value("state").toObject();
        t.state.visible = so.value("visible").toBool(true);
        t.state.locked  = so.value("locked").toBool(false);
        t.state.muted   = so.value("muted").toBool(false);

        const QJsonArray clips = to.value("clips").toArray();
        for (const QJsonValue& cv : clips) {
            QJsonObject co = cv.toObject();
            ClipInfo c;
            c.id                = static_cast<uint64_t>(co.value("id").toVariant().toULongLong());
            c.mediaPath         = co.value("media_path").toString();
            c.mediaName         = co.value("media_name").toString();
            c.startFrame        = static_cast<uint64_t>(co.value("start_frame").toVariant().toULongLong());
            c.durationFrames    = static_cast<uint64_t>(co.value("duration_frames").toVariant().toULongLong());
            c.trimInFrames      = static_cast<uint64_t>(co.value("trim_in_frames").toVariant().toULongLong());
            c.adjust            = parseAdjust(co.value("adjust").toObject());
            c.mediaWidth        = 0;
            c.mediaHeight       = 0;
            c.mediaDurationFrames = 0;
            t.clips.append(c);
        }
        snap.tracks.append(t);
    }
    return snap;
}

QString EngineBridge::engineVersion() const
{
    const char* v = engine_version_static();
    return QString::fromUtf8(v);
}

} // namespace beta
