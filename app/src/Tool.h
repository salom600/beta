#pragma once

#include <QObject>

namespace beta {

/// Editing tools, mirroring the standard NLE convention used by
/// Kdenlive, DaVinci Resolve, and Premiere Pro:
///   - Select  (V): default — click to select, drag to move, edges to trim
///   - Razor    (C): click on a clip to split it at that point
///   - Spacer   (T): drag to move a range of clips together
///   - Hand     (H): drag to pan the timeline view
class Tool {
    Q_GADGET
public:
    enum Kind {
        Select = 0,
        Razor  = 1,
        Spacer = 2,
        Hand   = 3,
    };
    Q_ENUM(Kind)
};

} // namespace beta
