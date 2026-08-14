#include "MediaBrowser.h"
#include "MediaProber.h"

#include <QDrag>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>
#include <QLabel>

namespace beta {

namespace {

// Custom item delegate that paints thumbnail + name + duration/dimensions.
class MediaItemDelegate : public QStyledItemDelegate {
public:
    explicit MediaItemDelegate(MediaProber* prober, QObject* parent = nullptr)
        : QStyledItemDelegate(parent), prober_(prober) {}

    void paint(QPainter* p, const QStyleOptionViewItem& opt,
               const QModelIndex& index) const override
    {
        p->save();
        p->setRenderHint(QPainter::Antialiasing, true);

        // Background
        if (opt.state & QStyle::State_Selected) {
            p->fillRect(opt.rect, QColor(14, 99, 212));
        } else if (opt.state & QStyle::State_MouseOver) {
            p->fillRect(opt.rect, QColor(42, 43, 50));
        } else {
            p->fillRect(opt.rect, QColor(30, 31, 34));
        }

        QString path = index.data(Qt::UserRole).toString();
        QString name = index.data(Qt::DisplayRole).toString();
        QString meta = index.data(Qt::UserRole + 1).toString();
        QString kind = index.data(Qt::UserRole + 2).toString();

        // Thumbnail rect (left side)
        QRect thumbRect(opt.rect.x() + 8, opt.rect.y() + 6, 96, 54);
        if (prober_ && prober_->hasThumbnail(path)) {
            QPixmap pix = prober_->thumbnail(path);
            if (!pix.isNull()) {
                QRect drawRect = pix.rect();
                drawRect.setSize(pix.size().scaled(thumbRect.size(), Qt::KeepAspectRatioByExpanding));
                drawRect.moveTo(thumbRect.x() + (thumbRect.width()  - drawRect.width())  / 2,
                                thumbRect.y() + (thumbRect.height() - drawRect.height()) / 2);
                p->setClipRect(thumbRect.adjusted(0, 0, 0, 0));
                p->drawPixmap(drawRect, pix);
                p->setClipping(false);
            }
        } else {
            // Placeholder
            p->fillRect(thumbRect, QColor(20, 20, 24));
            p->setPen(QColor(80, 80, 88));
            QFont f = p->font();
            f.setPointSize(8);
            p->setFont(f);
            p->drawText(thumbRect, Qt::AlignCenter, "...");
        }
        // Thumbnail border
        p->setPen(QPen(QColor(0, 0, 0, 160), 1));
        p->drawRoundedRect(thumbRect, 3, 3);

        // Duration badge overlay (bottom-right of thumbnail)
        if (!meta.isEmpty() && kind != "image") {
            QRect badgeRect(thumbRect.right() - 42, thumbRect.bottom() - 16, 38, 12);
            p->setPen(Qt::NoPen);
            p->setBrush(QColor(0, 0, 0, 200));
            p->drawRoundedRect(badgeRect, 2, 2);
            p->setPen(QColor(255, 255, 255));
            QFont f = p->font();
            f.setPointSize(8);
            p->setFont(f);
            p->drawText(badgeRect, Qt::AlignCenter, meta);
        }

        // Name (right of thumbnail)
        QRect textRect(thumbRect.right() + 12, opt.rect.y() + 8,
                       opt.rect.right() - thumbRect.right() - 20,
                       opt.rect.height() - 16);
        p->setPen(opt.state & QStyle::State_Selected
                  ? QColor(255, 255, 255) : QColor(220, 221, 226));
        QFont nameFont = p->font();
        nameFont.setPointSize(10);
        nameFont.setBold(true);
        p->setFont(nameFont);
        QFontMetrics fm(nameFont);
        QString elided = fm.elidedText(name, Qt::ElideRight, textRect.width());
        p->drawText(textRect.adjusted(0, 0, 0, -14),
                    Qt::AlignLeft | Qt::AlignVCenter, elided);

        // Sub-label (kind + dims)
        p->setPen(opt.state & QStyle::State_Selected
                  ? QColor(220, 220, 220) : QColor(140, 141, 148));
        QFont subFont = p->font();
        subFont.setPointSize(8);
        subFont.setBold(false);
        p->setFont(subFont);
        QString subText;
        if (kind == "image")      subText = "Image";
        else if (kind == "audio") subText = "Audio";
        else if (kind == "video") subText = "Video";
        else                       subText = "Unknown";
        if (!meta.isEmpty() && kind == "image") {
            subText = QString("Image • %1").arg(meta);
        }
        p->drawText(textRect.adjusted(0, 14, 0, 0),
                    Qt::AlignLeft | Qt::AlignVCenter, subText);

        p->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& opt,
                   const QModelIndex& index) const override
    {
        Q_UNUSED(opt); Q_UNUSED(index);
        return QSize(220, 70);
    }

private:
    MediaProber* prober_;
};

} // namespace

MediaBrowser::MediaBrowser(QWidget* parent)
    : QListWidget(parent)
{
    setWindowTitle(tr("Media"));
    setAlternatingRowColors(false);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setIconSize(QSize(96, 54));
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setMouseTracking(true);
    setUniformItemSizes(true);
    setSpacing(2);
    setToolTip(tr("Imported media. Double-click to preview, drag onto the timeline."));
    setStyleSheet("QListWidget { background: #1e1f22; border: none; }");

    connect(this, &QListWidget::itemActivated,
            this, &MediaBrowser::onItemActivated);
    connect(this, &QListWidget::itemSelectionChanged, this, [this]() {
        QString path = selectedPath();
        if (!path.isEmpty()) {
            emit mediaActivated(path, QFileInfo(path).fileName());
        }
    });
}

void MediaBrowser::setProber(MediaProber* prober)
{
    prober_ = prober;
    if (prober_) {
        setItemDelegate(new MediaItemDelegate(prober_, this));
        connect(prober_, &MediaProber::thumbnailReady,
                this, &MediaBrowser::onThumbnailReady);
        connect(prober_, &MediaProber::probed,
                this, &MediaBrowser::onProbed);
    }
}

void MediaBrowser::addMedia(const QString& path)
{
    for (int i = 0; i < count(); ++i) {
        if (item(i)->data(Qt::UserRole).toString() == path) return;
    }

    QFileInfo info(path);
    auto* it = new QListWidgetItem(this);
    it->setData(Qt::UserRole, path);
    it->setData(Qt::DisplayRole, info.fileName());
    QString kind;
    if (MediaProber::isVideoFile(path))      kind = "video";
    else if (MediaProber::isAudioFile(path)) kind = "audio";
    else if (MediaProber::isImageFile(path)) kind = "image";
    else                                      kind = "unknown";
    it->setData(Qt::UserRole + 2, kind);
    it->setData(Qt::UserRole + 1, QString());
    it->setToolTip(path);
    addItem(it);

    if (prober_) {
        prober_->probeAsync(path);
        prober_->requestThumbnail(path, QSize(96, 54));
    }
    // Trigger a repaint by toggling item data
    QModelIndex idx = indexFromItem(it);
    emit dataChanged(idx, idx, {Qt::DisplayRole});
}

QString MediaBrowser::pathForItem(QListWidgetItem* item) const
{
    return item ? item->data(Qt::UserRole).toString() : QString();
}

QString MediaBrowser::selectedPath() const
{
    QList<QListWidgetItem*> items = selectedItems();
    return items.isEmpty() ? QString() : items.first()->data(Qt::UserRole).toString();
}

void MediaBrowser::onItemActivated(QListWidgetItem* item)
{
    if (!item) return;
    const QString path = item->data(Qt::UserRole).toString();
    emit mediaActivated(path, item->data(Qt::DisplayRole).toString());
}

void MediaBrowser::onThumbnailReady(const QString& path, const QPixmap& pix)
{
    Q_UNUSED(pix);
    for (int i = 0; i < count(); ++i) {
        if (item(i)->data(Qt::UserRole).toString() == path) {
            QModelIndex idx = indexFromItem(item(i));
            emit dataChanged(idx, idx, {Qt::DisplayRole});
            break;
        }
    }
}

void MediaBrowser::onProbed(const QString& path, const MediaProber::Info& info)
{
    QString meta;
    if (info.kind == "image") {
        meta = QString("%1×%2").arg(info.width).arg(info.height);
    } else {
        if (info.durationMs > 0) {
            double sec = info.durationMs / 1000.0;
            int mm = int(sec) / 60;
            int ss = int(sec) % 60;
            meta = QString::asprintf("%02d:%02d", mm, ss);
        }
    }
    for (int i = 0; i < count(); ++i) {
        if (item(i)->data(Qt::UserRole).toString() == path) {
            item(i)->setData(Qt::UserRole + 1, meta);
            item(i)->setData(Qt::UserRole + 2, info.kind);
            QModelIndex idx = indexFromItem(item(i));
            emit dataChanged(idx, idx, {Qt::DisplayRole});
            break;
        }
    }
    emit mediaProbed(path);
}

void MediaBrowser::startDrag(Qt::DropActions supportedActions)
{
    QListWidgetItem* it = currentItem();
    if (!it) return;
    QString path = it->data(Qt::UserRole).toString();
    if (path.isEmpty()) return;

    QMimeData* mime = new QMimeData;
    mime->setData("application/x-beta-media-path", path.toUtf8());
    mime->setData("text/uri-list", QUrl::fromLocalFile(path).toEncoded());
    mime->setText(path);

    // Use the thumbnail as drag pixmap if available
    QPixmap dragPix;
    if (prober_ && prober_->hasThumbnail(path)) {
        dragPix = prober_->thumbnail(path).scaled(80, 45, Qt::KeepAspectRatioByExpanding,
                                                   Qt::SmoothTransformation);
    }
    QDrag* drag = new QDrag(this);
    drag->setMimeData(mime);
    if (!dragPix.isNull()) drag->setPixmap(dragPix);
    drag->setHotSpot(QPoint(20, 12));
    drag->exec(supportedActions, Qt::CopyAction);
}

} // namespace beta
