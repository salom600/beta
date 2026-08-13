#include "TimelineWidget.h"
#include "EngineBridge.h"
#include "MediaProber.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QRect>
#include <QStyleOption>
#include <QTimer>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QWheelEvent>
#include <QScrollBar>
#include <QScrollArea>
#include <cmath>

namespace beta {

namespace {
const QColor CLR_BG         ( 30,  31,  34);   // #1e1f22
const QColor CLR_ALT_BG     ( 37,  38,  42);   // #25262a
const QColor CLR_HEADER_BG  ( 45,  46,  51);   // #2d2e33
const QColor CLR_HEADER_VID ( 35,  60, 110);
const QColor CLR_HEADER_AUD ( 70,  50, 110);
const QColor CLR_HEADER_IMG (110,  75,  40);
const QColor CLR_RULER      ( 22,  23,  26);
const QColor CLR_GRID       ( 60,  61,  68);
const QColor CLR_TEXT       (220, 221, 226);
const QColor CLR_TEXT_DIM   (140, 141, 148);
const QColor CLR_PLAYHEAD   (255,  90,  90);
const QColor CLR_CLIP_VID   ( 74, 158, 255);
const QColor CLR_CLIP_AUD   (181, 102, 224);
const QColor CLR_CLIP_IMG   (224, 160,  64);
const QColor CLR_SELECTED   (255, 200,  90);
const QColor CLR_BTN_ON     ( 14,  99, 212);
const QColor CLR_BTN_OFF    ( 60,  61,  68);
const QColor CLR_BTN_HOVER  ( 90,  91,  98);
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

    playheadTimer_ = new QTimer(this);
    playheadTimer_->setInterval(33);
    connect(playheadTimer_, &QTimer::timeout, this, &TimelineWidget::onPlayheadTimer);

    refreshTracks();
}

void TimelineWidget::setProber(MediaProber* prober) { prober_ = prober; }

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
            clip.volume            = c.volume;
            clip.opacity           = c.opacity;
            clip.scale             = c.scale;
            clip.mediaWidth        = c.mediaWidth;
            clip.mediaHeight       = c.mediaHeight;
            clip.mediaDurationFrames = c.mediaDurationFrames;
            row.clips.append(clip);
        }
        tracks_.append(row);
    }
    update();
}

QSize TimelineWidget::sizeHint() const
{
    return QSize(1000, 380);
}

QSize TimelineWidget::minimumSizeHint() const
{
    return QSize(700, 280);
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
    return QRect(x, y + 6, w, rowHeight_ - 12);
}

void TimelineWidget::toggleVisible(uint64_t trackId)
{
    for (auto& t : tracks_) {
        if (t.id == trackId) {
            t.visible = !t.visible;
            EngineBridge::TrackState s;
            s.visible = t.visible;
            s.locked = t.locked;
            s.muted = t.muted;
            engine_->setTrackState(projectId_, trackId, s);
            update();
            return;
        }
    }
}

void TimelineWidget::toggleMute(uint64_t trackId)
{
    for (auto& t : tracks_) {
        if (t.id == trackId) {
            t.muted = !t.muted;
            EngineBridge::TrackState s;
            s.visible = t.visible;
            s.locked = t.locked;
            s.muted = t.muted;
            engine_->setTrackState(projectId_, trackId, s);
            update();
            return;
        }
    }
}

void TimelineWidget::toggleLock(uint64_t trackId)
{
    for (auto& t : tracks_) {
        if (t.id == trackId) {
            t.locked = !t.locked;
            EngineBridge::TrackState s;
            s.visible = t.visible;
            s.locked = t.locked;
            s.muted = t.muted;
            engine_->setTrackState(projectId_, trackId, s);
            update();
            return;
        }
    }
}

