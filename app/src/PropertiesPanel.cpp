#include "PropertiesPanel.h"
#include "EngineBridge.h"

#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QFrame>

namespace beta {

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    showProjectInfo();
}

void PropertiesPanel::setupUi()
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(10, 10, 10, 10);
    outer->setSpacing(8);

    title_ = new QLabel(tr("Properties"), this);
    title_->setObjectName("titleLabel");
    outer->addWidget(title_);

    pathValue_ = new QLabel(this);
    pathValue_->setObjectName("hintLabel");
    pathValue_->setWordWrap(true);
    pathValue_->setContentsMargins(0, 0, 0, 8);
    outer->addWidget(pathValue_);

    auto* box = new QGroupBox(tr("Clip"), this);
    auto* form = new QFormLayout(box);
    form->setSpacing(6);

    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(tr("Clip name"));
    form->addRow(tr("Name:"), nameEdit_);

    startFrame_ = new QSpinBox(this);
    startFrame_->setRange(0, 1'000'000);
    startFrame_->setSuffix(" f");
    form->addRow(tr("Start:"), startFrame_);

    duration_ = new QSpinBox(this);
    duration_->setRange(1, 1'000'000);
    duration_->setSuffix(" f");
    form->addRow(tr("Duration:"), duration_);

    trimIn_ = new QSpinBox(this);
    trimIn_->setRange(0, 1'000'000);
    trimIn_->setSuffix(" f");
    form->addRow(tr("Trim in:"), trimIn_);

    volumeSpin_ = new QDoubleSpinBox(this);
    volumeSpin_->setRange(0.0, 2.0);
    volumeSpin_->setSingleStep(0.05);
    volumeSpin_->setValue(1.0);
    form->addRow(tr("Volume:"), volumeSpin_);

    opacitySpin_ = new QDoubleSpinBox(this);
    opacitySpin_->setRange(0.0, 1.0);
    opacitySpin_->setSingleStep(0.05);
    opacitySpin_->setValue(1.0);
    form->addRow(tr("Opacity:"), opacitySpin_);

    scaleSpin_ = new QDoubleSpinBox(this);
    scaleSpin_->setRange(0.1, 4.0);
    scaleSpin_->setSingleStep(0.05);
    scaleSpin_->setValue(1.0);
    form->addRow(tr("Scale:"), scaleSpin_);

    outer->addWidget(box);
    outer->addStretch();

    connect(volumeSpin_,  qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &PropertiesPanel::onPropsChanged);
    connect(opacitySpin_, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &PropertiesPanel::onPropsChanged);
    connect(scaleSpin_,   qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &PropertiesPanel::onPropsChanged);
    connect(startFrame_,  qOverload<int>(&QSpinBox::valueChanged),
            this, &PropertiesPanel::onPropsChanged);
    connect(duration_,    qOverload<int>(&QSpinBox::valueChanged),
            this, &PropertiesPanel::onPropsChanged);
    connect(trimIn_,      qOverload<int>(&QSpinBox::valueChanged),
            this, &PropertiesPanel::onPropsChanged);
}

void PropertiesPanel::setClipMode(bool enabled)
{
    inClipMode_ = enabled;
    startFrame_->setEnabled(enabled);
    duration_->setEnabled(enabled);
    trimIn_->setEnabled(enabled);
    volumeSpin_->setEnabled(enabled);
    opacitySpin_->setEnabled(enabled);
    scaleSpin_->setEnabled(enabled);
}

void PropertiesPanel::showProjectInfo()
{
    title_->setText(tr("Project"));
    pathValue_->setText(tr("No clip selected. Click a clip on the "
                           "timeline to edit its properties here."));
    nameEdit_->clear();
    startFrame_->setValue(0);
    duration_->setValue(150);
    trimIn_->setValue(0);
    volumeSpin_->setValue(1.0);
    opacitySpin_->setValue(1.0);
    scaleSpin_->setValue(1.0);
    setClipMode(false);
    clipId_ = 0;
    trackId_ = 0;
}

void PropertiesPanel::clearSelection()
{
    showProjectInfo();
}

void PropertiesPanel::showMediaInfo(const QString& path, const QString& name)
{
    title_->setText(tr("Media"));
    pathValue_->setText(path);
    nameEdit_->setText(name);
    setClipMode(false);
    clipId_ = 0;
    trackId_ = 0;
}

void PropertiesPanel::showClipInfo(const QString& name, const QString& path,
                                    uint64_t start, uint64_t duration,
                                    uint64_t trimIn,
                                    double volume, double opacity, double scale,
                                    uint64_t trackId, uint64_t clipId)
{
    title_->setText(tr("Clip"));
    if (!path.isEmpty()) pathValue_->setText(path);
    nameEdit_->setText(name);
    trackId_ = trackId;
    clipId_  = clipId;
    // Block signals so we don't write back to the engine while loading
    startFrame_->blockSignals(true);
    duration_->blockSignals(true);
    trimIn_->blockSignals(true);
    volumeSpin_->blockSignals(true);
    opacitySpin_->blockSignals(true);
    scaleSpin_->blockSignals(true);

    startFrame_->setValue(static_cast<int>(start));
    duration_->setValue(static_cast<int>(duration));
    trimIn_->setValue(static_cast<int>(trimIn));
    volumeSpin_->setValue(volume);
    opacitySpin_->setValue(opacity);
    scaleSpin_->setValue(scale);

    startFrame_->blockSignals(false);
    duration_->blockSignals(false);
    trimIn_->blockSignals(false);
    volumeSpin_->blockSignals(false);
    opacitySpin_->blockSignals(false);
    scaleSpin_->blockSignals(false);

    setClipMode(true);
}

void PropertiesPanel::onPropsChanged()
{
    if (!engine_ || !inClipMode_ || clipId_ == 0 || trackId_ == 0) return;
    engine_->setClipProps(projectId_, trackId_, clipId_,
        static_cast<float>(volumeSpin_->value()),
        static_cast<float>(opacitySpin_->value()),
        static_cast<float>(scaleSpin_->value()));
    engine_->moveClip(projectId_, trackId_, clipId_,
                      static_cast<uint64_t>(startFrame_->value()));
    engine_->trimClip(projectId_, trackId_, clipId_,
                      static_cast<uint64_t>(trimIn_->value()),
                      static_cast<uint64_t>(duration_->value()));
}

} // namespace beta
