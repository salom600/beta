//! C ABI surface. Every function here is `extern "C"` and uses only
//! C-compatible types so the C++ side can call into the engine through
//! a single `engine.h` header.

use crate::engine::Engine;
use crate::media::{MediaAsset, MediaKind};
use crate::project::ProjectId;
use crate::timeline::{Clip, ClipId, Track, TrackId, TrackKind, TrackState};
use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int};
use std::ptr;

/// Opaque engine handle. The C side treats this as `void*`.
pub type EngineHandle = *mut Engine;

#[no_mangle]
pub extern "C" fn engine_new() -> EngineHandle {
    Box::into_raw(Box::new(Engine::new()))
}

#[no_mangle]
pub extern "C" fn engine_free(engine: EngineHandle) {
    if engine.is_null() {
        return;
    }
    unsafe {
        drop(Box::from_raw(engine));
    }
}

#[no_mangle]
pub extern "C" fn engine_create_project(engine: EngineHandle, name: *const c_char) -> u64 {
    if engine.is_null() {
        return 0;
    }
    let eng = unsafe { &*engine };
    let name = if name.is_null() {
        "Untitled".to_string()
    } else {
        unsafe { CStr::from_ptr(name) }
            .to_string_lossy()
            .to_string()
    };
    eng.create_project(name)
}

#[no_mangle]
pub extern "C" fn engine_close_project(engine: EngineHandle, project_id: u64) -> bool {
    if engine.is_null() {
        return false;
    }
    let eng = unsafe { &*engine };
    eng.close_project(project_id)
}

#[no_mangle]
pub extern "C" fn engine_add_track(
    engine: EngineHandle,
    project_id: u64,
    kind: c_int,
    name: *const c_char,
) -> TrackId {
    if engine.is_null() {
        return 0;
    }
    let eng = unsafe { &*engine };
    let kind = match TrackKind::from_i32(kind as i32) {
        Some(k) => k,
        None => return 0,
    };
    let name = if name.is_null() {
        format!("{:?}", kind)
    } else {
        unsafe { CStr::from_ptr(name) }
            .to_string_lossy()
            .to_string()
    };
    let mut projects = eng.projects.lock().unwrap();
    let project = match projects.get_mut(&project_id) {
        Some(p) => p,
        None => return 0,
    };
    project.add_track(kind, name)
}

#[no_mangle]
pub extern "C" fn engine_remove_track(
    engine: EngineHandle,
    project_id: u64,
    track_id: TrackId,
) -> bool {
    if engine.is_null() {
        return false;
    }
    let eng = unsafe { &*engine };
    let mut projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get_mut(&project_id) {
        p.remove_track(track_id)
    } else {
        false
    }
}

#[no_mangle]
pub extern "C" fn engine_track_count(engine: EngineHandle, project_id: u64) -> usize {
    if engine.is_null() {
        return 0;
    }
    let eng = unsafe { &*engine };
    let projects = eng.projects.lock().unwrap();
    projects
        .get(&project_id)
        .map(|p| p.tracks.len())
        .unwrap_or(0)
}

#[no_mangle]
pub extern "C" fn engine_track_state(
    engine: EngineHandle,
    project_id: u64,
    track_id: TrackId,
) -> TrackState {
    if engine.is_null() {
        return TrackState::default();
    }
    let eng = unsafe { &*engine };
    let mut projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get_mut(&project_id) {
        if let Some(t) = p.tracks.get(&track_id) {
            return t.state;
        }
    }
    TrackState::default()
}

#[no_mangle]
pub extern "C" fn engine_set_track_state(
    engine: EngineHandle,
    project_id: u64,
    track_id: TrackId,
    state: TrackState,
) -> bool {
    if engine.is_null() {
        return false;
    }
    let eng = unsafe { &*engine };
    let mut projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get_mut(&project_id) {
        if let Some(t) = p.tracks.get_mut(&track_id) {
            t.state = state;
            return true;
        }
    }
    false
}

