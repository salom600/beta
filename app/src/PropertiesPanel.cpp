#include "PropertiesPanel.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QVBoxLayout>

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
    outer->setContentsMargins(8, 8, 8, 8);

    title_ = new QLabel(tr("Properties"), this);
    QFont f = title_->font();
    f.setPointSize(12);
    f.setBold(true);
    title_->setFont(f);
    outer->addWidget(title_);

    pathValue_ = new QLabel(this);
    pathValue_->setWordWrap(true);
    pathValue_->setStyleSheet("color: #888; padding: 4px;");
    outer->addWidget(pathValue_);

    auto* box = new QGroupBox(tr("Clip"), this);
    auto* form = new QFormLayout(box);

    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(tr("Clip name"));
    form->addRow(tr("Name:"), nameEdit_);

    startFrame_ = new QSpinBox(this);
    startFrame_->setRange(0, 1'000'000);
    form->addRow(tr("Start frame:"), startFrame_);

    duration_ = new QSpinBox(this);
    duration_->setRange(1, 1'000'000);
    form->addRow(tr("Duration (frames):"), duration_);

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
}

void PropertiesPanel::showProjectInfo()
{
    title_->setText(tr("Project"));
    pathValue_->setText(tr("No media selected. Import a file and click a clip "
                           "on the timeline to edit its properties here."));
    nameEdit_->clear();
    startFrame_->setValue(0);
    duration_->setValue(150);
    volumeSpin_->setValue(1.0);
    opacitySpin_->setValue(1.0);
    scaleSpin_->setValue(1.0);
}

void PropertiesPanel::showMediaInfo(const QString& path, const QString& name)
{
    title_->setText(tr("Media"));
    pathValue_->setText(path);
    nameEdit_->setText(name);
}

void PropertiesPanel::showClipInfo(const QString& name, const QString& path,
                                    uint64_t start, uint64_t duration)
{
    title_->setText(tr("Clip"));
    if (!path.isEmpty()) pathValue_->setText(path);
    nameEdit_->setText(name);
    startFrame_->setValue(static_cast<int>(start));
    duration_->setValue(static_cast<int>(duration));
}

} // namespace beta
