#include "TimelineWidget.h"
#include "Commands.h"
#include "EngineBridge.h"
#include "MediaProber.h"
#include "Project.h"
#include "Timecode.h"
#include "UndoHelper.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPoint>
#include <QRect>
#include <QStyleOption>
#include <QTimer>
#include <QUrl>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace beta {

namespace {
const QColor CLR_BG         ( 24,  25,  28);
const QColor CLR_ALT_BG     ( 30,  31,  34);
const QColor CLR_HEADER_BG  ( 38,  39,  44);
const QColor CLR_HEADER_VID ( 32,  55, 100);
const QColor CLR_HEADER_AUD ( 60,  42,  95);
const QColor CLR_HEADER_IMG (100,  68,  36);
const QColor CLR_RULER      ( 20,  21,  24);
const QColor CLR_GRID       ( 48,  49,  56);
const QColor CLR_TEXT       (220, 221, 226);
const QColor CLR_TEXT_DIM   (130, 131, 138);
const QColor CLR_PLAYHEAD   (255, 110, 110);
const QColor CLR_CLIP_VID   ( 60, 130, 200);
const QColor CLR_CLIP_AUD   (140,  90, 200);
const QColor CLR_CLIP_IMG   (220, 150,  60);
const QColor CLR_SELECTED   (255, 200,  90);
const QColor CLR_BTN_ON     ( 14,  99, 212);
const QColor CLR_BTN_OFF    ( 55,  56,  64);
const QColor CLR_SNAP       (255, 255, 150);
}

TimelineWidget::TimelineWidget(QWidget* parent, EngineBridge* engine, uint64_t projectId)
    : QWidget(parent), engine_(engine), projectId_(projectId)
{
    setMinimumHeight(320);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    snapModel_ = std::make_unique<SnapModel>();

    playheadTimer_ = new QTimer(this);
    playheadTimer_->setInterval(33);
    connect(playheadTimer_, &QTimer::timeout, this, &TimelineWidget::onPlayheadTimer);

    refreshTracks();
}

void TimelineWidget::setProber(MediaProber* prober) { prober_ = prober; }
void TimelineWidget::setProject(Project* project) { project_ = project; }

void TimelineWidget::setProjectId(uint64_t id)
{
    projectId_ = id;
    refreshTracks();
}

void TimelineWidget::refreshTracks()
{
    tracks_.clear();
    EngineBridge::ProjectSnapshot snap = engine_->snapshot(projectId_);
    for (const auto& t : snap.tracks) {
        TrackRow row;
        row.id   = t.id;
        row.kind = t.kind;
        row.name = t.name;
        row.visible = t.state.visible;
        row.locked  = t.state.locked;
        row.muted   = t.state.muted;
        for (const auto& c : t.clips) {
            TrackRow::Clip clip;
            clip.id                = c.id;
            clip.name              = c.mediaName;
            clip.path              = c.mediaPath;
            clip.startFrame        = c.startFrame;
            clip.durationFrames    = c.durationFrames;
            clip.trimInFrames      = c.trimInFrames;
            clip.adjust            = c.adjust;
            clip.mediaWidth        = c.mediaWidth;
            clip.mediaHeight       = c.mediaHeight;
            clip.mediaDurationFrames = c.mediaDurationFrames;
            row.clips.append(clip);
        }
        std::sort(row.clips.begin(), row.clips.end(),
                  [](const TrackRow::Clip& a, const TrackRow::Clip& b) {
                      return a.startFrame < b.startFrame;
                  });
        tracks_.append(row);
    }

    // Rebuild the snap model from all clip start/end points (refcounted)
    if (snapModel_) {
        snapModel_->clear();
        for (const auto& row : tracks_) {
            for (const auto& c : row.clips) {
                snapModel_->addPoint(static_cast<int>(c.startFrame));
                snapModel_->addPoint(static_cast<int>(c.startFrame + c.durationFrames));
            }
        }
    }

    update();
    emit timelineChanged();
}

QSize TimelineWidget::sizeHint() const { return QSize(1000, 380); }
QSize TimelineWidget::minimumSizeHint() const { return QSize(700, 280); }

int TimelineWidget::totalHeight() const
{
    return rulerHeight_ + tracks_.size() * rowHeight_;
}

int TimelineWidget::xForFrame(uint64_t frame) const
{
    return headerWidth_ + static_cast<int>(frame) * pixelsPerFrame_;
}

