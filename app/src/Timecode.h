#pragma once

#include <QString>
#include <cstdint>

namespace beta {

/// Timecode helpers. Internally the engine works in frames; the UI
/// displays timecodes in HH:MM:SS:FF format based on the project fps.
namespace Timecode {

inline QString fromFrames(uint64_t frames, double fps)
{
    if (fps <= 0) fps = 30.0;
    int ifps = (fps + 0.5);
    if (ifps <= 0) ifps = 30;
    uint64_t totalSec = frames / ifps;
    int ff = int(frames % ifps);
    int ss = int(totalSec % 60);
    int mm = int((totalSec / 60) % 60);
    int hh = int(totalSec / 3600);
    return QString::asprintf("%02d:%02d:%02d:%02d", hh, mm, ss, ff);
}

inline QString fromMs(qint64 ms, double fps)
{
    if (fps <= 0) fps = 30.0;
    double frames = ms * fps / 1000.0;
    return fromFrames(static_cast<uint64_t>(frames + 0.5), fps);
}

inline uint64_t toFrames(int hh, int mm, int ss, int ff, double fps)
{
    if (fps <= 0) fps = 30.0;
    int ifps = int(fps + 0.5);
    return uint64_t(hh) * 3600 * ifps + uint64_t(mm) * 60 * ifps +
           uint64_t(ss) * ifps + uint64_t(ff);
}

} // namespace Timecode

} // namespace beta
