cmake_minimum_required(VERSION 3.21)
project(beta VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Detect Rust
find_program(CARGO_CMD cargo REQUIRED)
if(NOT CARGO_CMD)
    message(FATAL_ERROR "cargo not found — install Rust toolchain (https://rustup.rs).")
endif()

# Build the Rust static library via cargo, then expose it as a CMake
# imported target. The static lib name differs by platform.
if(WIN32)
    set(RUST_STATIC_LIB "${CMAKE_BINARY_DIR}/engine/beta_engine.lib")
    set(RUST_LIB_NAME   "beta_engine")
    set(RUST_TARGET     "x86_64-pc-windows-msvc")
else()
    set(RUST_STATIC_LIB "${CMAKE_BINARY_DIR}/engine/libbeta_engine.a")
    set(RUST_LIB_NAME   "beta_engine")
    set(RUST_TARGET     "")
endif()

set(RUST_BUILD_DIR "${CMAKE_BINARY_DIR}/engine")

# We invoke cargo manually so the host compiler and the C++ build stay
# in sync. The output goes to ${RUST_BUILD_DIR} so we can locate it
# from the app/CMakeLists.txt.
add_custom_target(beta_engine_build ALL
    COMMAND ${CARGO_CMD} build
            --release
            --manifest-path ${CMAKE_SOURCE_DIR}/engine/Cargo.toml
            --target-dir    ${RUST_BUILD_DIR}
    BYPRODUCTS
        "${RUST_BUILD_DIR}/release/${RUST_LIB_NAME}.lib"
        "${RUST_BUILD_DIR}/release/lib${RUST_LIB_NAME}.a"
        "${RUST_BUILD_DIR}/release/${RUST_LIB_NAME}.dll"
        "${RUST_BUILD_DIR}/release/lib${RUST_LIB_NAME}.so"
        "${RUST_BUILD_DIR}/release/lib${RUST_LIB_NAME}.dylib"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/engine
    COMMENT "Building Rust engine (release)"
    VERBATIM
)

# Imported target that the app links against.
add_library(beta_engine STATIC IMPORTED)
set_target_properties(beta_engine PROPERTIES
    IMPORTED_LOCATION
        "${RUST_BUILD_DIR}/release/${RUST_STATIC_NAME}")
# On Windows, MSVC names the staticlib beta_engine.lib; on Linux/macOS
# the name is libbeta_engine.a. Provide both locations so CMake can
# pick whichever one exists.
set_property(TARGET beta_engine APPEND PROPERTY
    IMPORTED_CONFIGURATIONS RELEASE)

if(WIN32)
    set_target_properties(beta_engine PROPERTIES
        IMPORTED_LOCATION_RELEASE
            "${RUST_BUILD_DIR}/release/beta_engine.lib"
        IMPORTED_LOCATION
            "${RUST_BUILD_DIR}/release/beta_engine.lib")
else()
    set_target_properties(beta_engine PROPERTIES
        IMPORTED_LOCATION_RELEASE
            "${RUST_BUILD_DIR}/release/libbeta_engine.a"
        IMPORTED_LOCATION
            "${RUST_BUILD_DIR}/release/libbeta_engine.a")
endif()

add_dependencies(beta_engine beta_engine_build)

# Tell the app where to find the engine lib + headers.
set(ENGINE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/engine")
set(ENGINE_LIB_DIR     "${RUST_BUILD_DIR}/release")

add_subdirectory(app)