int TimelineWidget::frameAt(int x) const
{
    if (x < headerWidth_) return 0;
    return (x - headerWidth_) / pixelsPerFrame_;
}

int TimelineWidget::rowAt(int y) const
{
    if (y < rulerHeight_) return -1;
    int r = (y - rulerHeight_) / rowHeight_;
    if (r < 0 || r >= tracks_.size()) return -1;
    return r;
}

QRect TimelineWidget::trackHeaderRect(int row) const
{
    return QRect(0, rulerHeight_ + row * rowHeight_, headerWidth_, rowHeight_);
}

QRect TimelineWidget::trackBodyRect(int row) const
{
    int y = rulerHeight_ + row * rowHeight_;
    return QRect(headerWidth_, y, width() - headerWidth_, rowHeight_);
}

QRect TimelineWidget::clipRect(int row, const TrackRow::Clip& c) const
{
    int x = xForFrame(c.startFrame);
    int w = static_cast<int>(c.durationFrames) * pixelsPerFrame_;
    int y = rulerHeight_ + row * rowHeight_;
    return QRect(x, y + 4, w, rowHeight_ - 8);
}

uint64_t TimelineWidget::snapFrame(uint64_t frame, int threshold) const
{
    // Snap to playhead first
    if (frame >= playheadFrame_ - threshold && frame <= playheadFrame_ + threshold)
        return playheadFrame_;

    // Use the refcounted SnapModel for clip edges + ruler marks
    if (snapModel_ && !snapModel_->isEmpty()) {
        int pos = static_cast<int>(frame);
        int closest = snapModel_->getClosestPoint(pos);
        if (closest >= 0 && std::abs(closest - pos) <= threshold) {
            return static_cast<uint64_t>(closest);
        }
    }

    // Snap to ruler second marks (every 30 frames at 30fps)
    int ifps = 30;
    uint64_t snap = (frame / ifps) * ifps;
    if (frame >= snap - threshold && frame <= snap + threshold)
        return snap;
    snap += ifps;
    if (frame >= snap - threshold && frame <= snap + threshold)
        return snap;

    return frame;
}

void TimelineWidget::jumpToNextCut()
{
    uint64_t target = playheadFrame_ + 1;
    uint64_t best = UINT64_MAX;
    for (const auto& row : tracks_) {
        for (const auto& c : row.clips) {
            if (c.startFrame >= target && c.startFrame < best) best = c.startFrame;
            uint64_t end = c.startFrame + c.durationFrames;
            if (end >= target && end < best) best = end;
        }
    }
    if (best != UINT64_MAX) {
        playheadFrame_ = best;
        update();
        emit playheadMoved(playheadFrame_);
    }
}

void TimelineWidget::jumpToPrevCut()
{
    uint64_t target = playheadFrame_;
    uint64_t best = 0;
    bool found = false;
    for (const auto& row : tracks_) {
        for (const auto& c : row.clips) {
            if (c.startFrame < target && c.startFrame > best) {
                best = c.startFrame;
                found = true;
            }
            uint64_t end = c.startFrame + c.durationFrames;
            if (end < target && end > best) {
                best = end;
                found = true;
            }
        }
    }
    if (found) {
        playheadFrame_ = best;
        update();
        emit playheadMoved(playheadFrame_);
    }
}

void TimelineWidget::toggleVisible(uint64_t trackId)
{
    for (auto& t : tracks_) {
        if (t.id == trackId) {
            EngineBridge::TrackState oldState = { t.visible, t.locked, t.muted };
            EngineBridge::TrackState newState = { !t.visible, t.locked, t.muted };
            TimelineFunctions::setTrackState(engine_, projectId_, trackId,
                                              oldState, newState, this);
            return;
        }
    }
}

void TimelineWidget::toggleMute(uint64_t trackId)
{
    for (auto& t : tracks_) {
        if (t.id == trackId) {
            EngineBridge::TrackState oldState = { t.visible, t.locked, t.muted };
            EngineBridge::TrackState newState = { t.visible, t.locked, !t.muted };
            TimelineFunctions::setTrackState(engine_, projectId_, trackId,
                                              oldState, newState, this);
            return;
        }
    }
}

