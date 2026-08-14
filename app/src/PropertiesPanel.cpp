#include "PropertiesPanel.h"
#include "EngineBridge.h"
#include "TimelineWidget.h"

#include <QCheckBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QFrame>
#include <cmath>

namespace beta {

namespace {

QSlider* makeSlider(int lo, int hi, int step = 1) {
    auto* s = new QSlider(Qt::Horizontal);
    s->setRange(lo, hi);
    s->setSingleStep(step);
    return s;
}

QLabel* makeValueLabel() {
    auto* l = new QLabel("0");
    l->setMinimumWidth(40);
    l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    l->setObjectName("hintLabel");
    return l;
}

QHBoxLayout* sliderRow(QSlider* s, QLabel* v) {
    auto* row = new QHBoxLayout;
    row->addWidget(s, 1);
    row->addWidget(v);
    return row;
}

} // namespace

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

    // ---- Clip group (basic) ----
    auto* basicBox = new QGroupBox(tr("Clip"), this);
    auto* basicForm = new QFormLayout(basicBox);
    basicForm->setSpacing(6);

    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(tr("Clip name"));
    basicForm->addRow(tr("Name:"), nameEdit_);

    startFrame_ = new QSpinBox(this);
    startFrame_->setRange(0, 1'000'000);
    startFrame_->setSuffix(" f");
    basicForm->addRow(tr("Start:"), startFrame_);

    duration_ = new QSpinBox(this);
    duration_->setRange(1, 1'000'000);
    duration_->setSuffix(" f");
    basicForm->addRow(tr("Duration:"), duration_);

    trimIn_ = new QSpinBox(this);
    trimIn_->setRange(0, 1'000'000);
    trimIn_->setSuffix(" f");
    basicForm->addRow(tr("Trim in:"), trimIn_);

    outer->addWidget(basicBox);

    // ---- Color group ----
    auto* colorBox = new QGroupBox(tr("Color"), this);
    auto* colorForm = new QFormLayout(colorBox);
    colorForm->setSpacing(6);

    brightness_ = makeSlider(-100, 100);
    brightnessVal_ = makeValueLabel();
    colorForm->addRow(tr("Brightness:"), sliderRow(brightness_, brightnessVal_));

    contrast_ = makeSlider(-100, 100);
    contrastVal_ = makeValueLabel();
    colorForm->addRow(tr("Contrast:"), sliderRow(contrast_, contrastVal_));

    saturation_ = makeSlider(-100, 100);
    saturationVal_ = makeValueLabel();
    colorForm->addRow(tr("Saturation:"), sliderRow(saturation_, saturationVal_));

    hue_ = makeSlider(-180, 180);
    hueVal_ = makeValueLabel();
    colorForm->addRow(tr("Hue:"), sliderRow(hue_, hueVal_));

    auto* resetColorBtn = new QPushButton(tr("Reset Color"), this);
    colorForm->addRow(resetColorBtn);
    connect(resetColorBtn, &QPushButton::clicked, this, &PropertiesPanel::onResetColor);

    outer->addWidget(colorBox);

    // ---- Transform group ----
    auto* transformBox = new QGroupBox(tr("Transform"), this);
    auto* transformForm = new QFormLayout(transformBox);
    transformForm->setSpacing(6);

    posX_ = makeSlider(-100, 100);
    posXVal_ = makeValueLabel();
    transformForm->addRow(tr("Position X:"), sliderRow(posX_, posXVal_));

    posY_ = makeSlider(-100, 100);
    posYVal_ = makeValueLabel();
    transformForm->addRow(tr("Position Y:"), sliderRow(posY_, posYVal_));

    scale_ = makeSlider(10, 400);
    scaleVal_ = makeValueLabel();
    transformForm->addRow(tr("Scale:"), sliderRow(scale_, scaleVal_));

    rotation_ = makeSlider(-180, 180);
    rotationVal_ = makeValueLabel();
    transformForm->addRow(tr("Rotation:"), sliderRow(rotation_, rotationVal_));

    auto* resetTransformBtn = new QPushButton(tr("Reset Transform"), this);
    transformForm->addRow(resetTransformBtn);
    connect(resetTransformBtn, &QPushButton::clicked, this, &PropertiesPanel::onResetTransform);

    outer->addWidget(transformBox);

    // ---- Speed / fade ----
    auto* speedBox = new QGroupBox(tr("Speed & Fade"), this);
    auto* speedForm = new QFormLayout(speedBox);
    speedForm->setSpacing(6);

