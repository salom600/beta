//! Timeline primitives: tracks, clips, track kinds.

use serde::{Serialize, Serializer};
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

    pub fn as_str(&self) -> &'static str {
        match self {
            TrackKind::Video => "video",
            TrackKind::Audio => "audio",
            TrackKind::Image => "image",
        }
    }
}

impl Serialize for TrackKind {
    fn serialize<S: Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
        s.serialize_str(self.as_str())
    }
}

/// Per-track visibility / lock state.
#[derive(Debug, Clone, Copy, Serialize)]
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

#[derive(Debug, Clone, Serialize)]
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

/// Color & transform adjustment for a clip. Defaults are neutral.
#[derive(Debug, Clone, Copy, Serialize)]
#[repr(C)]
pub struct ClipAdjust {
    pub brightness: f32,   // -1.0 .. +1.0  (0 = neutral)
    pub contrast:   f32,   // -1.0 .. +1.0  (0 = neutral)
    pub saturation: f32,   // -1.0 .. +1.0  (0 = neutral)
    pub hue:        f32,   // -180 .. +180  (0 = neutral)
    pub pos_x:      f32,   // -1.0 .. +1.0  (offset relative to frame width)
    pub pos_y:      f32,   // -1.0 .. +1.0
    pub scale:      f32,   // 0.1 .. 4.0
    pub rotation:   f32,   // degrees
    pub speed:      f32,   // 0.25 .. 4.0  (1.0 = normal)
    pub fade_in:    u64,   // frames
    pub fade_out:   u64,   // frames
    pub volume:     f32,   // 0.0 .. 2.0
    pub opacity:    f32,   // 0.0 .. 1.0
}

impl Default for ClipAdjust {
    fn default() -> Self {
        Self {
            brightness: 0.0,
            contrast: 0.0,
            saturation: 0.0,
            hue: 0.0,
            pos_x: 0.0,
            pos_y: 0.0,
            scale: 1.0,
            rotation: 0.0,
            speed: 1.0,
            fade_in: 0,
            fade_out: 0,
            volume: 1.0,
            opacity: 1.0,
        }
    }
}

#[derive(Debug, Clone, Serialize)]
pub struct Clip {
    pub id: ClipId,
    pub media_path: String,
    pub media_name: String,
    pub start_frame: u64,
    pub duration_frames: u64,
    pub trim_in_frames: u64,
    pub adjust: ClipAdjust,
    #[serde(skip)]
    pub media_width: u32,
    #[serde(skip)]
    pub media_height: u32,
    #[serde(skip)]
    pub media_duration_frames: u64,
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
            adjust: ClipAdjust::default(),
            media_width: 0,
            media_height: 0,
            media_duration_frames: 0,
        }
    }
}