void TimelineWidget::commitClipMove(int row, int clipIdx)
{
    if (row < 0 || row >= tracks_.size()) return;
    auto& clip = tracks_[row].clips[clipIdx];
    clip.startFrame = dragNewStart_;
    engine_->moveClip(projectId_, tracks_[row].id, clip.id, dragNewStart_);
    update();
}

void TimelineWidget::commitClipTrim(int row, int clipIdx)
{
    if (row < 0 || row >= tracks_.size()) return;
    auto& clip = tracks_[row].clips[clipIdx];
    clip.startFrame     = dragNewStart_;
    clip.durationFrames = dragNewDuration_;
    clip.trimInFrames   = dragNewTrimIn_;
    engine_->trimClip(projectId_, tracks_[row].id, clip.id,
                       dragNewTrimIn_, dragNewDuration_);
    engine_->moveClip(projectId_, tracks_[row].id, clip.id, dragNewStart_);
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
    int fx = frameAt(pos.x());
    for (int i = 0; i < row.clips.size(); ++i) {
        const auto& c = row.clips[i];
        QRect cr = clipRect(r, c);
        if (cr.contains(pos)) {
            *outClipIdx = i;
            // Trim handles
            if (pos.x() - cr.x() <= trimHandleWidth_) {
                return DragMode::TrimClipLeft;
            }
            if (cr.x() + cr.width() - pos.x() <= trimHandleWidth_) {
                return DragMode::TrimClipRight;
            }
            return DragMode::MoveClip;
        }
        (void)fx;
    }
    return DragMode::None;
}

void TimelineWidget::onPlayheadTimer()
{
    ++playheadFrame_;
    update();
    emit playheadMoved(playheadFrame_);
}