    speedSpin_ = new QDoubleSpinBox(this);
    speedSpin_->setRange(0.25, 4.0);
    speedSpin_->setSingleStep(0.05);
    speedSpin_->setValue(1.0);
    speedSpin_->setSuffix("x");
    speedForm->addRow(tr("Speed:"), speedSpin_);

    fadeIn_ = new QSpinBox(this);
    fadeIn_->setRange(0, 1'000'000);
    fadeIn_->setSuffix(" f");
    speedForm->addRow(tr("Fade in:"), fadeIn_);

    fadeOut_ = new QSpinBox(this);
    fadeOut_->setRange(0, 1'000'000);
    fadeOut_->setSuffix(" f");
    speedForm->addRow(tr("Fade out:"), fadeOut_);

    outer->addWidget(speedBox);

    // ---- Audio / opacity ----
    auto* audioBox = new QGroupBox(tr("Audio & Opacity"), this);
    auto* audioForm = new QFormLayout(audioBox);
    audioForm->setSpacing(6);

    volume_ = makeSlider(0, 200);
    volumeVal_ = makeValueLabel();
    audioForm->addRow(tr("Volume:"), sliderRow(volume_, volumeVal_));

    opacity_ = makeSlider(0, 100);
    opacityVal_ = makeValueLabel();
    audioForm->addRow(tr("Opacity:"), sliderRow(opacity_, opacityVal_));

    outer->addWidget(audioBox);
    outer->addStretch();

    // ---- Wire signals ----
    auto wireSlider = [this](QSlider* s, QLabel* v, std::function<void()> cb) {
        connect(s, &QSlider::valueChanged, this, [this, v, cb](int val) {
            v->setText(QString::number(val));
            if (!loading_) cb();
        });
    };
    wireSlider(brightness_, brightnessVal_, [this]{ onAdjustChanged(); });
    wireSlider(contrast_,   contrastVal_,   [this]{ onAdjustChanged(); });
    wireSlider(saturation_, saturationVal_, [this]{ onAdjustChanged(); });
    wireSlider(hue_,        hueVal_,        [this]{ onAdjustChanged(); });
    wireSlider(posX_,       posXVal_,       [this]{ onAdjustChanged(); });
    wireSlider(posY_,       posYVal_,       [this]{ onAdjustChanged(); });
    wireSlider(scale_,      scaleVal_,      [this]{ onAdjustChanged(); });
    wireSlider(rotation_,   rotationVal_,   [this]{ onAdjustChanged(); });
    wireSlider(volume_,     volumeVal_,     [this]{ onAdjustChanged(); });
    wireSlider(opacity_,    opacityVal_,    [this]{ onAdjustChanged(); });

    connect(speedSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double){ if (!loading_) onAdjustChanged(); });
    connect(fadeIn_,  qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int){ if (!loading_) onAdjustChanged(); });
    connect(fadeOut_, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int){ if (!loading_) onAdjustChanged(); });
    connect(startFrame_, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int){ if (!loading_) onAdjustChanged(); });
    connect(duration_,   qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int){ if (!loading_) onAdjustChanged(); });
    connect(trimIn_,     qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int){ if (!loading_) onAdjustChanged(); });
}

