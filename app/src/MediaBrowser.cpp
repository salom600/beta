#include "MediaBrowser.h"

#include <QFileInfo>
#include <QFileInfoList>
#include <QHeaderView>
#include <QListWidgetItem>

namespace beta {

MediaBrowser::MediaBrowser(QWidget* parent)
    : QListWidget(parent)
{
    setWindowTitle(tr("Media"));
    setWindowIcon(style()->standardIcon(QStyle::SP_DirIcon));
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setIconSize(QSize(32, 32));
    setToolTip(tr("Imported media assets. Double-click to preview."));
    connect(this, &QListWidget::itemActivated,
            this, &MediaBrowser::onItemActivated);
}

void MediaBrowser::addMedia(const QString& path)
{
    // De-duplicate by file path
    for (int i = 0; i < count(); ++i) {
        if (item(i)->data(Qt::UserRole).toString() == path) return;
    }

    QFileInfo info(path);
    QString suffix = info.suffix().toLower();

    QIcon icon;
    if (suffix == "mp4" || suffix == "mov" || suffix == "mkv" ||
        suffix == "avi" || suffix == "webm" || suffix == "m4v") {
        icon = style()->standardIcon(QStyle::SP_MediaVideo);
    } else if (suffix == "mp3" || suffix == "wav" || suffix == "aac" ||
               suffix == "flac" || suffix == "ogg" || suffix == "m4a") {
        icon = style()->standardIcon(QStyle::SP_MediaVolume);
    } else if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
               suffix == "bmp" || suffix == "gif" || suffix == "webp") {
        icon = style()->standardIcon(QStyle::SP_FileIcon);
    } else {
        icon = style()->standardIcon(QStyle::SP_FileDialogContentsView);
    }

    auto* it = new QListWidgetItem(icon, info.fileName(), this);
    it->setData(Qt::UserRole, path);
    it->setToolTip(path);
    addItem(it);
}

void MediaBrowser::onItemActivated(QListWidgetItem* item)
{
    if (!item) return;
    const QString path = item->data(Qt::UserRole).toString();
    emit mediaActivated(path, item->text());
}

} // namespace beta