void TimelineWidget::toggleLock(uint64_t trackId)
{
    for (auto& t : tracks_) {
        if (t.id == trackId) {
            EngineBridge::TrackState oldState = { t.visible, t.locked, t.muted };
            EngineBridge::TrackState newState = { t.visible, !t.locked, t.muted };
            TimelineFunctions::setTrackState(engine_, projectId_, trackId,
                                              oldState, newState, this);
            return;
        }
    }
}

std::pair<int, int> TimelineWidget::clipAt(const QPoint& pos) const
{
    int r = rowAt(pos.y());
    if (r < 0) return {-1, -1};
    const TrackRow& row = tracks_[r];
    for (int i = 0; i < row.clips.size(); ++i) {
        QRect cr = clipRect(r, row.clips[i]);
        if (cr.contains(pos)) return {r, i};
    }
    return {-1, -1};
}

std::pair<int, int> TimelineWidget::clipUnderPlayhead() const
{
    for (int r = 0; r < tracks_.size(); ++r) {
        for (int i = 0; i < tracks_[r].clips.size(); ++i) {
            const auto& c = tracks_[r].clips[i];
            if (playheadFrame_ >= c.startFrame &&
                playheadFrame_ <  c.startFrame + c.durationFrames) {
                return {r, i};
            }
        }
    }
    return {-1, -1};
}

bool TimelineWidget::splitAtPlayhead()
{
    auto [r, i] = clipUnderPlayhead();
    if (r < 0 || i < 0) return false;
    if (tracks_[r].locked) return false;
    const auto& c = tracks_[r].clips[i];
    return TimelineFunctions::splitClip(engine_, projectId_, tracks_[r].id,
                                         c.id, playheadFrame_, this);
}

bool TimelineWidget::deleteSelectedClip()
{
    if (selectedRow_ < 0 || selectedClipIdx_ < 0) {
        auto [r, i] = clipUnderPlayhead();
        if (r < 0 || i < 0) return false;
        selectedRow_ = r;
        selectedClipIdx_ = i;
    }
    if (selectedRow_ >= tracks_.size()) return false;
    if (tracks_[selectedRow_].locked) return false;
    const auto& c = tracks_[selectedRow_].clips[selectedClipIdx_];
    bool ok = TimelineFunctions::removeClip(engine_, projectId_,
                                             tracks_[selectedRow_].id, c.id, this);
    if (ok) {
        selectedRow_ = -1;
        selectedClipIdx_ = -1;
    }
    return ok;
}

bool TimelineWidget::mergeSelectedWithNext()
{
    if (selectedRow_ < 0 || selectedClipIdx_ < 0) return false;
    if (selectedRow_ >= tracks_.size()) return false;
    if (tracks_[selectedRow_].clips.size() < 2) return false;
    if (tracks_[selectedRow_].locked) return false;

    const auto& left = tracks_[selectedRow_].clips[selectedClipIdx_];
    if (selectedClipIdx_ + 1 >= tracks_[selectedRow_].clips.size()) return false;
    const auto& right = tracks_[selectedRow_].clips[selectedClipIdx_ + 1];
    return TimelineFunctions::mergeClips(engine_, projectId_, tracks_[selectedRow_].id,
                                          left.id, right.id, this);
}

void TimelineWidget::setZoom(int ppf)
{
    pixelsPerFrame_ = std::clamp(ppf, 1, 32);
    update();
}

TimelineWidget::DragMode
TimelineWidget::hitTest(const QPoint& pos, int* outRow, int* outClipIdx) const
{
    *outRow = -1;
    *outClipIdx = -1;
    int r = rowAt(pos.y());
    if (r < 0) return DragMode::None;
    if (pos.x() < headerWidth_) return DragMode::None;
    *outRow = r;
    const TrackRow& row = tracks_[r];
    for (int i = 0; i < row.clips.size(); ++i) {
        const auto& c = row.clips[i];
        QRect cr = clipRect(r, c);
        if (cr.contains(pos)) {
            *outClipIdx = i;
            if (pos.x() - cr.x() <= trimHandleWidth_) return DragMode::TrimClipLeft;
            if (cr.x() + cr.width() - pos.x() <= trimHandleWidth_) return DragMode::TrimClipRight;
            return DragMode::MoveClip;
        }
    }
    return DragMode::None;
}

void TimelineWidget::onPlayheadTimer()
{
    ++playheadFrame_;
    update();
    emit playheadMoved(playheadFrame_);
}

void TimelineWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), CLR_BG);

    QFont labelFont = font();
    labelFont.setPointSize(9);
    QFont rulerFont = font();
    rulerFont.setPointSize(8);
    QFontMetrics fm(labelFont);

    // Ruler
    QRect rulerRect(headerWidth_, 0, width() - headerWidth_, rulerHeight_);
    p.fillRect(rulerRect, CLR_RULER);
    p.fillRect(0, 0, headerWidth_, rulerHeight_, CLR_HEADER_BG);

    int frameStep = 30;
    p.setPen(QPen(CLR_GRID, 1));
    for (int f = 0; xForFrame(f) < width(); f += frameStep) {
        int x = xForFrame(f);
        p.drawLine(x, rulerHeight_ - 8, x, rulerHeight_);
        if (f % (frameStep * 2) == 0) {
            p.setPen(CLR_TEXT_DIM);
            p.setFont(rulerFont);
            p.drawText(x + 4, rulerHeight_ - 10,
                       Timecode::fromFrames(f, 30.0).mid(3, 5));  // MM:SS
            p.setPen(CLR_GRID);
        }
    }

    p.setPen(QPen(CLR_GRID, 1));
    p.drawLine(0, rulerHeight_, width(), rulerHeight_);
    p.drawLine(headerWidth_, 0, headerWidth_, height());

    // Tool indicator
    p.setPen(CLR_TEXT_DIM);
    QFont toolFont = font();
    toolFont.setPointSize(8);
    p.setFont(toolFont);
    QString toolName;
    switch (tool_) {
        case Tool::SelectTool:   toolName = "SELECT";  break;
        case Tool::RazorTool:    toolName = "RAZOR";   break;
        case Tool::SpacerTool:   toolName = "SPACER";  break;
        case Tool::RippleTool:   toolName = "RIPPLE";  break;
        case Tool::RollTool:     toolName = "ROLL";    break;
        case Tool::SlipTool:     toolName = "SLIP";    break;
        case Tool::SlideTool:    toolName = "SLIDE";   break;
        case Tool::MulticamTool: toolName = "MULTICAM"; break;
    }
    p.drawText(QRect(8, 0, headerWidth_ - 16, rulerHeight_),
               Qt::AlignVCenter | Qt::AlignLeft, toolName);

    if (tracks_.isEmpty()) {
        p.setPen(CLR_TEXT_DIM);
        QFont fnt = font();
        fnt.setPointSize(11);
        fnt.setItalic(true);
        p.setFont(fnt);
        p.drawText(rect().adjusted(0, rulerHeight_, 0, 0), Qt::AlignCenter,
                   tr("No tracks yet.\nUse Track → Add Video/Audio/Image Track,\n"
                      "then drag media from the Project Bin onto the timeline."));
        return;
    }

    for (int i = 0; i < tracks_.size(); ++i) {
        const TrackRow& row = tracks_[i];
        QRect hdr  = trackHeaderRect(i);
        QRect body = trackBodyRect(i);

        QColor hdrBg = (row.kind == 0) ? CLR_HEADER_VID :
                       (row.kind == 1) ? CLR_HEADER_AUD :
                                          CLR_HEADER_IMG;
        if (!row.visible) hdrBg = hdrBg.darker(180);
        p.fillRect(hdr, hdrBg);

        QColor bodyBg = (i % 2 == 0) ? CLR_BG : CLR_ALT_BG;
        if (!row.visible) bodyBg = bodyBg.darker(140);
        p.fillRect(body, bodyBg);

        p.setPen(QPen(CLR_GRID, 1));
        for (int f = 0; xForFrame(f) < width(); f += frameStep) {
            int x = xForFrame(f);
            p.drawLine(x, body.top(), x, body.bottom());
        }

        p.setPen(QPen(QColor(0, 0, 0, 160), 1));
        p.drawRect(hdr.adjusted(0, 0, -1, -1));
        p.setPen(QPen(CLR_GRID, 1));
        p.drawLine(body.right(), body.top(), body.right(), body.bottom());
        p.drawLine(0, hdr.bottom(), width(), hdr.bottom());

        p.setPen(QColor(255, 255, 255, 230));
        p.setFont(labelFont);
        QString label = row.name;
        if (fm.horizontalAdvance(label) > hdr.width() - 100) {
            label = fm.elidedText(label, Qt::ElideRight, hdr.width() - 100);
        }
        p.drawText(hdr.adjusted(10, 6, 100, -6),
                   Qt::AlignVCenter | Qt::AlignLeft, label);

        int btnSize = 22;
        int btnY = hdr.center().y() - btnSize / 2;

        auto drawBtn = [&](const QRect& btn, bool on, const QString& iconOn, const QString& iconOff) {
            p.setBrush(on ? CLR_BTN_ON : CLR_BTN_OFF);
            p.setPen(QPen(QColor(0, 0, 0, 200), 1));
            p.drawRoundedRect(btn, 4, 4);
            p.drawPixmap(btn.adjusted(4, 4, -4, -4),
                         QIcon(on ? iconOn : iconOff).pixmap(14, 14));
        };

        drawBtn(QRect(hdr.right() - 84, btnY, btnSize, btnSize),
                row.visible, ":/icons/eye.svg", ":/icons/eye-off.svg");
        drawBtn(QRect(hdr.right() - 56, btnY, btnSize, btnSize),
                row.muted, ":/icons/speaker-off.svg", ":/icons/speaker-on.svg");
        drawBtn(QRect(hdr.right() - 28, btnY, btnSize, btnSize),
                row.locked, ":/icons/lock-closed.svg", ":/icons/lock-open.svg");

        // Clips
        for (int ci = 0; ci < row.clips.size(); ++ci) {
            const auto& c = row.clips[ci];
            QRect cr = clipRect(i, c);
            if (cr.width() < 2) continue;

            QColor clipBg = (row.kind == 0) ? CLR_CLIP_VID :
                            (row.kind == 1) ? CLR_CLIP_AUD :
                                                CLR_CLIP_IMG;

            // Clip body
            QLinearGradient grad(cr.topLeft(), cr.bottomLeft());
            grad.setColorAt(0, clipBg.lighter(120));
            grad.setColorAt(1, clipBg.darker(115));
            p.setBrush(grad);
            p.setPen(QPen(QColor(0, 0, 0, 160), 1));
            p.drawRoundedRect(cr, 3, 3);

            // Thumbnail strip for video / image clips
            if ((row.kind == 0 || row.kind == 2) && prober_ && prober_->hasThumbnail(c.path)) {
                QPixmap thumb = prober_->thumbnail(c.path);
                if (!thumb.isNull() && cr.width() > 20) {
                    int thumbW = thumb.width();
                    int thumbH = cr.height() - 4;
                    QRect target(cr.x() + 2, cr.y() + 2, thumbW, thumbH);
                    p.save();
                    p.setClipRect(cr.adjusted(1, 1, -1, -1));
                    // Tile the thumbnail across the clip width
                    int x = cr.x() + 2;
                    while (x < cr.right() - 2) {
                        p.drawPixmap(x, cr.y() + 2, thumbW, thumbH, thumb);
                        x += thumbW;
                    }
                    p.restore();
                    // Overlay a translucent dark band at the bottom for label readability
                    p.fillRect(QRect(cr.x(), cr.bottom() - 18, cr.width(), 14),
                               QColor(0, 0, 0, 140));
                }
            }

            // Waveform overlay for audio clips
            if (row.kind == 1 && prober_ && prober_->hasThumbnail(c.path)) {
                QPixmap wave = prober_->thumbnail(c.path);
                if (!wave.isNull()) {
                    p.save();
                    p.setClipRect(cr.adjusted(1, 1, -1, -1));
                    p.drawPixmap(cr.x() + 2, cr.y() + 2,
                                  cr.width() - 4, cr.height() - 4, wave);
                    p.restore();
                }
            }

            // Selected highlight
            if (selectedRow_ == i && selectedClipIdx_ == ci) {
                p.setBrush(Qt::NoBrush);
                p.setPen(QPen(CLR_SELECTED, 2));
                p.drawRoundedRect(cr.adjusted(1, 1, -1, -1), 3, 3);
            }

            // Trim handles
            if (cr.width() > 24) {
                p.setPen(QPen(QColor(255, 255, 255, 100), 1));
                p.drawLine(cr.x() + 3, cr.y() + 4, cr.x() + 3, cr.bottom() - 4);
                p.drawLine(cr.right() - 3, cr.y() + 4, cr.right() - 3, cr.bottom() - 4);
            }

            // Label
            p.setPen(QColor(255, 255, 255));
            p.setFont(labelFont);
            QString clipLabel = c.name;
            if (fm.horizontalAdvance(clipLabel) > cr.width() - 10) {
                clipLabel = fm.elidedText(clipLabel, Qt::ElideRight, cr.width() - 10);
            }
            p.drawText(cr.adjusted(8, 2, -8, -2),
                       Qt::AlignBottom | Qt::AlignLeft, clipLabel);
        }
    }

    // Playhead
    int phX = xForFrame(playheadFrame_);
    p.setPen(QPen(CLR_PLAYHEAD, 2));
    p.drawLine(phX, 0, phX, height());
    p.setBrush(CLR_PLAYHEAD);
    p.setPen(Qt::NoPen);
    QPolygon tri;
    tri << QPoint(phX - 7, 0) << QPoint(phX + 7, 0) << QPoint(phX, 10);
    p.drawPolygon(tri);

    // Snap indicator (during drag)
    if (dragMode_ == DragMode::MoveClip || dragMode_ == DragMode::TrimClipLeft ||
        dragMode_ == DragMode::TrimClipRight) {
        uint64_t snapped = snapFrame(dragNewStart_);
        if (snapped != dragNewStart_) {
            int sx = xForFrame(snapped);
            p.setPen(QPen(CLR_SNAP, 1, Qt::DashLine));
            p.drawLine(sx, 0, sx, height());
        }
    }
}

