# Beta — Hybrid Video Editor (Rust + Qt6)

A modern, dark-themed video editor with a Rust core engine and a Qt6
desktop UI. Cross-platform binaries for Linux and Windows are produced
automatically by GitHub Actions on every push, with cached toolchains
for fast rebuilds.

> Repository: <https://github.com/salom600/beta>

## v0.2 highlights

- **Modern dark UI** — custom QSS theme, custom SVG icon set, Fusion
  palette, polished timeline rendering with gradients, drop shadows
  and per-track header colors.
- **Drag & drop media onto the timeline** — drag a file from the
  Media Browser (left panel) and drop it onto any track at the
  desired position.
- **Real media probing** — `MediaProber` uses QtMultimedia to read
  actual duration, frame rate and resolution for video/audio, and
  `QImageReader` for image dimensions. Clip duration auto-fills from
  the probed value.
- **Clip drag / trim** — click a clip body and drag to move it along
  the track; grab the left or right edge (8 px) to trim. Cursor
  changes to `SizeHor` / `SizeAll` to indicate which operation is
  available. Locked tracks are read-only.
- **Properties panel** — live-edit the selected clip's name, start
  frame, duration, trim-in, volume, opacity and scale; changes are
  written back to the engine immediately.
- **FFmpeg-based export** — `ExportDialog` exposes container / codec
  / resolution / bitrate settings; `Exporter` runs FFmpeg as a
  subprocess to re-encode each clip to a normalized intermediate
  and then concatenates them. Progress bar in the status bar.
- **CI caching** — `Swatinem/rust-cache@v2` for the Rust target dir,
  `actions/cache@v4` for apt packages and the Qt install, plus a
  cached FFmpeg download on Windows. Rebuilds are significantly
  faster after the first run.

## Architecture

```
+-----------------------------+        +----------------------------+
|        app/ (C++ / Qt6)     |        |     engine/ (Rust)         |
|                             |        |                            |
|  MainWindow                 |        |  lib.rs   Engine handle    |
|  MediaBrowser   ----+       |        |  project.rs  Project       |
|  MediaProber     ---+       |        |  timeline.rs Track/Clip    |
|  PreviewWidget   ---+---->  | FFI -> |  media.rs    MediaAsset    |
|  TimelineWidget   --+       |        |  ffi.rs      C ABI + JSON  |
|  PropertiesPanel   +        |        |  engine.h    C header      |
|  ExportDialog / Exporter    |        |                            |
|  EngineBridge (RAII)|       |        |                            |
+-----------------------------+        +----------------------------+
```

* `engine/` — Rust crate, builds as `staticlib + cdylib + rlib`.
  Exposes a small C ABI declared in `engine/engine.h`, plus a
  `engine_serialize_project` function that returns the full project
  state as JSON (parsed by `EngineBridge::snapshot()` for UI sync and
  for export).
* `app/` — Qt6 desktop app. Links the Rust static lib through
  `EngineBridge` (RAII wrapper around the C handle).
* `app/resources/` — Qt resource bundle (`beta.qrc`) containing the
  dark QSS stylesheet and a set of monochrome SVG icons.
* `.github/workflows/build.yml` — Linux + Windows CI with cached
  Rust / apt / Qt / FFmpeg, uploads artifacts on every push and
  publishes a GitHub release on `v*` tags.

## Build locally

### Linux

```bash
sudo apt install qt6-base-dev qt6-multimedia-dev build-essential cmake ninja-build ffmpeg
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/app/beta_editor
```

### Windows

* Install Visual Studio 2022 (Desktop C++ workload), Qt 6.6+ with
  `qtmultimedia`, Rust (MSVC toolchain), CMake, Ninja, and FFmpeg
  (place `ffmpeg.exe` on PATH or set `BETA_FFMPEG` env var).
* In a *Developer Command Prompt for VS 2022*:

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
build\app\beta_editor.exe
```

## Usage

1. **Import media** — `File → Import Media...` or `Ctrl+I`, or drag
   files into the Media Browser.
2. **Add tracks** — toolbar buttons or `Track` menu.
3. **Drop media onto timeline** — drag an item from the Media Browser
   and drop it on a track. The clip's duration comes from real media
   probing.
4. **Move / trim clips** — drag a clip body to move; drag the left or
   right edge (8 px) to trim.
5. **Edit properties** — click a clip to load it into the right
   panel; spinboxes write back to the engine immediately.
6. **Preview** — double-click a media item to load it into the
   preview; `Space` to play/pause.
7. **Export** — `File → Export...` or `Ctrl+E`; pick container,
   codecs, resolution, bitrate. FFmpeg runs in the background with a
   progress bar in the status bar.

## Layout

```
beta/
├── .github/workflows/build.yml
├── CMakeLists.txt
├── engine/
│   ├── Cargo.toml
│   ├── engine.h
│   └── src/{lib,ffi,project,timeline,media}.rs
└── app/
    ├── CMakeLists.txt
    ├── resources/
    │   ├── beta.qrc
    │   ├── dark.qss
    │   └── icons/*.svg
    └── src/
        ├── main.cpp
        ├── MainWindow.{h,cpp}
        ├── EngineBridge.{h,cpp}
        ├── MediaBrowser.{h,cpp}
        ├── MediaProber.{h,cpp}
        ├── PreviewWidget.{h,cpp}
        ├── TimelineWidget.{h,cpp}
        ├── PropertiesPanel.{h,cpp}
        ├── ExportDialog.{h,cpp}
        └── Exporter.{h,cpp}
```

## License

MIT.
