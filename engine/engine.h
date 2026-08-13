#ifndef BETA_ENGINE_H
#define BETA_ENGINE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Engine Engine;

typedef Engine* EngineHandle;
typedef uint64_t ProjectId;
typedef uint64_t TrackId;
typedef uint64_t ClipId;

enum TrackKind {
    TRACK_VIDEO = 0,
    TRACK_AUDIO = 1,
    TRACK_IMAGE = 2,
};

enum MediaKind {
    MEDIA_UNKNOWN = 0,
    MEDIA_VIDEO = 1,
    MEDIA_AUDIO = 2,
    MEDIA_IMAGE = 3,
};

struct TrackState {
    int visible;  /* 0 = hidden, 1 = visible */
    int locked;   /* 0 = unlocked, 1 = locked */
    int muted;    /* 0 = unmuted, 1 = muted */
};

/* lifecycle */
EngineHandle engine_new(void);
void         engine_free(EngineHandle engine);

/* projects */
ProjectId    engine_create_project(EngineHandle engine, const char* name);
int          engine_close_project(EngineHandle engine, ProjectId project_id);

/* tracks */
TrackId      engine_add_track(EngineHandle engine, ProjectId project_id, int kind, const char* name);
int          engine_remove_track(EngineHandle engine, ProjectId project_id, TrackId track_id);
size_t       engine_track_count(EngineHandle engine, ProjectId project_id);
int          engine_track_kind(EngineHandle engine, ProjectId project_id, TrackId track_id);
char*        engine_track_name(EngineHandle engine, ProjectId project_id, TrackId track_id);

struct TrackState engine_track_state(EngineHandle engine, ProjectId project_id, TrackId track_id);
int          engine_set_track_state(EngineHandle engine, ProjectId project_id, TrackId track_id, struct TrackState state);

/* clips */
ClipId       engine_add_clip(EngineHandle engine, ProjectId project_id, TrackId track_id,
                              const char* media_path, const char* media_name,
                              uint64_t start_frame, uint64_t duration_frames);
int          engine_remove_clip(EngineHandle engine, ProjectId project_id, TrackId track_id, ClipId clip_id);
size_t       engine_clip_count(EngineHandle engine, ProjectId project_id, TrackId track_id);

/* clip editing */
int          engine_move_clip(EngineHandle engine, ProjectId project_id, TrackId track_id, ClipId clip_id, uint64_t new_start_frame);
int          engine_trim_clip(EngineHandle engine, ProjectId project_id, TrackId track_id, ClipId clip_id, uint64_t new_trim_in, uint64_t new_duration);
int          engine_set_clip_media_info(EngineHandle engine, ProjectId project_id, TrackId track_id, ClipId clip_id, uint32_t width, uint32_t height, uint64_t duration_frames);
int          engine_set_clip_props(EngineHandle engine, ProjectId project_id, TrackId track_id, ClipId clip_id, float volume, float opacity, float scale);

/* media probe */
int          engine_probe_media(const char* path);

/* serialization (returns JSON; caller must engine_string_free) */
char*        engine_serialize_project(EngineHandle engine, ProjectId project_id);

/* helpers */
char*        engine_version(void);
const char*  engine_version_static(void);
void         engine_string_free(char* s);

#ifdef __cplusplus
}
#endif

#endif /* BETA_ENGINE_H */