void PropertiesPanel::setClipMode(bool enabled)
{
    inClipMode_ = enabled;
    startFrame_->setEnabled(enabled);
    duration_->setEnabled(enabled);
    trimIn_->setEnabled(enabled);
    brightness_->setEnabled(enabled);
    contrast_->setEnabled(enabled);
    saturation_->setEnabled(enabled);
    hue_->setEnabled(enabled);
    posX_->setEnabled(enabled);
    posY_->setEnabled(enabled);
    scale_->setEnabled(enabled);
    rotation_->setEnabled(enabled);
    speedSpin_->setEnabled(enabled);
    fadeIn_->setEnabled(enabled);
    fadeOut_->setEnabled(enabled);
    volume_->setEnabled(enabled);
    opacity_->setEnabled(enabled);
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
    brightness_->setValue(0);
    contrast_->setValue(0);
    saturation_->setValue(0);
    hue_->setValue(0);
    posX_->setValue(0);
    posY_->setValue(0);
    scale_->setValue(100);
    rotation_->setValue(0);
    speedSpin_->setValue(1.0);
    fadeIn_->setValue(0);
    fadeOut_->setValue(0);
    volume_->setValue(100);
    opacity_->setValue(100);
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
                                    const EngineBridge::ClipAdjust& adjust,
                                    uint64_t trackId, uint64_t clipId)
{
    loading_ = true;
    title_->setText(tr("Clip"));
    if (!path.isEmpty()) pathValue_->setText(path);
    nameEdit_->setText(name);
    trackId_ = trackId;
    clipId_  = clipId;

    startFrame_->setValue(static_cast<int>(start));
    duration_->setValue(static_cast<int>(duration));
    trimIn_->setValue(static_cast<int>(trimIn));

    brightness_->setValue(static_cast<int>(adjust.brightness * 100));
    contrast_->setValue(static_cast<int>(adjust.contrast * 100));
    saturation_->setValue(static_cast<int>(adjust.saturation * 100));
    hue_->setValue(static_cast<int>(adjust.hue));
    posX_->setValue(static_cast<int>(adjust.posX * 100));
    posY_->setValue(static_cast<int>(adjust.posY * 100));
    scale_->setValue(static_cast<int>(adjust.scale * 100));
    rotation_->setValue(static_cast<int>(adjust.rotation));
    speedSpin_->setValue(adjust.speed);
    fadeIn_->setValue(static_cast<int>(adjust.fadeIn));
    fadeOut_->setValue(static_cast<int>(adjust.fadeOut));
    volume_->setValue(static_cast<int>(adjust.volume * 100));
    opacity_->setValue(static_cast<int>(adjust.opacity * 100));

    brightnessVal_->setText(QString::number(brightness_->value()));
    contrastVal_->setText(QString::number(contrast_->value()));
    saturationVal_->setText(QString::number(saturation_->value()));
    hueVal_->setText(QString::number(hue_->value()));
    posXVal_->setText(QString::number(posX_->value()));
    posYVal_->setText(QString::number(posY_->value()));
    scaleVal_->setText(QString::number(scale_->value()));
    rotationVal_->setText(QString::number(rotation_->value()));
    volumeVal_->setText(QString::number(volume_->value()));
    opacityVal_->setText(QString::number(opacity_->value()));

    loading_ = false;
    setClipMode(true);
}

void PropertiesPanel::onAdjustChanged()
{
    if (!engine_ || !inClipMode_ || clipId_ == 0 || trackId_ == 0) return;
    pushAdjustToEngine();
    // Also push the basic clip props (start/duration/trim)
    engine_->moveClip(projectId_, trackId_, clipId_,
                      static_cast<uint64_t>(startFrame_->value()));
    engine_->trimClip(projectId_, trackId_, clipId_,
                      static_cast<uint64_t>(trimIn_->value()),
                      static_cast<uint64_t>(duration_->value()));
    if (timeline_) timeline_->refreshTracks();
}

void PropertiesPanel::pushAdjustToEngine()
{
    EngineBridge::ClipAdjust a;
    a.brightness = brightness_->value() / 100.0f;
    a.contrast   = contrast_->value()   / 100.0f;
    a.saturation = saturation_->value() / 100.0f;
    a.hue        = static_cast<float>(hue_->value());
    a.posX       = posX_->value() / 100.0f;
    a.posY       = posY_->value() / 100.0f;
    a.scale      = scale_->value() / 100.0f;
    a.rotation   = static_cast<float>(rotation_->value());
    a.speed      = static_cast<float>(speedSpin_->value());
    a.fadeIn     = static_cast<uint64_t>(fadeIn_->value());
    a.fadeOut    = static_cast<uint64_t>(fadeOut_->value());
    a.volume     = volume_->value() / 100.0f;
    a.opacity    = opacity_->value() / 100.0f;
    engine_->setClipAdjust(projectId_, trackId_, clipId_, a);
}

void PropertiesPanel::onResetColor()
{
    if (loading_) return;
    loading_ = true;
    brightness_->setValue(0);
    contrast_->setValue(0);
    saturation_->setValue(0);
    hue_->setValue(0);
    brightnessVal_->setText("0");
    contrastVal_->setText("0");
    saturationVal_->setText("0");
    hueVal_->setText("0");
    loading_ = false;
    onAdjustChanged();
}

void PropertiesPanel::onResetTransform()
{
    if (loading_) return;
    loading_ = true;
    posX_->setValue(0);
    posY_->setValue(0);
    scale_->setValue(100);
    rotation_->setValue(0);
    posXVal_->setText("0");
    posYVal_->setText("0");
    scaleVal_->setText("100");
    rotationVal_->setText("0");
    loading_ = false;
    onAdjustChanged();
}

} // namespace beta
