#pragma once

#include <QListWidget>

namespace beta {

/// Left-side panel showing imported media assets. Double-click an item
/// to push it to the preview and properties panels.
class MediaBrowser : public QListWidget {
    Q_OBJECT
public:
    explicit MediaBrowser(QWidget* parent = nullptr);

    /// Adds a media file path to the browser (deduplicated).
    void addMedia(const QString& path);

signals:
    /// Emitted when an item is double-clicked or activated.
    void mediaActivated(const QString& path, const QString& name);

private slots:
    void onItemActivated(QListWidgetItem* item);
};

} // namespace beta
