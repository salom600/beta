//! C ABI surface. Every function here is `extern "C"` and uses only
//! C-compatible types so the C++ side can call into the engine through
//! a single `engine.h` header.

use crate::media::{MediaAsset, MediaKind};
use crate::project::ProjectId;
use crate::timeline::{Clip, ClipAdjust, ClipId, Track, TrackId, TrackKind, TrackState};
use crate::Engine;
use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_float};
use std::ptr;

/// Opaque engine handle.
pub type EngineHandle = *mut Engine;

#[no_mangle]
pub extern "C" fn engine_new() -> EngineHandle {
    Box::into_raw(Box::new(Engine::new()))
}

#[no_mangle]
pub extern "C" fn engine_free(engine: EngineHandle) {
    if engine.is_null() { return; }
    unsafe { drop(Box::from_raw(engine)); }
}

#[no_mangle]
pub extern "C" fn engine_create_project(engine: EngineHandle, name: *const c_char) -> u64 {
    if engine.is_null() { return 0; }
    let eng = unsafe { &*engine };
    let name = if name.is_null() {
        "Untitled".to_string()
    } else {
        unsafe { CStr::from_ptr(name) }.to_string_lossy().to_string()
    };
    eng.create_project(name)
}

#[no_mangle]
pub extern "C" fn engine_close_project(engine: EngineHandle, project_id: u64) -> bool {
    if engine.is_null() { return false; }
    let eng = unsafe { &*engine };
    eng.close_project(project_id)
}

#[no_mangle]
pub extern "C" fn engine_add_track(
    engine: EngineHandle, project_id: u64, kind: c_int, name: *const c_char,
) -> TrackId {
    if engine.is_null() { return 0; }
    let eng = unsafe { &*engine };
    let kind = match TrackKind::from_i32(kind as i32) {
        Some(k) => k, None => return 0,
    };
    let name = if name.is_null() {
        format!("{:?}", kind)
    } else {
        unsafe { CStr::from_ptr(name) }.to_string_lossy().to_string()
    };
    let mut projects = eng.projects.lock().unwrap();
    match projects.get_mut(&project_id) {
        Some(p) => p.add_track(kind, name),
        None => 0,
    }
}

#[no_mangle]
pub extern "C" fn engine_remove_track(
    engine: EngineHandle, project_id: u64, track_id: TrackId,
) -> bool {
    if engine.is_null() { return false; }
    let eng = unsafe { &*engine };
    let mut projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get_mut(&project_id) { p.remove_track(track_id) } else { false }
}

#[no_mangle]
pub extern "C" fn engine_track_count(engine: EngineHandle, project_id: u64) -> usize {
    if engine.is_null() { return 0; }
    let eng = unsafe { &*engine };
    let projects = eng.projects.lock().unwrap();
    projects.get(&project_id).map(|p| p.tracks.len()).unwrap_or(0)
}

#[no_mangle]
pub extern "C" fn engine_track_state(
    engine: EngineHandle, project_id: u64, track_id: TrackId,
) -> TrackState {
    if engine.is_null() { return TrackState::default(); }
    let eng = unsafe { &*engine };
    let projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get(&project_id) {
        if let Some(t) = p.tracks.get(&track_id) { return t.state; }
    }
    TrackState::default()
}

#[no_mangle]
pub extern "C" fn engine_set_track_state(
    engine: EngineHandle, project_id: u64, track_id: TrackId, state: TrackState,
) -> bool {
    if engine.is_null() { return false; }
    let eng = unsafe { &*engine };
    let mut projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get_mut(&project_id) {
        if let Some(t) = p.tracks.get_mut(&track_id) { t.state = state; return true; }
    }
    false
}

#[no_mangle]
pub extern "C" fn engine_add_clip(
    engine: EngineHandle, project_id: u64, track_id: TrackId,
    media_path: *const c_char, media_name: *const c_char,
    start_frame: u64, duration_frames: u64,
) -> ClipId {
    if engine.is_null() { return 0; }
    let eng = unsafe { &*engine };
    let path = if media_path.is_null() { String::new() }
        else { unsafe { CStr::from_ptr(media_path) }.to_string_lossy().to_string() };
    let name = if media_name.is_null() { String::new() }
        else { unsafe { CStr::from_ptr(media_name) }.to_string_lossy().to_string() };
    let mut projects = eng.projects.lock().unwrap();
    let project = match projects.get_mut(&project_id) { Some(p) => p, None => return 0 };
    let track = match project.tracks.get_mut(&track_id) { Some(t) => t, None => return 0 };
    let clip = Clip {
        id: 0, media_path: path, media_name: name,
        start_frame,
        duration_frames: if duration_frames == 0 { 150 } else { duration_frames },
        trim_in_frames: 0,
        adjust: ClipAdjust::default(),
        media_width: 0, media_height: 0, media_duration_frames: 0,
    };
    track.add_clip(clip)
}

