#pragma once

#include <QWidget>
#include <cstdint>

class QFormLayout;
class QLabel;
class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QSlider;
class QPushButton;
class QGroupBox;
class QCheckBox;

namespace beta {

class EngineBridge;
class TimelineWidget;

/// Right-side panel. Shows properties of the currently selected clip
/// or media asset. Edits are propagated to the engine via the
/// EngineBridge so the timeline picks them up.
class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr);

    void setEngine(EngineBridge* engine) { engine_ = engine; }
    void setProjectId(uint64_t id)       { projectId_ = id; }
    void setTimeline(TimelineWidget* t)  { timeline_ = t; }

public slots:
    void showMediaInfo(const QString& path, const QString& name);
    void showClipInfo(const QString& name, const QString& path,
                      uint64_t start, uint64_t duration,
                      uint64_t trimIn,
                      const EngineBridge::ClipAdjust& adjust,
                      uint64_t trackId, uint64_t clipId);
    void showProjectInfo();
    void clearSelection();

private slots:
    void onAdjustChanged();
    void onResetColor();
    void onResetTransform();

private:
    void setupUi();
    void setClipMode(bool enabled);
    void pushAdjustToEngine();

    QLabel*         title_       = nullptr;
    QLabel*         pathValue_   = nullptr;
    QLineEdit*      nameEdit_    = nullptr;
    QSpinBox*       startFrame_  = nullptr;
    QSpinBox*       duration_    = nullptr;
    QSpinBox*       trimIn_      = nullptr;

    // Color group
    QSlider*        brightness_  = nullptr;
    QSlider*        contrast_    = nullptr;
    QSlider*        saturation_  = nullptr;
    QSlider*        hue_         = nullptr;
    QLabel*         brightnessVal_ = nullptr;
    QLabel*         contrastVal_   = nullptr;
    QLabel*         saturationVal_ = nullptr;
    QLabel*         hueVal_        = nullptr;

    // Transform group
    QSlider*        posX_        = nullptr;
    QSlider*        posY_        = nullptr;
    QSlider*        scale_       = nullptr;
    QSlider*        rotation_    = nullptr;
    QLabel*         posXVal_     = nullptr;
    QLabel*         posYVal_     = nullptr;
    QLabel*         scaleVal_    = nullptr;
    QLabel*         rotationVal_ = nullptr;

    // Speed / fade / audio
    QDoubleSpinBox* speedSpin_   = nullptr;
    QSpinBox*       fadeIn_      = nullptr;
    QSpinBox*       fadeOut_     = nullptr;
    QSlider*        volume_      = nullptr;
    QSlider*        opacity_     = nullptr;
    QLabel*         volumeVal_   = nullptr;
    QLabel*         opacityVal_  = nullptr;

    EngineBridge* engine_   = nullptr;
    TimelineWidget* timeline_ = nullptr;
    uint64_t      projectId_ = 0;
    uint64_t      trackId_  = 0;
    uint64_t      clipId_   = 0;
    bool          inClipMode_ = false;
    bool          loading_  = false;
};

} // namespace beta
