cmake_minimum_required(VERSION 3.21)
project(beta_editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ---- Qt6 ---------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)

# ---- Sources -----------------------------------------------------------
set(APP_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MediaBrowser.cpp
    src/PreviewWidget.cpp
    src/TimelineWidget.cpp
    src/PropertiesPanel.cpp
    src/EngineBridge.cpp
)

set(APP_HEADERS
    src/MainWindow.h
    src/MediaBrowser.h
    src/PreviewWidget.h
    src/TimelineWidget.h
    src/PropertiesPanel.h
    src/EngineBridge.h
)

add_executable(beta_editor WIN32
    ${APP_SOURCES}
    ${APP_HEADERS}
)

target_include_directories(beta_editor PRIVATE
    ${ENGINE_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link the Rust engine (imported static target) + Qt
target_link_libraries(beta_editor PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    beta_engine
)

# Rust's std pulls in NT primitives (NtReadFile, NtCreateFile, ...) on
# Windows. They live in ntdll.lib, which MSVC does not link by default.
if(WIN32)
    target_link_libraries(beta_editor PRIVATE ntdll ws2_32 userenv bcrypt advapi32)
endif()

# Windows: make sure we link against the C runtime statically if the
# Rust staticlib was built that way; on Linux/macOS nothing extra needed.
if(WIN32)
    set_target_properties(beta_editor PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    find_program(QT_DEPLOY_TOOL windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(QT_DEPLOY_TOOL)
        add_custom_command(TARGET beta_editor POST_BUILD
            COMMAND ${QT_DEPLOY_TOOL} --no-translations --no-system-d3d-compiler
                    --multimedia --release "$<TARGET_FILE:beta_editor>"
            COMMENT "Running windeployqt on beta_editor")
    endif()
endif()

install(TARGETS beta_editor RUNTIME DESTINATION bin)