#[no_mangle]
pub extern "C" fn engine_remove_clip(
    engine: EngineHandle, project_id: u64, track_id: TrackId, clip_id: ClipId,
) -> bool {
    if engine.is_null() { return false; }
    let eng = unsafe { &*engine };
    let mut projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get_mut(&project_id) {
        if let Some(t) = p.tracks.get_mut(&track_id) { return t.remove_clip(clip_id); }
    }
    false
}

#[no_mangle]
pub extern "C" fn engine_move_clip(
    engine: EngineHandle, project_id: u64, track_id: TrackId, clip_id: ClipId,
    new_start_frame: u64,
) -> bool {
    if engine.is_null() { return false; }
    let eng = unsafe { &*engine };
    let mut projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get_mut(&project_id) {
        if let Some(t) = p.tracks.get_mut(&track_id) {
            if let Some(c) = t.clips.get_mut(&clip_id) {
                c.start_frame = new_start_frame;
                return true;
            }
        }
    }
    false
}

#[no_mangle]
pub extern "C" fn engine_trim_clip(
    engine: EngineHandle, project_id: u64, track_id: TrackId, clip_id: ClipId,
    new_trim_in: u64, new_duration: u64,
) -> bool {
    if engine.is_null() { return false; }
    let eng = unsafe { &*engine };
    let mut projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get_mut(&project_id) {
        if let Some(t) = p.tracks.get_mut(&track_id) {
            if let Some(c) = t.clips.get_mut(&clip_id) {
                c.trim_in_frames = new_trim_in;
                c.duration_frames = new_duration.max(1);
                return true;
            }
        }
    }
    false
}

#[no_mangle]
pub extern "C" fn engine_set_clip_media_info(
    engine: EngineHandle, project_id: u64, track_id: TrackId, clip_id: ClipId,
    width: u32, height: u32, duration_frames: u64,
) -> bool {
    if engine.is_null() { return false; }
    let eng = unsafe { &*engine };
    let mut projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get_mut(&project_id) {
        if let Some(t) = p.tracks.get_mut(&track_id) {
            if let Some(c) = t.clips.get_mut(&clip_id) {
                c.media_width = width;
                c.media_height = height;
                c.media_duration_frames = duration_frames;
                return true;
            }
        }
    }
    false
}

/// Set the full clip adjust block (color / transform / speed / fade / volume / opacity).
#[no_mangle]
pub extern "C" fn engine_set_clip_adjust(
    engine: EngineHandle, project_id: u64, track_id: TrackId, clip_id: ClipId,
    adjust: ClipAdjust,
) -> bool {
    if engine.is_null() { return false; }
    let eng = unsafe { &*engine };
    let mut projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get_mut(&project_id) {
        if let Some(t) = p.tracks.get_mut(&track_id) {
            if let Some(c) = t.clips.get_mut(&clip_id) {
                c.adjust = adjust;
                return true;
            }
        }
    }
    false
}

/// Backwards-compat: set just volume/opacity/scale.
#[no_mangle]
pub extern "C" fn engine_set_clip_props(
    engine: EngineHandle, project_id: u64, track_id: TrackId, clip_id: ClipId,
    volume: c_float, opacity: c_float, scale: c_float,
) -> bool {
    if engine.is_null() { return false; }
    let eng = unsafe { &*engine };
    let mut projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get_mut(&project_id) {
        if let Some(t) = p.tracks.get_mut(&track_id) {
            if let Some(c) = t.clips.get_mut(&clip_id) {
                c.adjust.volume = volume;
                c.adjust.opacity = opacity;
                c.adjust.scale = scale;
                return true;
            }
        }
    }
    false
}

/// Split the given clip at `split_frame` (absolute frame on the timeline).
/// Returns the new (right-hand) clip id, or 0 on failure.
/// The original clip's duration is reduced; the new clip starts at
/// `split_frame` and its trim_in is adjusted accordingly.
#[no_mangle]
pub extern "C" fn engine_split_clip(
    engine: EngineHandle, project_id: u64, track_id: TrackId, clip_id: ClipId,
    split_frame: u64,
) -> ClipId {
    if engine.is_null() { return 0; }
    let eng = unsafe { &*engine };
    let mut projects = eng.projects.lock().unwrap();
    let project = match projects.get_mut(&project_id) { Some(p) => p, None => return 0 };
    let track = match project.tracks.get_mut(&track_id) { Some(t) => t, None => return 0 };

    // Snapshot the original clip
    let original = match track.clips.get(&clip_id).cloned() { Some(c) => c, None => return 0 };
    if split_frame <= original.start_frame { return 0; }
    if split_frame >= original.start_frame + original.duration_frames { return 0; }

    let left_dur  = split_frame - original.start_frame;
    let right_dur = original.duration_frames - left_dur;
    let right_trim = original.trim_in_frames + left_dur;

    // Shrink the left half
    if let Some(c) = track.clips.get_mut(&clip_id) {
        c.duration_frames = left_dur;
    }

    // Build the right half
    let mut right = original.clone();
    right.id = 0;
    right.start_frame = split_frame;
    right.duration_frames = right_dur;
    right.trim_in_frames = right_trim;
    track.add_clip(right)
}