void TimelineWidget::setupUi() {}

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

    // --- Ruler ---
    QRect rulerRect(headerWidth_, 0, width() - headerWidth_, rulerHeight_);
    p.fillRect(rulerRect, CLR_RULER);
    p.fillRect(0, 0, headerWidth_, rulerHeight_, CLR_HEADER_BG);

    int frameStep = 30; // every second @ 30fps
    p.setPen(QPen(CLR_GRID, 1));
    for (int f = 0; xForFrame(f) < width(); f += frameStep) {
        int x = xForFrame(f);
        p.drawLine(x, rulerHeight_ - 8, x, rulerHeight_);
        if (f % (frameStep * 2) == 0) {
            p.setPen(CLR_TEXT_DIM);
            p.setFont(rulerFont);
            int sec = f / 30;
            int mm = sec / 60;
            int ss = sec % 60;
            p.drawText(x + 4, rulerHeight_ - 10,
                       QString::asprintf("%02d:%02d", mm, ss));
            p.setPen(CLR_GRID);
        }
    }

    p.setPen(QPen(CLR_GRID, 1));
    p.drawLine(0, rulerHeight_, width(), rulerHeight_);
    p.drawLine(headerWidth_, 0, headerWidth_, height());

    if (tracks_.isEmpty()) {
        p.setPen(CLR_TEXT_DIM);
        QFont fnt = font();
        fnt.setPointSize(11);
        fnt.setItalic(true);
        p.setFont(fnt);
        p.drawText(rect().adjusted(0, rulerHeight_, 0, 0), Qt::AlignCenter,
                   tr("No tracks yet.\nUse the toolbar to add a Video / Audio / Image track,\n"
                      "then drag media from the left panel onto the timeline."));
        return;
    }

    // --- Track rows ---
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

        // Vertical grid lines
        p.setPen(QPen(CLR_GRID, 1));
        for (int f = 0; xForFrame(f) < width(); f += frameStep) {
            int x = xForFrame(f);
            p.drawLine(x, body.top(), x, body.bottom());
        }

        // Header separators
        p.setPen(QPen(QColor(0, 0, 0, 160), 1));
        p.drawRect(hdr.adjusted(0, 0, -1, -1));
        p.setPen(QPen(CLR_GRID, 1));
        p.drawLine(body.right(), body.top(), body.right(), body.bottom());
        p.drawLine(0, hdr.bottom(), width(), hdr.bottom());

        // Track name + kind icon
        p.setPen(QColor(255, 255, 255, 230));
        p.setFont(labelFont);
        QString label = row.name;
        if (fm.horizontalAdvance(label) > hdr.width() - 100) {
            label = fm.elidedText(label, Qt::ElideRight, hdr.width() - 100);
        }
        p.drawText(hdr.adjusted(10, 6, 100, -6),
                   Qt::AlignVCenter | Qt::AlignLeft, label);

        // Buttons: visible/mute, lock
        int btnSize = 24;
        int btnY = hdr.center().y() - btnSize / 2;

        // Eye / mute (top right corner of header)
        QRect eyeBtn(hdr.right() - 84, btnY, btnSize, btnSize);
        QColor eyeCol = row.visible ? CLR_BTN_ON : CLR_BTN_OFF;
        p.setBrush(eyeCol);
        p.setPen(QPen(QColor(0, 0, 0, 200), 1));
        p.drawRoundedRect(eyeBtn, 4, 4);
        p.drawPixmap(eyeBtn.adjusted(5, 5, -5, -5),
                     QIcon(row.visible ? ":/icons/eye.svg" : ":/icons/eye-off.svg")
                         .pixmap(14, 14));

        QRect muteBtn(hdr.right() - 54, btnY, btnSize, btnSize);
        QColor muteCol = row.muted ? QColor(220, 90, 90) : CLR_BTN_OFF;
        p.setBrush(muteCol);
        p.setPen(QPen(QColor(0, 0, 0, 200), 1));
        p.drawRoundedRect(muteBtn, 4, 4);
        p.drawPixmap(muteBtn.adjusted(5, 5, -5, -5),
                     QIcon(row.muted ? ":/icons/speaker-off.svg" : ":/icons/speaker-on.svg")
                         .pixmap(14, 14));

        QRect lockBtn(hdr.right() - 24, btnY, btnSize, btnSize);
        QColor lockCol = row.locked ? QColor(230, 200, 80) : CLR_BTN_OFF;
        p.setBrush(lockCol);
        p.setPen(QPen(QColor(0, 0, 0, 200), 1));
        p.drawRoundedRect(lockBtn, 4, 4);
        p.drawPixmap(lockBtn.adjusted(5, 5, -5, -5),
                     QIcon(row.locked ? ":/icons/lock-closed.svg" : ":/icons/lock-open.svg")
                         .pixmap(14, 14));

        // Clips
        for (int ci = 0; ci < row.clips.size(); ++ci) {
            const auto& c = row.clips[ci];
            QRect cr = clipRect(i, c);
            if (cr.width() < 2) continue;

            QColor clipBg = (row.kind == 0) ? CLR_CLIP_VID :
                            (row.kind == 1) ? CLR_CLIP_AUD :
                                                CLR_CLIP_IMG;

            QLinearGradient grad(cr.topLeft(), cr.bottomLeft());
            grad.setColorAt(0, clipBg.lighter(125));
            grad.setColorAt(1, clipBg.darker(115));
            p.setBrush(grad);
            p.setPen(QPen(QColor(0, 0, 0, 160), 1));
            p.drawRoundedRect(cr, 4, 4);

            // Top stripe
            p.setPen(Qt::NoPen);
            p.setBrush(clipBg.lighter(150));
            p.drawRoundedRect(QRect(cr.x(), cr.y(), cr.width(), 3), 4, 4);

            // Selected highlight
            if (dragMode_ == DragMode::MoveClip && dragRow_ == i && dragClipIdx_ == ci) {
                p.setBrush(Qt::NoBrush);
                p.setPen(QPen(CLR_SELECTED, 2));
                p.drawRoundedRect(cr.adjusted(1, 1, -1, -1), 4, 4);
            }

            // Trim handles (visual hint)
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
            p.drawText(cr.adjusted(8, 4, -8, -4),
                       Qt::AlignVCenter | Qt::AlignLeft, clipLabel);

            // Duration badge (if clip is wide enough)
            if (cr.width() > 80) {
                QString dur = QString::asprintf("%.1fs", c.durationFrames / 30.0);
                p.setPen(QColor(255, 255, 255, 160));
                QFont sm = labelFont;
                sm.setPointSize(8);
                p.setFont(sm);
                p.drawText(cr.adjusted(8, 0, -8, -4),
                           Qt::AlignBottom | Qt::AlignRight, dur);
            }
        }
    }

    // --- Playhead ---
    int phX = xForFrame(playheadFrame_);
    p.setPen(QPen(CLR_PLAYHEAD, 2));
    p.drawLine(phX, 0, phX, height());
    p.setBrush(CLR_PLAYHEAD);
    p.setPen(Qt::NoPen);
    QPolygon tri;
    tri << QPoint(phX - 7, 0) << QPoint(phX + 7, 0) << QPoint(phX, 10);
    p.drawPolygon(tri);
}

void TimelineWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(e);
        return;
    }
    int x = e->pos().x();
    int y = e->pos().y();

    // Header area: handle buttons
    if (x < headerWidth_) {
        int r = rowAt(y);
        if (r >= 0) {
            const QRect hdr = trackHeaderRect(r);
            int bx = e->pos().x();
            int right = hdr.right();
            if (bx >= right - 84 && bx <= right - 60) toggleVisible(tracks_[r].id);
            else if (bx >= right - 54 && bx <= right - 30) toggleMute(tracks_[r].id);
            else if (bx >= right - 24 && bx <= right - 0)   toggleLock(tracks_[r].id);
        }
        return;
    }

    // Ruler area: scrub playhead
    if (y < rulerHeight_) {
        playheadFrame_ = static_cast<uint64_t>(std::max(0, frameAt(x)));
        dragMode_ = DragMode::ScrubPlayhead;
        update();
        emit playheadMoved(playheadFrame_);
        return;
    }

    // Body: hit-test clips
    int row = -1, clipIdx = -1;
    DragMode m = hitTest(e->pos(), &row, &clipIdx);
    if (m != DragMode::None && row >= 0 && clipIdx >= 0) {
        const auto& c = tracks_[row].clips[clipIdx];
        // Locked tracks can't be edited
        if (tracks_[row].locked) {
            emit clipSelected(c.name, c.path, c.startFrame, c.durationFrames,
                              c.trimInFrames, c.volume, c.opacity, c.scale,
                              tracks_[row].id, c.id);
            return;
        }
        dragMode_ = m;
        dragRow_ = row;
        dragClipIdx_ = clipIdx;
        dragAnchor_ = e->pos();
        dragOrigStart_     = c.startFrame;
        dragOrigDuration_  = c.durationFrames;
        dragOrigTrimIn_    = c.trimInFrames;
        dragNewStart_      = c.startFrame;
        dragNewDuration_   = c.durationFrames;
        dragNewTrimIn_     = c.trimInFrames;
        dragStartFrame_    = frameAt(x);
        emit clipSelected(c.name, c.path, c.startFrame, c.durationFrames,
                          c.trimInFrames, c.volume, c.opacity, c.scale,
                          tracks_[row].id, c.id);
        update();
        return;
    }

    // Empty area: scrub playhead
    playheadFrame_ = static_cast<uint64_t>(std::max(0, frameAt(x)));
    dragMode_ = DragMode::ScrubPlayhead;
    update();
    emit playheadMoved(playheadFrame_);
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (dragMode_ == DragMode::None) {
        // Cursor feedback
        int row = -1, ci = -1;
        DragMode m = hitTest(e->pos(), &row, &ci);
        if (m == DragMode::TrimClipLeft || m == DragMode::TrimClipRight) {
            setCursor(Qt::SizeHorCursor);
        } else if (m == DragMode::MoveClip) {
            setCursor(Qt::SizeAllCursor);
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

    if (dragRow_ < 0 || dragClipIdx_ < 0) return;
    auto& clip = tracks_[dragRow_].clips[dragClipIdx_];
    int curFrame = frameAt(e->pos().x());
    int deltaFrames = curFrame - dragStartFrame_;

    if (dragMode_ == DragMode::MoveClip) {
        int64_t newStart = static_cast<int64_t>(dragOrigStart_) + deltaFrames;
        if (newStart < 0) newStart = 0;
        clip.startFrame = static_cast<uint64_t>(newStart);
        dragNewStart_ = clip.startFrame;
    } else if (dragMode_ == DragMode::TrimClipLeft) {
        int64_t newStart  = static_cast<int64_t>(dragOrigStart_) + deltaFrames;
        int64_t newTrim   = static_cast<int64_t>(dragOrigTrimIn_) + deltaFrames;
        int64_t newDur    = static_cast<int64_t>(dragOrigDuration_) - deltaFrames;
        if (newStart < 0) { newDur += newStart; newTrim -= newStart; newStart = 0; }
        if (newDur < 1) newDur = 1;
        if (newTrim < 0) newTrim = 0;
        clip.startFrame     = static_cast<uint64_t>(newStart);
        clip.trimInFrames   = static_cast<uint64_t>(newTrim);
        clip.durationFrames = static_cast<uint64_t>(newDur);
        dragNewStart_       = clip.startFrame;
        dragNewTrimIn_      = clip.trimInFrames;
        dragNewDuration_    = clip.durationFrames;
    } else if (dragMode_ == DragMode::TrimClipRight) {
        int64_t newDur = static_cast<int64_t>(dragOrigDuration_) + deltaFrames;
        if (newDur < 1) newDur = 1;
        clip.durationFrames = static_cast<uint64_t>(newDur);
        dragNewDuration_ = clip.durationFrames;
    }
    update();
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (dragMode_ == DragMode::MoveClip && dragRow_ >= 0 && dragClipIdx_ >= 0) {
        commitClipMove(dragRow_, dragClipIdx_);
    } else if ((dragMode_ == DragMode::TrimClipLeft ||
                dragMode_ == DragMode::TrimClipRight) &&
               dragRow_ >= 0 && dragClipIdx_ >= 0) {
        commitClipTrim(dragRow_, dragClipIdx_);
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
        pixelsPerFrame_ = std::clamp(pixelsPerFrame_ + delta, 1, 32);
        update();
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
        // Drop above tracks: if there are tracks, drop into first matching
        // track by kind. Otherwise ignore.
        if (tracks_.isEmpty()) { e->ignore(); return; }
        // Pick first track
        r = 0;
    }
    if (r >= tracks_.size()) { e->ignore(); return; }
    if (tracks_[r].locked) { e->ignore(); return; }

    uint64_t startFrame = static_cast<uint64_t>(std::max(0, frameAt(e->pos().x())));
    QString name = QFileInfo(path).fileName();

    // Determine duration & dimensions from prober if available
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
                duration = 150; // 5s default for images @ 30fps
            }
        } else {
            // Not probed yet — kick off probe and add with default duration
            prober_->probeAsync(path);
        }
    }

    uint64_t clipId = engine_->addClip(projectId_, tracks_[r].id,
                                        path, name, startFrame, duration);
    if (clipId == 0) { e->ignore(); return; }

    if (width > 0 || mediaDurationFrames > 0) {
        engine_->setClipMediaInfo(projectId_, tracks_[r].id, clipId,
                                    width, height, mediaDurationFrames);
    }

    refreshTracks();
    e->acceptProposedAction();
}

void TimelineWidget::ensureScrollbarIfNeeded()
{
    // (Future) if total width exceeds viewport, parent should provide a
    // QScrollArea. For now we rely on the widget growing with the layout.
}

} // namespace beta