void TimelineWidget::handleSelectPress(const QPoint& pos)
{
    int x = pos.x();
    int y = pos.y();

    if (x < headerWidth_) {
        int r = rowAt(y);
        if (r >= 0) {
            const QRect hdr = trackHeaderRect(r);
            int bx = pos.x();
            int right = hdr.right();
            if (bx >= right - 84 && bx <= right - 62) toggleVisible(tracks_[r].id);
            else if (bx >= right - 56 && bx <= right - 34) toggleMute(tracks_[r].id);
            else if (bx >= right - 28 && bx <= right - 6) toggleLock(tracks_[r].id);
        }
        return;
    }

    if (y < rulerHeight_) {
        playheadFrame_ = static_cast<uint64_t>(std::max(0, frameAt(x)));
        dragMode_ = DragMode::ScrubPlayhead;
        update();
        emit playheadMoved(playheadFrame_);
        return;
    }

    int row = -1, clipIdx = -1;
    DragMode m = hitTest(pos, &row, &clipIdx);
    if (m != DragMode::None && row >= 0 && clipIdx >= 0) {
        const auto& c = tracks_[row].clips[clipIdx];
        selectedRow_ = row;
        selectedClipIdx_ = clipIdx;
        emit clipSelected(c.name, c.path, c.startFrame, c.durationFrames,
                          c.trimInFrames, c.adjust, tracks_[row].id, c.id);
        if (tracks_[row].locked) { update(); return; }
        dragMode_ = m;
        dragRow_ = row;
        dragClipIdx_ = clipIdx;
        dragAnchor_ = pos;
        dragOrigStart_     = c.startFrame;
        dragOrigDuration_  = c.durationFrames;
        dragOrigTrimIn_    = c.trimInFrames;
        dragNewStart_      = c.startFrame;
        dragNewDuration_   = c.durationFrames;
        dragNewTrimIn_     = c.trimInFrames;
        dragStartFrame_    = frameAt(x);
        dragCommitted_ = false;
        update();
        return;
    }

    playheadFrame_ = static_cast<uint64_t>(std::max(0, frameAt(x)));
    dragMode_ = DragMode::ScrubPlayhead;
    selectedRow_ = -1;
    selectedClipIdx_ = -1;
    update();
    emit playheadMoved(playheadFrame_);
}