/// Merge two adjacent clips on the same track into a single clip.
/// The left clip is extended; the right clip is removed.
/// Returns true on success.
#[no_mangle]
pub extern "C" fn engine_merge_clips(
    engine: EngineHandle, project_id: u64, track_id: TrackId,
    left_clip_id: ClipId, right_clip_id: ClipId,
) -> bool {
    if engine.is_null() { return false; }
    let eng = unsafe { &*engine };
    let mut projects = eng.projects.lock().unwrap();
    let project = match projects.get_mut(&project_id) { Some(p) => p, None => return false };
    let track = match project.tracks.get_mut(&track_id) { Some(t) => t, None => return false };

    let right = match track.clips.get(&right_clip_id).cloned() { Some(c) => c, None => return false };
    let left  = match track.clips.get(&left_clip_id).cloned()  { Some(c) => c, None => return false };

    // Only merge if they are actually adjacent (left.end == right.start)
    // and share the same source media.
    if left.start_frame + left.duration_frames != right.start_frame { return false; }
    if left.media_path != right.media_path { return false; }

    // Extend left's duration to cover right
    if let Some(c) = track.clips.get_mut(&left_clip_id) {
        c.duration_frames += right.duration_frames;
    }
    track.remove_clip(right_clip_id);
    true
}

#[no_mangle]
pub extern "C" fn engine_clip_count(
    engine: EngineHandle, project_id: u64, track_id: TrackId,
) -> usize {
    if engine.is_null() { return 0; }
    let eng = unsafe { &*engine };
    let projects = eng.projects.lock().unwrap();
    projects.get(&project_id)
        .and_then(|p| p.tracks.get(&track_id))
        .map(|t| t.clips.len())
        .unwrap_or(0)
}

#[no_mangle]
pub extern "C" fn engine_track_name(
    engine: EngineHandle, project_id: u64, track_id: TrackId,
) -> *mut c_char {
    if engine.is_null() { return ptr::null_mut(); }
    let eng = unsafe { &*engine };
    let projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get(&project_id) {
        if let Some(t) = p.tracks.get(&track_id) {
            if let Ok(s) = CString::new(t.name.clone()) { return s.into_raw(); }
        }
    }
    ptr::null_mut()
}

#[no_mangle]
pub extern "C" fn engine_string_free(s: *mut c_char) {
    if s.is_null() { return; }
    unsafe { drop(CString::from_raw(s)); }
}

#[no_mangle]
pub extern "C" fn engine_track_kind(
    engine: EngineHandle, project_id: u64, track_id: TrackId,
) -> c_int {
    if engine.is_null() { return -1; }
    let eng = unsafe { &*engine };
    let projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get(&project_id) {
        if let Some(t) = p.tracks.get(&track_id) { return t.kind as c_int; }
    }
    -1
}

#[no_mangle]
pub extern "C" fn engine_probe_media(path: *const c_char) -> MediaKind {
    if path.is_null() { return MediaKind::Unknown; }
    let s = unsafe { CStr::from_ptr(path) }.to_string_lossy().to_string();
    MediaAsset::from_path(&s).kind
}

#[no_mangle]
pub extern "C" fn engine_version() -> *mut c_char {
    let v = env!("CARGO_PKG_VERSION");
    CString::new(v).unwrap_or_default().into_raw()
}

#[no_mangle]
pub extern "C" fn engine_version_static() -> *const c_char {
    static VERSION: &[u8] = b"0.5.0\0";
    VERSION.as_ptr() as *const c_char
}

#[no_mangle]
pub extern "C" fn engine_serialize_project(
    engine: EngineHandle, project_id: u64,
) -> *mut c_char {
    if engine.is_null() { return ptr::null_mut(); }
    let eng = unsafe { &*engine };
    let projects = eng.projects.lock().unwrap();
    let project = match projects.get(&project_id) { Some(p) => p, None => return ptr::null_mut() };
    match serde_json::to_string(project) {
        Ok(s) => CString::new(s).unwrap_or_default().into_raw(),
        Err(_) => ptr::null_mut(),
    }
}

#[allow(dead_code)]
fn _unused(_: &ProjectId, _: &Track, _: &Clip) {}
