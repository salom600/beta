#pragma once

#include <QWidget>
#include <QListWidget>

class QToolBar;
class QLineEdit;

namespace beta {

/// Effects/Transitions panel — like Kdenlive's effect list. Shows a
/// searchable list of available effects that can be dragged onto clips
/// in the timeline.
class EffectsPanel : public QWidget {
    Q_OBJECT
public:
    explicit EffectsPanel(QWidget* parent = nullptr);

    /// Populate the list with built-in effects. In a real editor this
    /// would come from an effects repository; for v0.6 we ship a
    /// static list of common adjustments.
    void loadBuiltinEffects();

signals:
    /// Emitted when the user double-clicks an effect (apply to
    /// selected clip).
    void effectActivated(const QString& effectId, const QString& effectName);

private slots:
    void onItemActivated(QListWidgetItem* item);
    void onFilterChanged(const QString& text);

private:
    void setupUi();

    QToolBar*   toolbar_     = nullptr;
    QLineEdit*  searchEdit_  = nullptr;
    QListWidget* list_       = nullptr;

    struct EffectEntry {
        QString id;
        QString name;
        QString category;
        QString description;
    };
    QList<EffectEntry> allEffects_;
};

} // namespace beta
