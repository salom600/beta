#include "MediaBrowser.h"
#include "MediaProber.h"

#include <QFileInfo>
#include <QListWidgetItem>
#include <QMimeData>
#include <QStyle>
#include <QPixmap>
#include <QIcon>
#include <QPainter>
#include <QByteArray>
#include <QBuffer>

namespace beta {

namespace {

QIcon iconForKind(const QString& suffix)
{
    // Lazy-load SVG icons from the Qt resource system
    static const QIcon videoIcon(":/icons/video-track.svg");
    static const QIcon audioIcon(":/icons/audio-track.svg");
    static const QIcon imageIcon(":/icons/image-track.svg");
    static const QIcon fileIcon (":/icons/film.svg");

    if (suffix == "mp4" || suffix == "mov" || suffix == "mkv" ||
        suffix == "avi" || suffix == "webm" || suffix == "m4v" ||
        suffix == "wmv" || suffix == "flv") {
        return videoIcon;
    }
    if (suffix == "mp3" || suffix == "wav" || suffix == "aac" ||
        suffix == "flac" || suffix == "ogg" || suffix == "m4a" ||
        suffix == "opus") {
        return audioIcon;
    }
    if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
        suffix == "bmp" || suffix == "gif" || suffix == "webp" ||
        suffix == "tiff" || suffix == "tga") {
        return imageIcon;
    }
    return fileIcon;
}

} // namespace

MediaBrowser::MediaBrowser(QWidget* parent)
    : QListWidget(parent)
{
    setWindowTitle(tr("Media"));
    setAlternatingRowColors(false);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setIconSize(QSize(24, 24));
    setSpacing(2);
    setUniformItemSizes(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setToolTip(tr("Imported media. Double-click to preview, drag onto the timeline to add a clip."));
    setStyleSheet("QListWidget { background: #1e1f22; border: none; }");

    connect(this, &QListWidget::itemActivated,
            this, &MediaBrowser::onItemActivated);
    connect(this, &QListWidget::itemSelectionChanged,
            this, &MediaBrowser::onItemSelectionChanged);
}

void MediaBrowser::setProber(MediaProber* prober)
{
    prober_ = prober;
    if (prober_) {
        connect(prober_, &MediaProber::probed,
                this, [this](const QString& path, const MediaProber::Info& info) {
            for (int i = 0; i < count(); ++i) {
                QListWidgetItem* it = item(i);
                if (it->data(Qt::UserRole).toString() == path) {
                    QString tip = QString("%1\n%2×%3 • %4 s • %5 fps")
                        .arg(path)
                        .arg(info.width).arg(info.height)
                        .arg(info.durationMs / 1000.0, 0, 'f', 2)
                        .arg(info.fps, 0, 'f', 2);
                    it->setToolTip(tip);
                    break;
                }
            }
            emit mediaProbed(path);
        });
    }
}

void MediaBrowser::addMedia(const QString& path)
{
    for (int i = 0; i < count(); ++i) {
        if (item(i)->data(Qt::UserRole).toString() == path) return;
    }

    QFileInfo info(path);
    QIcon icon = iconForKind(info.suffix().toLower());

    auto* it = new QListWidgetItem(icon, info.fileName(), this);
    it->setData(Qt::UserRole, path);
    it->setToolTip(path);
    addItem(it);

    if (prober_) prober_->probeAsync(path);
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
    emit mediaActivated(path, item->text());
}

void MediaBrowser::onItemSelectionChanged()
{
    QString path = selectedPath();
    if (!path.isEmpty()) {
        emit mediaActivated(path, QFileInfo(path).fileName());
    }
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

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mime);
    drag->setPixmap(it->icon().pixmap(32, 32));
    drag->setHotSpot(QPoint(16, 16));
    drag->exec(supportedActions, Qt::CopyAction);
}

} // namespace beta
