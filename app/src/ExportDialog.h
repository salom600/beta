#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;
class QPushButton;

namespace beta {

struct ExportSettings {
    QString outputPath;
    QString container  = "mp4";
    QString videoCodec = "libx264";
    QString audioCodec = "aac";
    int     width       = 1920;
    int     height      = 1080;
    int     fps         = 30;
    int     videoBitrate = 5000;   // kbps
    int     audioBitrate = 128;    // kbps
    bool    includeAudio = true;
};

class ExportDialog : public QDialog {
    Q_OBJECT
public:
    explicit ExportDialog(QWidget* parent = nullptr);

    ExportSettings settings() const;
    void setProjectInfo(int trackCount, int clipCount, int width, int height, double fps);

private slots:
    void chooseOutputFile();

private:
    QLineEdit*       outputPathEdit_  = nullptr;
    QComboBox*       containerCombo_  = nullptr;
    QComboBox*       videoCodecCombo_ = nullptr;
    QComboBox*       audioCodecCombo_ = nullptr;
    QSpinBox*        widthSpin_       = nullptr;
    QSpinBox*        heightSpin_      = nullptr;
    QSpinBox*        fpsSpin_         = nullptr;
    QSpinBox*        videoBitrateSpin_ = nullptr;
    QSpinBox*        audioBitrateSpin_ = nullptr;
    QCheckBox*       includeAudioCheck_ = nullptr;
    QLabel*          infoLabel_       = nullptr;
};

} // namespace beta
