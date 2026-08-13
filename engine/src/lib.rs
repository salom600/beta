//! Beta video editor engine
//!
//! Holds the project model: tracks (video / audio / image), clips, and
//! media asset metadata. The engine is intentionally dependency-free so it
//! can be compiled into a small static library that the C++ UI links
//! against through the C ABI exposed in [`ffi`].

pub mod ffi;
pub mod media;
pub mod project;
pub mod timeline;

pub use project::{Project, ProjectId};
pub use timeline::{Clip, ClipId, Track, TrackKind, TrackId};
pub use media::MediaAsset;

use std::sync::Mutex;
use std::collections::HashMap;

/// Top-level engine handle. Holds every open project and provides a stable
/// integer id for each one so the C side never has to deal with raw pointers
/// to Rust data structures.
pub struct Engine {
    pub(crate) projects: Mutex<HashMap<u64, Project>>,
    pub(crate) next_id: Mutex<u64>,
}

impl Engine {
    pub fn new() -> Self {
        Self {
            projects: Mutex::new(HashMap::new()),
            next_id: Mutex::new(1),
        }
    }

    pub fn create_project(&self, name: String) -> u64 {
        let id = {
            let mut next = self.next_id.lock().unwrap();
            let v = *next;
            *next += 1;
            v
        };
        let project = Project::new(id, name);
        self.projects.lock().unwrap().insert(id, project);
        id
    }

    pub fn project(&self, id: u64) -> Option<std::sync::MutexGuard<'_, HashMap<u64, Project>>> {
        let guard = self.projects.lock().unwrap();
        if guard.contains_key(&id) {
            drop(guard);
            Some(self.projects.lock().unwrap())
        } else {
            None
        }
    }

    pub fn close_project(&self, id: u64) -> bool {
        self.projects.lock().unwrap().remove(&id).is_some()
    }
}

impl Default for Engine {
    fn default() -> Self {
        Self::new()
    }
}
