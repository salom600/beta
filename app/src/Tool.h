#pragma once

#include <QObject>

namespace beta {

/// Editing tools, mirroring Kdenlive's `ToolType::ProjectTool` enum:
///   https://github.com/KDE/kdenlive/blob/master/src/definitions.h
///
/// Kdenlive defines 8 tools (Roll/Slide are reserved enums even
/// though the actions are commented out as TODO). We mirror this so
/// our tool palette matches what professional NLE users expect.
class Tool {
    Q_GADGET
public:
    enum Kind {
        SelectTool   = 0,   // S — default; click to select, drag to move, edges to trim
        RazorTool    = 1,   // X — click on a clip to split it at that point
        SpacerTool   = 2,   // M — drag to move a range of clips together
        RippleTool   = 3,   // like Select, but trims ripple (no gap left)
        RollTool     = 4,   // trim two adjacent clips at the cut (reserved)
        SlipTool     = 5,   // move in/out without moving position (reserved)
        SlideTool    = 6,   // move position without changing in/out (reserved)
        MulticamTool = 7,   // multi-camera switching (reserved)
    };
    Q_ENUM(Kind)
};

} // namespace beta
