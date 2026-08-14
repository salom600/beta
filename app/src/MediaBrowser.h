#pragma once

#include <QListWidget>
#include "MediaProber.h"

namespace beta {

/// Left-side panel showing imported media assets. Each item is
/// rendered by a custom delegate that paints:
///   • a thumbnail (video frame / image / audio waveform)
///   • the file name
///   • the duration (for video/audio) or dimensions (for image)
///
/// Supports:
///   • Double-click → load into preview
///   • Drag → drop onto a timeline track
class MediaBrowser : public QListWidget {
    Q_OBJECT
public:
    explicit MediaBrowser(QWidget* parent = nullptr);

    void setProber(MediaProber* prober);

    void addMedia(const QString& path);
    QString pathForItem(QListWidgetItem* item) const;
    QString selectedPath() const;

signals:
    void mediaActivated(const QString& path, const QString& name);
    void mediaProbed(const QString& path);

protected:
    void startDrag(Qt::DropActions supportedActions) override;

private slots:
    void onItemActivated(QListWidgetItem* item);
    void onThumbnailReady(const QString& path, const QPixmap& pix);
    void onProbed(const QString& path, const MediaProber::Info& info);

private:
    MediaProber* prober_ = nullptr;
};

} // namespace beta