#[no_mangle]
pub extern "C" fn engine_add_clip(
    engine: EngineHandle,
    project_id: u64,
    track_id: TrackId,
    media_path: *const c_char,
    media_name: *const c_char,
    start_frame: u64,
    duration_frames: u64,
) -> ClipId {
    if engine.is_null() {
        return 0;
    }
    let eng = unsafe { &*engine };
    let path = if media_path.is_null() {
        String::new()
    } else {
        unsafe { CStr::from_ptr(media_path) }
            .to_string_lossy()
            .to_string()
    };
    let name = if media_name.is_null() {
        String::new()
    } else {
        unsafe { CStr::from_ptr(media_name) }
            .to_string_lossy()
            .to_string()
    };
    let mut projects = eng.projects.lock().unwrap();
    let project = match projects.get_mut(&project_id) {
        Some(p) => p,
        None => return 0,
    };
    let track = match project.tracks.get_mut(&track_id) {
        Some(t) => t,
        None => return 0,
    };
    let clip = Clip {
        id: 0,
        media_path: path,
        media_name: name,
        start_frame,
        duration_frames: if duration_frames == 0 { 150 } else { duration_frames },
        trim_in_frames: 0,
        volume: 1.0,
        opacity: 1.0,
        scale: 1.0,
    };
    track.add_clip(clip)
}

#[no_mangle]
pub extern "C" fn engine_remove_clip(
    engine: EngineHandle,
    project_id: u64,
    track_id: TrackId,
    clip_id: ClipId,
) -> bool {
    if engine.is_null() {
        return false;
    }
    let eng = unsafe { &*engine };
    let mut projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get_mut(&project_id) {
        if let Some(t) = p.tracks.get_mut(&track_id) {
            return t.remove_clip(clip_id);
        }
    }
    false
}

#[no_mangle]
pub extern "C" fn engine_clip_count(
    engine: EngineHandle,
    project_id: u64,
    track_id: TrackId,
) -> usize {
    if engine.is_null() {
        return 0;
    }
    let eng = unsafe { &*engine };
    let projects = eng.projects.lock().unwrap();
    projects
        .get(&project_id)
        .and_then(|p| p.tracks.get(&track_id))
        .map(|t| t.clips.len())
        .unwrap_or(0)
}

/// Returns a heap-allocated C string for the given track's name. The
/// caller owns the returned buffer and must free it with
/// [`engine_string_free`]. Returns null on failure.
#[no_mangle]
pub extern "C" fn engine_track_name(
    engine: EngineHandle,
    project_id: u64,
    track_id: TrackId,
) -> *mut c_char {
    if engine.is_null() {
        return ptr::null_mut();
    }
    let eng = unsafe { &*engine };
    let projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get(&project_id) {
        if let Some(t) = p.tracks.get(&track_id) {
            if let Ok(s) = CString::new(t.name.clone()) {
                return s.into_raw();
            }
        }
    }
    ptr::null_mut()
}

#[no_mangle]
pub extern "C" fn engine_string_free(s: *mut c_char) {
    if s.is_null() {
        return;
    }
    unsafe {
        drop(CString::from_raw(s));
    }
}

#[no_mangle]
pub extern "C" fn engine_track_kind(
    engine: EngineHandle,
    project_id: u64,
    track_id: TrackId,
) -> c_int {
    if engine.is_null() {
        return -1;
    }
    let eng = unsafe { &*engine };
    let projects = eng.projects.lock().unwrap();
    if let Some(p) = projects.get(&project_id) {
        if let Some(t) = p.tracks.get(&track_id) {
            return t.kind as c_int;
        }
    }
    -1
}

#[no_mangle]
pub extern "C" fn engine_probe_media(path: *const c_char) -> MediaKind {
    if path.is_null() {
        return MediaKind::Unknown;
    }
    let s = unsafe { CStr::from_ptr(path) }
        .to_string_lossy()
        .to_string();
    MediaAsset::from_path(&s).kind
}

#[no_mangle]
pub extern "C" fn engine_version() -> *mut c_char {
    let v = env!("CARGO_PKG_VERSION");
    CString::new(v).unwrap_or_default().into_raw()
}

/// Returns the version of the engine as a borrowed C string. Caller
/// must NOT free the returned pointer — it points at static storage.
#[no_mangle]
pub extern "C" fn engine_version_static() -> *const c_char {
    static VERSION: &[u8] = b"0.1.0\0";
    VERSION.as_ptr() as *const c_char
}

#[allow(dead_code)]
fn _unused(_: &ProjectId, _: &Track, _: &Clip) {}
