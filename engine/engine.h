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

enum TrackKind { TRACK_VIDEO = 0, TRACK_AUDIO = 1, TRACK_IMAGE = 2 };
enum MediaKind { MEDIA_UNKNOWN = 0, MEDIA_VIDEO = 1, MEDIA_AUDIO = 2, MEDIA_IMAGE = 3 };

struct TrackState {
    int visible;
    int locked;
    int muted;
};

/* Color / transform / speed / fade / volume / opacity for a clip.
 * Defaults: brightness=contrast=saturation=0, hue=0, pos_x=pos_y=0,
 * scale=1, rotation=0, speed=1, fade_in=fade_out=0, volume=1, opacity=1. */
struct ClipAdjust {
    float brightness;   /* -1 .. +1 */
    float contrast;     /* -1 .. +1 */
    float saturation;   /* -1 .. +1 */
    float hue;          /* -180 .. +180 */
    float pos_x;        /* -1 .. +1 */
    float pos_y;        /* -1 .. +1 */
    float scale;        /* 0.1 .. 4.0 */
    float rotation;     /* degrees */
    float speed;        /* 0.25 .. 4.0 */
    uint64_t fade_in;   /* frames */
    uint64_t fade_out;  /* frames */
    float volume;       /* 0 .. 2 */
    float opacity;      /* 0 .. 1 */
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
int          engine_set_clip_adjust(EngineHandle engine, ProjectId project_id, TrackId track_id, ClipId clip_id, struct ClipAdjust adjust);

/* split / merge */
ClipId       engine_split_clip(EngineHandle engine, ProjectId project_id, TrackId track_id, ClipId clip_id, uint64_t split_frame);
int          engine_merge_clips(EngineHandle engine, ProjectId project_id, TrackId track_id, ClipId left_clip_id, ClipId right_clip_id);

/* media probe */
int          engine_probe_media(const char* path);

/* serialization */
char*        engine_serialize_project(EngineHandle engine, ProjectId project_id);

/* helpers */
char*        engine_version(void);
const char*  engine_version_static(void);
void         engine_string_free(char* s);

#ifdef __cplusplus
}
#endif

#endif /* BETA_ENGINE_H */
