#pragma once

#include <QListWidget>

namespace beta {

class MediaProber;

/// Left-side panel showing imported media assets. Supports:
///   • Double-click → load into preview
///   • Drag → drop onto a timeline track
class MediaBrowser : public QListWidget {
    Q_OBJECT
public:
    explicit MediaBrowser(QWidget* parent = nullptr);

    void setProber(MediaProber* prober);

    /// Adds a media file path to the browser (deduplicated).
    void addMedia(const QString& path);

    /// Returns the file path for the given item (or empty string).
    QString pathForItem(QListWidgetItem* item) const;

    /// Returns the file path of the currently selected item (or empty).
    QString selectedPath() const;

signals:
    /// Emitted when an item is double-clicked or activated.
    void mediaActivated(const QString& path, const QString& name);

    /// Emitted when media metadata has been probed.
    void mediaProbed(const QString& path);

protected:
    void startDrag(Qt::DropActions supportedActions) override;

private slots:
    void onItemActivated(QListWidgetItem* item);
    void onItemSelectionChanged();

private:
    MediaProber* prober_ = nullptr;
};

} // namespace beta
