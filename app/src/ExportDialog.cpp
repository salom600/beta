#include "ExportDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <QFileInfo>

namespace beta {

ExportDialog::ExportDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Export Project"));
    setMinimumWidth(480);

    auto* outer = new QVBoxLayout(this);

    infoLabel_ = new QLabel(this);
    infoLabel_->setObjectName("hintLabel");
    outer->addWidget(infoLabel_);

    // Output group
    auto* outBox = new QGroupBox(tr("Output"), this);
    auto* outForm = new QFormLayout(outBox);

    outputPathEdit_ = new QLineEdit(this);
    outputPathEdit_->setPlaceholderText(tr("Choose output file..."));
    auto* browseBtn = new QPushButton(tr("Browse..."), this);
    auto* outRow = new QHBoxLayout;
    outRow->addWidget(outputPathEdit_, 1);
    outRow->addWidget(browseBtn);
    outForm->addRow(tr("File:"), outRow);
    connect(browseBtn, &QPushButton::clicked, this, &ExportDialog::chooseOutputFile);

    containerCombo_ = new QComboBox(this);
    containerCombo_->addItems({ "mp4", "mkv", "webm", "mov", "avi" });
    outForm->addRow(tr("Container:"), containerCombo_);

    outer->addWidget(outBox);

    // Video group
    auto* videoBox = new QGroupBox(tr("Video"), this);
    auto* vForm = new QFormLayout(videoBox);

    videoCodecCombo_ = new QComboBox(this);
    videoCodecCombo_->addItems({ "libx264", "libx265", "libvpx-vp9", "mpeg4", "copy" });
    vForm->addRow(tr("Codec:"), videoCodecCombo_);

    widthSpin_ = new QSpinBox(this);
    widthSpin_->setRange(16, 8192);
    widthSpin_->setValue(1920);
    heightSpin_ = new QSpinBox(this);
    heightSpin_->setRange(16, 8192);
    heightSpin_->setValue(1080);
    auto* resRow = new QHBoxLayout;
    resRow->addWidget(widthSpin_);
    resRow->addWidget(new QLabel("×", this));
    resRow->addWidget(heightSpin_);
    resRow->addStretch();
    vForm->addRow(tr("Resolution:"), resRow);

    fpsSpin_ = new QSpinBox(this);
    fpsSpin_->setRange(1, 120);
    fpsSpin_->setValue(30);
    vForm->addRow(tr("Frame rate:"), fpsSpin_);

    videoBitrateSpin_ = new QSpinBox(this);
    videoBitrateSpin_->setRange(100, 50000);
    videoBitrateSpin_->setSingleStep(100);
    videoBitrateSpin_->setValue(5000);
    videoBitrateSpin_->setSuffix(" kbps");
    vForm->addRow(tr("Bitrate:"), videoBitrateSpin_);

    outer->addWidget(videoBox);

    // Audio group
    auto* audioBox = new QGroupBox(tr("Audio"), this);
    auto* aForm = new QFormLayout(audioBox);

    includeAudioCheck_ = new QCheckBox(tr("Include audio"), this);
    includeAudioCheck_->setChecked(true);
    aForm->addRow(includeAudioCheck_);

    audioCodecCombo_ = new QComboBox(this);
    audioCodecCombo_->addItems({ "aac", "libmp3lame", "libopus", "libvorbis", "copy" });
    aForm->addRow(tr("Codec:"), audioCodecCombo_);

    audioBitrateSpin_ = new QSpinBox(this);
    audioBitrateSpin_->setRange(32, 1024);
    audioBitrateSpin_->setSingleStep(16);
    audioBitrateSpin_->setValue(128);
    audioBitrateSpin_->setSuffix(" kbps");
    aForm->addRow(tr("Bitrate:"), audioBitrateSpin_);

    outer->addWidget(audioBox);

    // Buttons
    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setText(tr("Export"));
    btns->button(QDialogButtonBox::Ok)->setObjectName("primaryBtn");
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    outer->addWidget(btns);
}

void ExportDialog::chooseOutputFile()
{
    QString filter = tr("Video Files (*.mp4 *.mkv *.webm *.mov *.avi);;All Files (*)");
    QString fn = QFileDialog::getSaveFileName(this, tr("Export Video"),
                                              QString(), filter);
    if (!fn.isEmpty()) {
        outputPathEdit_->setText(fn);
        // Auto-set container based on extension
        QString ext = QFileInfo(fn).suffix().toLower();
        int idx = containerCombo_->findText(ext);
        if (idx >= 0) containerCombo_->setCurrentIndex(idx);
    }
}

ExportSettings ExportDialog::settings() const
{
    ExportSettings s;
    s.outputPath    = outputPathEdit_->text().trimmed();
    s.container     = containerCombo_->currentText();
    s.videoCodec    = videoCodecCombo_->currentText();
    s.audioCodec    = audioCodecCombo_->currentText();
    s.width         = widthSpin_->value();
    s.height        = heightSpin_->value();
    s.fps           = fpsSpin_->value();
    s.videoBitrate  = videoBitrateSpin_->value();
    s.audioBitrate  = audioBitrateSpin_->value();
    s.includeAudio  = includeAudioCheck_->isChecked();
    return s;
}

void ExportDialog::setProjectInfo(int trackCount, int clipCount,
                                   int width, int height, double fps)
{
    infoLabel_->setText(tr("Project: %1 tracks • %2 clips • %3×%4 @ %5 fps")
                            .arg(trackCount).arg(clipCount)
                            .arg(width).arg(height).arg(fps, 0, 'f', 2));
    if (width > 0 && height > 0) {
        widthSpin_->setValue(width);
        heightSpin_->setValue(height);
    }
    if (fps > 0) fpsSpin_->setValue(static_cast<int>(fps));
}

} // namespace beta
