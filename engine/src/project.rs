//! Project model — a single video-editing project.

use crate::timeline::{Track, TrackId, TrackKind};
use std::collections::HashMap;

pub type ProjectId = u64;

/// A project owns its tracks and a few global settings (resolution, fps).
/// The actual media bytes are never loaded by the engine — only metadata
/// is stored so the engine stays lightweight.
pub struct Project {
    pub id: ProjectId,
    pub name: String,
    pub width: u32,
    pub height: u32,
    pub fps: f64,
    pub tracks: HashMap<TrackId, Track>,
    pub next_track_id: TrackId,
    pub duration_frames: u64,
}

impl Project {
    pub fn new(id: ProjectId, name: String) -> Self {
        Self {
            id,
            name,
            width: 1920,
            height: 1080,
            fps: 30.0,
            tracks: HashMap::new(),
            next_track_id: 1,
            duration_frames: 0,
        }
    }

    pub fn add_track(&mut self, kind: TrackKind, name: String) -> TrackId {
        let id = self.next_track_id;
        self.next_track_id += 1;
        let track = Track::new(id, kind, name);
        self.tracks.insert(id, track);
        id
    }

    pub fn remove_track(&mut self, id: TrackId) -> bool {
        self.tracks.remove(&id).is_some()
    }

    pub fn track(&mut self, id: TrackId) -> Option<&mut Track> {
        self.tracks.get_mut(&id)
    }

    pub fn tracks_sorted(&self) -> Vec<&Track> {
        let mut v: Vec<&Track> = self.tracks.values().collect();
        v.sort_by_key(|t| t.id);
        v
    }
}