void TimelineWidget::handleRazorPress(const QPoint& pos)
{
    if (pos.x() < headerWidth_) return;
    if (pos.y() < rulerHeight_) {
        playheadFrame_ = static_cast<uint64_t>(std::max(0, frameAt(pos.x())));
        update();
        emit playheadMoved(playheadFrame_);
        return;
    }
    auto [r, i] = clipAt(pos);
    if (r < 0 || i < 0) return;
    if (tracks_[r].locked) return;
    const auto& c = tracks_[r].clips[i];
    uint64_t splitFrame = static_cast<uint64_t>(std::max(0, frameAt(pos.x())));
    if (splitFrame <= c.startFrame || splitFrame >= c.startFrame + c.durationFrames) return;
    if (project_) project_->splitClip(tracks_[r].id, c.id, splitFrame);
    else engine_->splitClip(projectId_, tracks_[r].id, c.id, splitFrame);
    refreshTracks();
}

void TimelineWidget::handleSpacerPress(const QPoint& /*pos*/)
{
    // For v0.4 we don't implement full multi-clip spacer; fall back to
    // select-mode behavior so the user still gets feedback.
    handleSelectPress(mapFromGlobal(QCursor::pos()));
}

void TimelineWidget::handleHandPress(const QPoint& pos)
{
    panAnchor_ = pos;
    dragMode_ = DragMode::Pan;
}

void TimelineWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(e);
        return;
    }
    switch (tool_) {
        case Tool::SelectTool:
        case Tool::RippleTool:
        case Tool::RollTool:
        case Tool::SlipTool:
        case Tool::SlideTool:
        case Tool::MulticamTool:
            handleSelectPress(e->pos()); break;
        case Tool::RazorTool:    handleRazorPress(e->pos());  break;
        case Tool::SpacerTool:   handleSpacerPress(e->pos()); break;
    }
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (dragMode_ == DragMode::None) {
        if (tool_ == Tool::SelectTool || tool_ == Tool::RippleTool) {
            int row = -1, ci = -1;
            DragMode m = hitTest(e->pos(), &row, &ci);
            if (m == DragMode::TrimClipLeft || m == DragMode::TrimClipRight) {
                setCursor(Qt::SizeHorCursor);
            } else if (m == DragMode::MoveClip) {
                setCursor(Qt::SizeAllCursor);
            } else {
                unsetCursor();
            }
        } else if (tool_ == Tool::RazorTool) {
            setCursor(Qt::CrossCursor);
        } else if (tool_ == Tool::SpacerTool) {
            setCursor(Qt::OpenHandCursor);
        } else {
            unsetCursor();
        }
        return;
    }

    if (dragMode_ == DragMode::ScrubPlayhead) {
        int fx = std::max(0, frameAt(e->pos().x()));
        playheadFrame_ = static_cast<uint64_t>(fx);
        update();
        emit playheadMoved(playheadFrame_);
        return;
    }

    if (dragMode_ == DragMode::Pan) {
        // For v0.4 pan is a no-op (we don't have a scrollable canvas yet),
        // but we keep the cursor closed-hand to indicate the gesture.
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (dragRow_ < 0 || dragClipIdx_ < 0) return;
    auto& clip = tracks_[dragRow_].clips[dragClipIdx_];
    int curFrame = frameAt(e->pos().x());
    int deltaFrames = curFrame - dragStartFrame_;

    if (dragMode_ == DragMode::MoveClip) {
        int64_t newStart = static_cast<int64_t>(dragOrigStart_) + deltaFrames;
        if (newStart < 0) newStart = 0;
        // Snap
        uint64_t snapped = snapFrame(static_cast<uint64_t>(newStart));
        dragNewStart_ = snapped;
        clip.startFrame = snapped;
    } else if (dragMode_ == DragMode::TrimClipLeft) {
        int64_t newStart  = static_cast<int64_t>(dragOrigStart_) + deltaFrames;
        int64_t newTrim   = static_cast<int64_t>(dragOrigTrimIn_) + deltaFrames;
        int64_t newDur    = static_cast<int64_t>(dragOrigDuration_) - deltaFrames;
        if (newStart < 0) { newDur += newStart; newTrim -= newStart; newStart = 0; }
        if (newDur < 1) newDur = 1;
        if (newTrim < 0) newTrim = 0;
        // Snap the left edge
        uint64_t snapped = snapFrame(static_cast<uint64_t>(newStart));
        int64_t adjust = static_cast<int64_t>(snapped) - newStart;
        newStart += adjust;
        newTrim  += adjust;
        newDur   -= adjust;
        if (newDur < 1) { newDur = 1; }
        clip.startFrame     = static_cast<uint64_t>(newStart);
        clip.trimInFrames   = static_cast<uint64_t>(newTrim);
        clip.durationFrames = static_cast<uint64_t>(newDur);
        dragNewStart_       = clip.startFrame;
        dragNewTrimIn_      = clip.trimInFrames;
        dragNewDuration_    = clip.durationFrames;
    } else if (dragMode_ == DragMode::TrimClipRight) {
        int64_t newDur = static_cast<int64_t>(dragOrigDuration_) + deltaFrames;
        if (newDur < 1) newDur = 1;
        // Snap the right edge
        uint64_t rightEdge = static_cast<uint64_t>(
            static_cast<int64_t>(dragOrigStart_) + newDur);
        uint64_t snapped = snapFrame(rightEdge);
        newDur = static_cast<int64_t>(snapped) - static_cast<int64_t>(dragOrigStart_);
        if (newDur < 1) newDur = 1;
        clip.durationFrames = static_cast<uint64_t>(newDur);
        dragNewDuration_ = clip.durationFrames;
    }
    update();
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (dragMode_ == DragMode::MoveClip && dragRow_ >= 0 && dragClipIdx_ >= 0) {
        if (dragNewStart_ != dragOrigStart_) {
            TimelineFunctions::moveClip(engine_, projectId_, tracks_[dragRow_].id,
                                         tracks_[dragRow_].clips[dragClipIdx_].id,
                                         dragOrigStart_, dragNewStart_, this);
        } else {
            refreshTracks();
        }
    } else if ((dragMode_ == DragMode::TrimClipLeft ||
                dragMode_ == DragMode::TrimClipRight) &&
               dragRow_ >= 0 && dragClipIdx_ >= 0) {
        TimelineFunctions::trimClip(engine_, projectId_, tracks_[dragRow_].id,
                                     tracks_[dragRow_].clips[dragClipIdx_].id,
                                     dragOrigStart_, dragOrigDuration_, dragOrigTrimIn_,
                                     dragNewStart_, dragNewDuration_, dragNewTrimIn_, this);
    }
    dragMode_ = DragMode::None;
    dragRow_ = -1;
    dragClipIdx_ = -1;
    QWidget::mouseReleaseEvent(e);
}

