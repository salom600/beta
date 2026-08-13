#pragma once

#include <QWidget>
#include <cstdint>

class QFormLayout;
class QLabel;
class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;

namespace beta {

/// Right-side panel. Shows properties of the currently selected clip
/// or media asset. When nothing is selected, shows project info.
class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr);

public slots:
    void showMediaInfo(const QString& path, const QString& name);
    void showClipInfo(const QString& name, const QString& path,
                      uint64_t start, uint64_t duration);
    void showProjectInfo();

private:
    void setupUi();

    QLabel*         title_       = nullptr;
    QLabel*         pathValue_   = nullptr;
    QLineEdit*      nameEdit_    = nullptr;
    QDoubleSpinBox* volumeSpin_  = nullptr;
    QDoubleSpinBox* opacitySpin_ = nullptr;
    QDoubleSpinBox* scaleSpin_   = nullptr;
    QSpinBox*       startFrame_  = nullptr;
    QSpinBox*       duration_    = nullptr;
};

} // namespace beta
