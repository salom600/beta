#include "EffectsPanel.h"

#include <QAction>
#include <QHeaderView>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QToolBar>
#include <QVBoxLayout>
#include <QLabel>

namespace beta {

namespace {

// Built-in effects catalog. In Kdenlive this comes from MLT's
// asset repository; for v0.6 we ship a curated list of common
// adjustments that map to our ClipAdjust fields.
const struct { const char* id; const char* name; const char* category; const char* desc; } kBuiltin[] = {
    {"brightness",     "Brightness",      "Color",      "Adjust image brightness"},
    {"contrast",       "Contrast",        "Color",      "Adjust image contrast"},
    {"saturation",     "Saturation",      "Color",      "Adjust color saturation"},
    {"hue",            "Hue Shift",       "Color",      "Shift hue degrees"},
    {"color_balance",  "Color Balance",   "Color",      "Combined brightness/contrast/saturation"},
    {"grayscale",      "Grayscale",       "Color",      "Desaturate to grayscale"},
    {"invert",         "Invert Colors",   "Color",      "Invert all color channels"},

    {"transform",      "Transform",       "Geometry",   "Position, scale, rotation"},
    {"position",       "Position",        "Geometry",   "Move clip in 2D"},
    {"scale",          "Scale",           "Geometry",   "Resize clip"},
    {"rotate",         "Rotate",          "Geometry",   "Rotate clip around center"},
    {"crop",           "Crop",            "Geometry",   "Crop edges"},

    {"speed",          "Speed",           "Time",       "Change playback speed"},
    {"reverse",        "Reverse",         "Time",       "Play clip backwards"},
    {"freeze_frame",   "Freeze Frame",    "Time",       "Hold a single frame"},

    {"fade_in",        "Fade In",         "Fade",       "Fade audio/video in"},
    {"fade_out",       "Fade Out",        "Fade",       "Fade audio/video out"},
    {"dissolve",       "Dissolve",        "Fade",       "Cross-dissolve transition"},

    {"volume",         "Volume",          "Audio",      "Adjust audio volume"},
    {"mute",           "Mute",            "Audio",      "Silence audio"},
    {"normalize",      "Normalize",       "Audio",      "Peak normalize audio"},

    {"blur",           "Blur",            "Blur",       "Gaussian blur"},
    {"sharpen",        "Sharpen",         "Blur",       "Unsharp mask"},
    {"vignette",       "Vignette",        "Stylize",    "Darken edges"},
    {"grain",          "Film Grain",      "Stylize",    "Add grain noise"},
    {"letterbox",      "Letterbox",       "Stylize",    "Add cinematic bars"},
};

} // namespace

EffectsPanel::EffectsPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    loadBuiltinEffects();
}

void EffectsPanel::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Top toolbar with search
    auto* topBar = new QWidget(this);
    topBar->setStyleSheet("background: #2a2b30; border-bottom: 1px solid #15161a;");
    topBar->setFixedHeight(36);
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(8, 4, 8, 4);

    auto* titleLabel = new QLabel(tr("Effects"), topBar);
    titleLabel->setStyleSheet("color: #ffffff; font-weight: 600; font-size: 13px;");
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();

    searchEdit_ = new QLineEdit(topBar);
    searchEdit_->setPlaceholderText(tr("Search..."));
    searchEdit_->setFixedWidth(140);
    searchEdit_->setStyleSheet(
        "QLineEdit { background: #15161a; border: 1px solid #3c3d44; "
        "border-radius: 3px; padding: 3px 8px; color: #e6e6e8; }"
        "QLineEdit:focus { border-color: #0e63d4; }");
    connect(searchEdit_, &QLineEdit::textChanged,
            this, &EffectsPanel::onFilterChanged);
    topLayout->addWidget(searchEdit_);

    layout->addWidget(topBar);

    // Effects list
    list_ = new QListWidget(this);
    list_->setStyleSheet(
        "QListWidget { background: #1e1f22; border: none; outline: 0; }"
        "QListWidget::item { background: transparent; border-bottom: 1px solid #25262a; "
        "padding: 8px 12px; color: #e6e6e8; }"
        "QListWidget::item:hover { background: #2a2b30; }"
        "QListWidget::item:selected { background: #0e63d4; color: #ffffff; }");
    list_->setIconSize(QSize(20, 20));
    list_->setUniformItemSizes(true);
    list_->setSortingEnabled(true);
    connect(list_, &QListWidget::itemActivated,
            this, &EffectsPanel::onItemActivated);
    connect(list_, &QListWidget::itemSelectionChanged, this, [this]() {
        auto items = list_->selectedItems();
        if (!items.isEmpty()) {
            emit effectActivated(items.first()->data(Qt::UserRole).toString(),
                                  items.first()->text());
        }
    });

    layout->addWidget(list_, 1);
}

void EffectsPanel::loadBuiltinEffects()
{
    allEffects_.clear();
    for (const auto& e : kBuiltin) {
        EffectEntry entry;
        entry.id = QString::fromLatin1(e.id);
        entry.name = QString::fromLatin1(e.name);
        entry.category = QString::fromLatin1(e.category);
        entry.description = QString::fromLatin1(e.desc);
        allEffects_.append(entry);
    }

    // Populate the list widget
    list_->clear();
    for (const auto& e : allEffects_) {
        auto* item = new QListWidgetItem(list_);
        item->setText(e.name);
        item->setData(Qt::UserRole, e.id);
        item->setToolTip(QString("%1\n%2\n\nCategory: %3")
                            .arg(e.name, e.description, e.category));
        // Category-based icon
        QString iconPath;
        if (e.category == "Color")      iconPath = ":/icons/image-track.svg";
        else if (e.category == "Audio") iconPath = ":/icons/audio-track.svg";
        else if (e.category == "Geometry") iconPath = ":/icons/video-track.svg";
        else                             iconPath = ":/icons/film.svg";
        item->setIcon(QIcon(iconPath));
        list_->addItem(item);
    }
}

void EffectsPanel::onItemActivated(QListWidgetItem* item)
{
    if (!item) return;
    emit effectActivated(item->data(Qt::UserRole).toString(), item->text());
}

void EffectsPanel::onFilterChanged(const QString& text)
{
    QString filter = text.trimmed().toLower();
    for (int i = 0; i < list_->count(); ++i) {
        QListWidgetItem* item = list_->item(i);
        bool match = item->text().toLower().contains(filter) ||
                     item->toolTip().toLower().contains(filter);
        item->setHidden(!match);
    }
}

} // namespace beta
