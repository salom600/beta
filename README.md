# Beta — Hybrid Video Editor (Rust + Qt6)

A small but complete video editing program. The engine / project model
is written in Rust (compiled to a static library) and the UI is C++ /
Qt6. Builds natively on Linux and Windows via CMake + Cargo, with
GitHub Actions producing downloadable executables on every push and
release ZIPs on every tag.

> Repository: <https://github.com/salom600/beta>

## Architecture

```
+-----------------------------+        +----------------------------+
|        app/ (C++ / Qt6)     |        |     engine/ (Rust)         |
|                             |        |                            |
|  MainWindow                 |        |  lib.rs   Engine handle    |
|  MediaBrowser   ----+       |        |  project.rs  Project       |
|  PreviewWidget   ----+----> | FFI -> |  timeline.rs Track/Clip    |
|  TimelineWidget   ---+      |        |  media.rs    MediaAsset    |
|  PropertiesPanel  ---+      |        |  ffi.rs      C ABI         |
|  EngineBridge (RAII)|       |        |  engine.h    C header      |
+-----------------------------+        +----------------------------+
```

* `engine/` — Rust crate. Builds as `staticlib` + `cdylib`. Exposes a
  small C ABI declared in `engine/engine.h`.
* `app/` — C++ / Qt6 desktop application. Links the Rust static lib via
  `EngineBridge` (RAII wrapper around the C handle).
* `.github/workflows/build.yml` — CI that builds the app on Linux +
  Windows, uploads build artifacts on every push, and publishes a
  GitHub release whenever a `v*` tag is pushed.

## Core features (current)

* Import video / audio / image media (toolbar Import button or `Ctrl+I`)
* Preview panel with QtMultimedia playback + transport controls
* Multi-track timeline:
  * Video, Audio and Image tracks
  * Per-track Eye / Mute / Lock controls (click the colored squares in
    the track header)
  * Manual clip blocks rendered with `paintEvent`
  * Clickable ruler to scrub the playhead
  * `Ctrl + wheel` to zoom the timeline
* Properties panel showing clip / media / project properties
* Status bar with engine version

## Build locally

### Linux

```bash
sudo apt install qt6-base-dev qt6-multimedia-dev build-essential cmake ninja-build
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/app/beta_editor
```

### Windows

* Install Visual Studio 2022 (with the *Desktop development with C++*
  workload), Qt 6.6+ (with `qtmultimedia`), Rust (MSVC toolchain), CMake,
  Ninja.
* Then in a *Developer Command Prompt for VS 2022*:

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
build\app\beta_editor.exe
```

## CI artifacts

Every push to `main` produces two downloadable artifacts on the Actions
run page:

* `beta-editor-linux-x86_64.tar.gz`
* `beta-editor-windows-x86_64.zip`

Tagging a release (`git tag v0.1.0 && git push --tags`) publishes a
GitHub release with both archives attached.

## Layout

```
beta/
├── .github/workflows/build.yml   # cross-platform CI + release
├── CMakeLists.txt                # top-level: builds Rust + app
├── engine/                       # Rust engine (staticlib)
│   ├── Cargo.toml
│   ├── engine.h                  # C ABI header
│   └── src/
│       ├── lib.rs
│       ├── ffi.rs                # extern "C" surface
│       ├── project.rs
│       ├── timeline.rs
│       └── media.rs
└── app/                          # C++ / Qt6 application
    ├── CMakeLists.txt
    └── src/
        ├── main.cpp
        ├── MainWindow.{h,cpp}
        ├── EngineBridge.{h,cpp}  # RAII wrapper over engine.h
        ├── MediaBrowser.{h,cpp}
        ├── PreviewWidget.{h,cpp}
        ├── TimelineWidget.{h,cpp}
        └── PropertiesPanel.{h,cpp}
```

## License

MIT.
