#include "TimelineWidget.h"
#include "EngineBridge.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QRect>
#include <QStyleOption>
#include <QTimer>
#include <QWheelEvent>

#include <algorithm>

namespace beta {

TimelineWidget::TimelineWidget(QWidget* parent, EngineBridge* engine, uint64_t projectId)
    : QWidget(parent), engine_(engine), projectId_(projectId)
{
    setMinimumHeight(260);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    playheadTimer_ = new QTimer(this);
    playheadTimer_->setInterval(33); // ~30fps
    connect(playheadTimer_, &QTimer::timeout, this, &TimelineWidget::onPlayheadTimer);

    refreshTracks();
}

void TimelineWidget::setProjectId(uint64_t id)
{
    projectId_ = id;
    refreshTracks();
}

void TimelineWidget::refreshTracks()
{
    tracks_.clear();
    const auto ids = engine_->trackIds(projectId_);
    for (uint64_t tid : ids) {
        TrackRow row;
        row.id = tid;
        row.kind = engine_->trackKind(projectId_, tid);
        row.name = engine_->trackName(projectId_, tid);
        auto s = engine_->trackState(projectId_, tid);
        row.visible = s.visible;
        row.locked = s.locked;
        row.muted = s.muted;
        tracks_.append(row);
    }
    update();
}

QSize TimelineWidget::sizeHint() const
{
    return QSize(900, 320);
}

QSize TimelineWidget::minimumSizeHint() const
{
    return QSize(600, 240);
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

void TimelineWidget::addClipToFirstTrack(const QString& path, const QString& name, int kindHint)
{
    if (tracks_.isEmpty()) return;
    // Find first track of matching kind (fallback to first track).
    int target = 0;
    for (int i = 0; i < tracks_.size(); ++i) {
        if (tracks_[i].kind == kindHint) {
            target = i;
            break;
        }
    }
    TrackRow& row = tracks_[target];
    uint64_t start = 0;
    if (!row.clips.isEmpty()) {
        const auto& last = row.clips.last();
        start = last.startFrame + last.durationFrames;
    }
    uint64_t dur = 150; // ~5s at 30fps; UI default until probe
    uint64_t cid = engine_->addClip(projectId_, row.id, path, name, start, dur);
    row.clips.append({ cid, name, start, dur });
    update();
}

void TimelineWidget::onPlayheadTimer()
{
    if (!playing_) return;
    playheadFrame_ += 1;
    update();
}

void TimelineWidget::setupUi() {}

void TimelineWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QColor bg       = palette().color(QPalette::Base);
    const QColor altBg    = palette().color(QPalette::AlternateBase);
    const QColor headerBg = QColor(34, 34, 38);
    const QColor rulerBg  = QColor(28, 28, 32);
    const QColor text     = palette().color(QPalette::Text);
    const QColor muted    = QColor(140, 140, 140);
    const QColor playhead = QColor(255, 90, 90);
    const QColor gridLine = QColor(60, 60, 64);

    // Background
    p.fillRect(rect(), bg);

    // --- Ruler ---
    QRect rulerRect(headerWidth_, 0, width() - headerWidth_, rulerHeight_);
    p.fillRect(rulerRect, rulerBg);
    p.fillRect(0, 0, headerWidth_, rulerHeight_, headerBg);

    p.setPen(QPen(gridLine, 1));
    int frameStep = 30; // every second @ 30fps
    int maxX = width();
    for (int f = 0; xForFrame(f) < maxX; f += frameStep) {
        int x = xForFrame(f);
        p.drawLine(x, 0, x, rulerHeight_);
        p.setPen(text);
        QFont fnt = font();
        fnt.setPointSize(8);
        p.setFont(fnt);
        int sec = f / 30;
        int mm = sec / 60;
        int ss = sec % 60;
        p.drawText(x + 4, rulerHeight_ - 6,
                   QString::asprintf("%02d:%02d", mm, ss));
        p.setPen(gridLine);
    }

    p.setPen(QPen(gridLine, 1));
    p.drawLine(0, rulerHeight_, width(), rulerHeight_);

    if (tracks_.isEmpty()) {
        p.setPen(muted);
        QFont fnt = font();
        fnt.setPointSize(10);
        fnt.setItalic(true);
        p.setFont(fnt);
        p.drawText(rect(), Qt::AlignCenter,
                   tr("No tracks yet — use the toolbar to add Video / Audio / Image tracks."));
        return;
    }

    // --- Track rows ---
    QFont labelFont = font();
    labelFont.setPointSize(9);
    labelFont.setBold(true);

    for (int i = 0; i < tracks_.size(); ++i) {
        const TrackRow& row = tracks_[i];
        QRect hdr = trackHeaderRect(i);
        QRect body = trackBodyRect(i);

        // Header background
        QColor hdrBg = (row.kind == 0) ? QColor(48, 70, 110) :
                       (row.kind == 1) ? QColor(80, 60, 110) :
                                          QColor(110, 80, 50);
        if (!row.visible) hdrBg = hdrBg.darker(180);
        p.fillRect(hdr, hdrBg);

        // Body background
        QColor bodyBg = (i % 2 == 0) ? bg : altBg;
        if (!row.visible) bodyBg = bodyBg.darker(140);
        p.fillRect(body, bodyBg);

        // Vertical grid lines
        p.setPen(QPen(gridLine, 1));
        for (int f = 0; xForFrame(f) < width(); f += frameStep) {
            int x = xForFrame(f);
            p.drawLine(x, body.top(), x, body.bottom());
        }

        // Header border
        p.setPen(QPen(QColor(0, 0, 0, 180), 1));
        p.drawRect(hdr.adjusted(0, 0, -1, -1));
        p.setPen(QPen(gridLine, 1));
        p.drawLine(body.right(), body.top(), body.right(), body.bottom());

        // Track name
        p.setPen(text);
        p.setFont(labelFont);
        QFontMetrics fm(labelFont);
        QString label = row.name;
        if (fm.horizontalAdvance(label) > hdr.width() - 80) {
            label = fm.elidedText(label, Qt::ElideRight, hdr.width() - 80);
        }
        p.drawText(hdr.adjusted(8, 4, 80, -4), Qt::AlignVCenter | Qt::AlignLeft, label);

        // Buttons drawn manually in header right side: eye, mute, lock
        // Each ~24px wide, stacked at x = hdr.right() - 76, 50, 24
        int btnY = hdr.center().y() - 10;
        int btnSize = 20;

        // Eye (visible)
        QRect eyeBtn(hdr.right() - 76, btnY, btnSize, btnSize);
        QColor eyeCol = row.visible ? QColor(120, 200, 120) : QColor(80, 80, 80);
        p.fillRect(eyeBtn, eyeCol);
        p.setPen(QPen(QColor(0, 0, 0, 200), 1));
        p.drawRect(eyeBtn);
        p.setPen(text);
        p.drawText(eyeBtn, Qt::AlignCenter, row.kind == 1 ? QStringLiteral("M") : QStringLiteral("V"));

        // Mute
        QRect muteBtn(hdr.right() - 50, btnY, btnSize, btnSize);
        QColor muteCol = row.muted ? QColor(220, 90, 90) : QColor(80, 80, 80);
        p.fillRect(muteBtn, muteCol);
        p.setPen(QPen(QColor(0, 0, 0, 200), 1));
        p.drawRect(muteBtn);
        p.setPen(text);
        p.drawText(muteBtn, Qt::AlignCenter, QStringLiteral("M"));

        // Lock
        QRect lockBtn(hdr.right() - 24, btnY, btnSize, btnSize);
        QColor lockCol = row.locked ? QColor(230, 200, 80) : QColor(80, 80, 80);
        p.fillRect(lockBtn, lockCol);
        p.setPen(QPen(QColor(0, 0, 0, 200), 1));
        p.drawRect(lockBtn);
        p.setPen(text);
        p.drawText(lockBtn, Qt::AlignCenter, QStringLiteral("L"));

        // Clips
        for (const auto& c : row.clips) {
            int cx = xForFrame(c.startFrame);
            int cw = static_cast<int>(c.durationFrames) * pixelsPerFrame_;
            QRect clipRect(cx + 1, body.top() + 4, cw - 2, body.height() - 8);

            QColor clipBg;
            if (row.kind == 0)      clipBg = QColor(70, 130, 200);
            else if (row.kind == 1) clipBg = QColor(150, 110, 200);
            else                    clipBg = QColor(220, 160, 80);

            QLinearGradient grad(clipRect.topLeft(), clipRect.bottomLeft());
            grad.setColorAt(0, clipBg.lighter(120));
            grad.setColorAt(1, clipBg);
            p.fillRect(clipRect, grad);

            p.setPen(QPen(QColor(20, 20, 20), 1));
            p.drawRect(clipRect.adjusted(0, 0, -1, -1));

            p.setPen(QColor(255, 255, 255));
            p.setFont(labelFont);
            QString clipLabel = c.name;
            if (fm.horizontalAdvance(clipLabel) > clipRect.width() - 8) {
                clipLabel = fm.elidedText(clipLabel, Qt::ElideRight, clipRect.width() - 8);
            }
            p.drawText(clipRect.adjusted(6, 4, -6, -4),
                       Qt::AlignVCenter | Qt::AlignLeft, clipLabel);
        }
    }

    // --- Playhead ---
    int phX = xForFrame(playheadFrame_);
    p.setPen(QPen(playhead, 2));
    p.drawLine(phX, 0, phX, height());
    p.setBrush(playhead);
    QPolygon tri;
    tri << QPoint(phX - 6, 0) << QPoint(phX + 6, 0) << QPoint(phX, 8);
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

    // Header area?
    if (x < headerWidth_) {
        int r = rowAt(y);
        if (r >= 0) {
            const QRect hdr = trackHeaderRect(r);
            int bx = e->pos().x();
            int right = hdr.right();
            if (bx >= right - 76 && bx <= right - 56) toggleVisible(tracks_[r].id);
            else if (bx >= right - 50 && bx <= right - 30) toggleMute(tracks_[r].id);
            else if (bx >= right - 24 && bx <= right - 4)  toggleLock(tracks_[r].id);
        }
        return;
    }

    // Ruler area? -> move playhead
    if (y < rulerHeight_) {
        playheadFrame_ = static_cast<uint64_t>(std::max(0, frameAt(x)));
        draggingPlayhead_ = true;
        update();
        return;
    }

    // Body area: select a clip if hit, else move playhead
    int r = rowAt(y);
    if (r < 0) return;
    const TrackRow& row = tracks_[r];
    int fx = frameAt(x);
    for (const auto& c : row.clips) {
        if (fx >= static_cast<int>(c.startFrame) &&
            fx <  static_cast<int>(c.startFrame + c.durationFrames)) {
            emit clipSelected(c.name, QStringLiteral(""), c.startFrame, c.durationFrames);
            update();
            return;
        }
    }
    // Otherwise move playhead
    playheadFrame_ = static_cast<uint64_t>(std::max(0, fx));
    draggingPlayhead_ = true;
    update();
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (draggingPlayhead_) {
        int fx = std::max(0, frameAt(e->pos().x()));
        playheadFrame_ = static_cast<uint64_t>(fx);
        update();
    } else {
        QWidget::mouseMoveEvent(e);
    }
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent* e)
{
    draggingPlayhead_ = false;
    QWidget::mouseReleaseEvent(e);
}

void TimelineWidget::wheelEvent(QWheelEvent* e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        // Zoom
        int delta = e->angleDelta().y() / 120;
        pixelsPerFrame_ = std::clamp(pixelsPerFrame_ + delta, 1, 24);
        update();
    } else {
        QWidget::wheelEvent(e);
    }
}

} // namespace beta
