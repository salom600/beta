//! Timeline primitives: tracks, clips, track kinds.

use std::collections::HashMap;

pub type TrackId = u64;
pub type ClipId = u64;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(C)]
pub enum TrackKind {
    Video = 0,
    Audio = 1,
    Image = 2,
}

impl TrackKind {
    pub fn from_i32(v: i32) -> Option<Self> {
        match v {
            0 => Some(TrackKind::Video),
            1 => Some(TrackKind::Audio),
            2 => Some(TrackKind::Image),
            _ => None,
        }
    }
}

/// Per-track visibility / lock state, mirrored on the UI as:
/// - Eye button (video/image tracks)
/// - Mute button (audio tracks)
/// - Lock button (any track)
#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub struct TrackState {
    pub visible: bool,
    pub locked: bool,
    pub muted: bool,
}

impl Default for TrackState {
    fn default() -> Self {
        Self {
            visible: true,
            locked: false,
            muted: false,
        }
    }
}

#[derive(Debug, Clone)]
pub struct Track {
    pub id: TrackId,
    pub kind: TrackKind,
    pub name: String,
    pub state: TrackState,
    pub clips: HashMap<ClipId, Clip>,
    pub next_clip_id: ClipId,
}

impl Track {
    pub fn new(id: TrackId, kind: TrackKind, name: String) -> Self {
        Self {
            id,
            kind,
            name,
            state: TrackState::default(),
            clips: HashMap::new(),
            next_clip_id: 1,
        }
    }

    pub fn add_clip(&mut self, mut clip: Clip) -> ClipId {
        let id = self.next_clip_id;
        self.next_clip_id += 1;
        clip.id = id;
        self.clips.insert(id, clip);
        id
    }

    pub fn remove_clip(&mut self, id: ClipId) -> bool {
        self.clips.remove(&id).is_some()
    }

    pub fn clips_sorted(&self) -> Vec<&Clip> {
        let mut v: Vec<&Clip> = self.clips.values().collect();
        v.sort_by_key(|c| c.start_frame);
        v
    }
}

#[derive(Debug, Clone)]
pub struct Clip {
    pub id: ClipId,
    pub media_path: String,
    pub media_name: String,
    pub start_frame: u64,
    pub duration_frames: u64,
    pub trim_in_frames: u64,
    pub volume: f32,
    pub opacity: f32,
    pub scale: f32,
}

impl Default for Clip {
    fn default() -> Self {
        Self {
            id: 0,
            media_path: String::new(),
            media_name: String::new(),
            start_frame: 0,
            duration_frames: 150,
            trim_in_frames: 0,
            volume: 1.0,
            opacity: 1.0,
            scale: 1.0,
        }
    }
}