void TimelineWidget::wheelEvent(QWheelEvent* e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        int delta = e->angleDelta().y() / 120;
        setZoom(pixelsPerFrame_ + delta);
    } else {
        QWidget::wheelEvent(e);
    }
}

void TimelineWidget::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasFormat("application/x-beta-media-path") ||
        e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    } else {
        e->ignore();
    }
}

void TimelineWidget::dropEvent(QDropEvent* e)
{
    QString path;
    if (e->mimeData()->hasFormat("application/x-beta-media-path")) {
        path = QString::fromUtf8(e->mimeData()->data("application/x-beta-media-path"));
    } else if (e->mimeData()->hasUrls()) {
        for (const QUrl& u : e->mimeData()->urls()) {
            if (u.isLocalFile()) { path = u.toLocalFile(); break; }
        }
    }
    if (path.isEmpty()) { e->ignore(); return; }

    int r = rowAt(e->pos().y());
    if (r < 0) {
        if (tracks_.isEmpty()) { e->ignore(); return; }
        r = 0;
    }
    if (r >= tracks_.size()) { e->ignore(); return; }
    if (tracks_[r].locked) { e->ignore(); return; }

    uint64_t startFrame = static_cast<uint64_t>(std::max(0, frameAt(e->pos().x())));
    // Snap the drop position to playhead or clip edges
    startFrame = snapFrame(startFrame);

    QString name = QFileInfo(path).fileName();
    uint64_t duration = 150;
    int width = 0, height = 0;
    uint64_t mediaDurationFrames = 0;
    double fps = 30.0;
    if (prober_) {
        MediaProber::Info info = prober_->info(path);
        if (info.probed) {
            width  = info.width;
            height = info.height;
            fps    = info.fps > 0 ? info.fps : 30.0;
            if (info.durationMs > 0) {
                mediaDurationFrames = static_cast<uint64_t>(info.durationMs * fps / 1000.0);
                duration = mediaDurationFrames;
            } else if (info.kind == "image") {
                duration = 150;
            }
        } else {
            prober_->probeAsync(path);
        }
    }

    if (project_) {
        project_->addClip(tracks_[r].id, path, name, startFrame, duration);
    } else {
        TimelineFunctions::addClip(engine_, projectId_, tracks_[r].id,
                                    path, name, startFrame, duration, this);
    }
    e->acceptProposedAction();
}

} // namespace beta
