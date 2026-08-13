//! Lightweight media asset metadata. The engine never decodes media —
//! it only records what the UI told it about an imported file. Decoding
//! and playback are handled on the C++ side by QtMultimedia.

use std::path::Path;

#[derive(Debug, Clone)]
pub struct MediaAsset {
    pub path: String,
    pub name: String,
    pub kind: MediaKind,
    pub duration_frames: u64,
    pub width: u32,
    pub height: u32,
    pub fps: f64,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(C)]
pub enum MediaKind {
    Unknown = 0,
    Video = 1,
    Audio = 2,
    Image = 3,
}

impl MediaAsset {
    /// Probe a path and produce a best-effort [`MediaAsset`] from the
    /// file extension. The UI may later overwrite duration / dimensions
    /// once QtMultimedia has actually inspected the file.
    pub fn from_path(path: &str) -> Self {
        let p = Path::new(path);
        let name = p
            .file_name()
            .map(|s| s.to_string_lossy().to_string())
            .unwrap_or_else(|| path.to_string());

        let ext = p
            .extension()
            .map(|s| s.to_string_lossy().to_lowercase())
            .unwrap_or_default();

        let kind = match ext.as_str() {
            "mp4" | "mov" | "mkv" | "avi" | "webm" | "m4v" | "wmv" | "flv" => MediaKind::Video,
            "mp3" | "wav" | "aac" | "flac" | "ogg" | "m4a" | "wma" | "opus" => MediaKind::Audio,
            "png" | "jpg" | "jpeg" | "bmp" | "gif" | "webp" | "tiff" | "tga" => MediaKind::Image,
            _ => MediaKind::Unknown,
        };

        Self {
            path: path.to_string(),
            name,
            kind,
            duration_frames: 0,
            width: 0,
            height: 0,
            fps: 30.0,
        }
    }
}
