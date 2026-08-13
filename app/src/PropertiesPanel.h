#pragma once

#include <QWidget>
#include <cstdint>

class QFormLayout;
class QLabel;
class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;

namespace beta {

class EngineBridge;

/// Right-side panel. Shows properties of the currently selected clip
/// or media asset. Edits are propagated to the engine via the
/// EngineBridge so the timeline picks them up on the next refresh.
class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr);

    void setEngine(EngineBridge* engine) { engine_ = engine; }
    void setProjectId(uint64_t id)       { projectId_ = id; }

public slots:
    void showMediaInfo(const QString& path, const QString& name);
    void showClipInfo(const QString& name, const QString& path,
                      uint64_t start, uint64_t duration,
                      uint64_t trimIn,
                      double volume, double opacity, double scale,
                      uint64_t trackId, uint64_t clipId);
    void showProjectInfo();
    void clearSelection();

private slots:
    void onPropsChanged();

private:
    void setupUi();
    void setClipMode(bool enabled);

    QLabel*         title_       = nullptr;
    QLabel*         pathValue_   = nullptr;
    QLineEdit*      nameEdit_    = nullptr;
    QSpinBox*       startFrame_  = nullptr;
    QSpinBox*       duration_    = nullptr;
    QSpinBox*       trimIn_      = nullptr;
    QDoubleSpinBox* volumeSpin_  = nullptr;
    QDoubleSpinBox* opacitySpin_ = nullptr;
    QDoubleSpinBox* scaleSpin_   = nullptr;

    EngineBridge* engine_   = nullptr;
    uint64_t      projectId_ = 0;
    uint64_t      trackId_  = 0;
    uint64_t      clipId_   = 0;
    bool          inClipMode_ = false;
};

} // namespace beta
