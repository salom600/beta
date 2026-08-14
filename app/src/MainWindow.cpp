#include "MainWindow.h"
#include "EngineBridge.h"
#include "MediaBrowser.h"
#include "MediaProber.h"
#include "PreviewWidget.h"
#include "TimelineWidget.h"
#include "PropertiesPanel.h"
#include "EffectsPanel.h"
#include "ExportDialog.h"
#include "Exporter.h"
#include "Project.h"
#include "Timecode.h"
#include "Tool.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyle>
#include <QStyleFactory>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QFile>
#include <QPixmap>

namespace beta {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Beta — Video Editor"));
    resize(1680, 1000);
    setMinimumSize(1280, 750);
    setWindowIcon(QIcon(":/icons/app-icon.svg"));
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

    engine_   = std::make_unique<EngineBridge>();
    prober_   = std::make_unique<MediaProber>();
    exporter_ = std::make_unique<Exporter>();
    openProject();

    applyTheme();
    setupActions();
    setupDockWidgets();
    setupToolbars();
    setupMenubar();
    setupStatusbar();

    // Restore window state
    QSettings s;
    if (s.contains("windowState")) {
        restoreState(s.value("windowState").toByteArray(), 1);
    }
    if (s.contains("geometry")) {
        restoreGeometry(s.value("geometry").toByteArray());
    }
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* e)
{
    QSettings s;
    s.setValue("geometry", saveGeometry());
    s.setValue("windowState", saveState(1));
    e->accept();
}

void MainWindow::keyPressEvent(QKeyEvent* e)
{
    QWidget* focus = QApplication::focusWidget();
    bool editingText = focus && (
        qobject_cast<QLineEdit*>(focus) ||
        qobject_cast<QSpinBox*>(focus) ||
        qobject_cast<QDoubleSpinBox*>(focus) ||
        qobject_cast<QPlainTextEdit*>(focus));
    if (editingText) {
        QMainWindow::keyPressEvent(e);
        return;
    }

    switch (e->key()) {
    case Qt::Key_Space:      onPlayPause(); return;
    // Kdenlive shortcuts: S=select, X=razor, M=spacer (NOT V/C/T like Premiere)
    case Qt::Key_S:          if (e->modifiers() == Qt::NoModifier) { setTool(Tool::SelectTool); return; } break;
    case Qt::Key_X:          if (e->modifiers() == Qt::NoModifier) { setTool(Tool::RazorTool);  return; } break;
    case Qt::Key_M:          if (e->modifiers() == Qt::NoModifier) { setTool(Tool::SpacerTool); return; } break;
    case Qt::Key_R:          if (e->modifiers() == Qt::NoModifier) { setTool(Tool::RippleTool); return; } break;
    // Keep V/C/T/H as alternates for Premiere-familiar users
    case Qt::Key_V:          if (e->modifiers() == Qt::NoModifier) { setTool(Tool::SelectTool); return; } break;
    case Qt::Key_C:          if (e->modifiers() == Qt::NoModifier) { setTool(Tool::RazorTool);  return; } break;
    case Qt::Key_T:          if (e->modifiers() == Qt::NoModifier) { setTool(Tool::SpacerTool); return; } break;
    case Qt::Key_H:          if (e->modifiers() == Qt::NoModifier) { setTool(Tool::SpacerTool); return; } break;
    // Shift+S = split (was S in v0.4; now S=select per Kdenlive)
    case Qt::Key_P:          if (e->modifiers() == Qt::NoModifier) { onSplit(); return; } break;
    case Qt::Key_Delete:
    case Qt::Key_Backspace: onDelete(); return;
    case Qt::Key_Home:      onSkipStart(); return;
    case Qt::Key_End:       onSkipEnd(); return;
    case Qt::Key_PageDown:  onNextCut(); return;
    case Qt::Key_PageUp:    onPrevCut(); return;
    case Qt::Key_J:         /* TODO: shuttle backward */ return;
    case Qt::Key_L:         /* TODO: shuttle forward */  return;
    case Qt::Key_Plus:
    case Qt::Key_Equal:     onZoomIn(); return;
    case Qt::Key_Minus:     onZoomOut(); return;
    case Qt::Key_Left:
        if (e->modifiers() & Qt::ControlModifier) {
            timeline_->setPlayheadFrame(timeline_->playheadFrame() > 0 ? timeline_->playheadFrame() - 1 : 0);
            return;
        }
        break;
    case Qt::Key_Right:
        if (e->modifiers() & Qt::ControlModifier) {
            timeline_->setPlayheadFrame(timeline_->playheadFrame() + 1);
            return;
        }
        break;
    default: break;
    }
    QMainWindow::keyPressEvent(e);
}

void MainWindow::openProject()
{
    project_ = std::make_unique<Project>(engine_.get(), prober_.get());
    project_->setTimeline(nullptr); // will be set in setupDockWidgets
}

void MainWindow::applyTheme()
{
    if (QStyleFactory::keys().contains("Fusion")) {
        qApp->setStyle(QStyleFactory::create("Fusion"));
    }
    QPalette p = qApp->palette();
    p.setColor(QPalette::Window,          QColor( 24,  25,  28));
    p.setColor(QPalette::WindowText,      QColor(220, 221, 226));
    p.setColor(QPalette::Base,            QColor( 28,  29,  32));
    p.setColor(QPalette::AlternateBase,   QColor( 34,  35,  40));
    p.setColor(QPalette::Text,            QColor(220, 221, 226));
    p.setColor(QPalette::Button,          QColor( 40,  41,  46));
    p.setColor(QPalette::ButtonText,      QColor(220, 221, 226));
    p.setColor(QPalette::BrightText,      Qt::white);
    p.setColor(QPalette::Highlight,       QColor( 14,  99, 212));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase,     QColor( 40,  41,  46));
    p.setColor(QPalette::ToolTipText,     QColor(240, 240, 242));
    qApp->setPalette(p);

    QFile qss(":/dark.qss");
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qApp->setStyleSheet(QString::fromUtf8(qss.readAll()));
    }
}

void MainWindow::setupActions()
{
    actImport_ = new QAction(QIcon(":/icons/import.svg"), tr("&Import Media..."), this);
    actImport_->setShortcut(QKeySequence("Ctrl+I"));
    actImport_->setToolTip(tr("Import media files (Ctrl+I)"));
    connect(actImport_, &QAction::triggered, this, &MainWindow::onImportMedia);

    actPlayPause_ = new QAction(QIcon(":/icons/play.svg"), tr("&Play / Pause"), this);
    actPlayPause_->setShortcut(QKeySequence("Space"));
    connect(actPlayPause_, &QAction::triggered, this, &MainWindow::onPlayPause);

    actStop_ = new QAction(QIcon(":/icons/stop.svg"), tr("&Stop"), this);
    connect(actStop_, &QAction::triggered, this, &MainWindow::onStop);

    actSkipStart_ = new QAction(tr("Skip to Start"), this);
    actSkipStart_->setShortcut(QKeySequence("Home"));
    connect(actSkipStart_, &QAction::triggered, this, &MainWindow::onSkipStart);

    actSkipEnd_ = new QAction(tr("Skip to End"), this);
    actSkipEnd_->setShortcut(QKeySequence("End"));
    connect(actSkipEnd_, &QAction::triggered, this, &MainWindow::onSkipEnd);

    actNextCut_ = new QAction(tr("Next Cut"), this);
    actNextCut_->setShortcut(QKeySequence("PgDown"));
    connect(actNextCut_, &QAction::triggered, this, &MainWindow::onNextCut);

    actPrevCut_ = new QAction(tr("Previous Cut"), this);
    actPrevCut_->setShortcut(QKeySequence("PgUp"));
    connect(actPrevCut_, &QAction::triggered, this, &MainWindow::onPrevCut);

    actAddVideoTrack_ = new QAction(QIcon(":/icons/video-track.svg"), tr("Video Track"), this);
    connect(actAddVideoTrack_, &QAction::triggered, this, &MainWindow::onAddVideoTrack);

    actAddAudioTrack_ = new QAction(QIcon(":/icons/audio-track.svg"), tr("Audio Track"), this);
    connect(actAddAudioTrack_, &QAction::triggered, this, &MainWindow::onAddAudioTrack);

    actAddImageTrack_ = new QAction(QIcon(":/icons/image-track.svg"), tr("Image Track"), this);
    connect(actAddImageTrack_, &QAction::triggered, this, &MainWindow::onAddImageTrack);

    actSplit_ = new QAction(QIcon(":/icons/split.svg"), tr("Split"), this);
    actSplit_->setShortcut(QKeySequence("P"));  // Kdenlive: S=select, so split=P
    actSplit_->setToolTip(tr("Split clip at playhead (P)"));
    connect(actSplit_, &QAction::triggered, this, &MainWindow::onSplit);

    actCut_ = new QAction(QIcon(":/icons/cut.svg"), tr("Cut"), this);
    actCut_->setShortcut(QKeySequence("Ctrl+X"));
    actCut_->setToolTip(tr("Cut: split + delete right half (Ctrl+X)"));
    connect(actCut_, &QAction::triggered, this, &MainWindow::onCut);

    actMerge_ = new QAction(QIcon(":/icons/merge.svg"), tr("Merge"), this);
    actMerge_->setShortcut(QKeySequence("Ctrl+M"));  // M=spacer, so merge=Ctrl+M
    actMerge_->setToolTip(tr("Merge with next clip (Ctrl+M)"));
    connect(actMerge_, &QAction::triggered, this, &MainWindow::onMerge);

    actDelete_ = new QAction(QIcon(":/icons/delete.svg"), tr("Delete"), this);
    actDelete_->setShortcut(QKeySequence("Del"));
    actDelete_->setToolTip(tr("Delete selected clip (Del)"));
    connect(actDelete_, &QAction::triggered, this, &MainWindow::onDelete);

    actExport_ = new QAction(QIcon(":/icons/export.svg"), tr("&Export..."), this);
    actExport_->setShortcut(QKeySequence("Ctrl+E"));
    connect(actExport_, &QAction::triggered, this, &MainWindow::onExport);

    actAbout_ = new QAction(tr("About Beta..."), this);
    connect(actAbout_, &QAction::triggered, this, &MainWindow::onAbout);

    actUndo_ = new QAction(tr("&Undo"), this);
    actUndo_->setShortcut(QKeySequence("Ctrl+Z"));
    connect(actUndo_, &QAction::triggered, this, &MainWindow::onUndo);

    actRedo_ = new QAction(tr("&Redo"), this);
    actRedo_->setShortcut(QKeySequence("Ctrl+Shift+Z"));
    connect(actRedo_, &QAction::triggered, this, &MainWindow::onRedo);

    actZoomIn_ = new QAction(tr("Zoom In"), this);
    actZoomIn_->setShortcut(QKeySequence("+"));
    connect(actZoomIn_, &QAction::triggered, this, &MainWindow::onZoomIn);

    actZoomOut_ = new QAction(tr("Zoom Out"), this);
    actZoomOut_->setShortcut(QKeySequence("-"));
    connect(actZoomOut_, &QAction::triggered, this, &MainWindow::onZoomOut);

    // Tools — Kdenlive keyboard shortcuts (S/X/M, plus V/C/T/H alternates)
    toolGroup_ = new QActionGroup(this);
    toolGroup_->setExclusive(true);

    actToolSelect_ = new QAction(QIcon(":/icons/split.svg"), tr("Select Tool"), this);
    actToolSelect_->setShortcuts({QKeySequence("S"), QKeySequence("V")});
    actToolSelect_->setCheckable(true);
    actToolSelect_->setChecked(true);
    connect(actToolSelect_, &QAction::triggered, this, &MainWindow::onToolSelect);
    toolGroup_->addAction(actToolSelect_);

    actToolRazor_ = new QAction(QIcon(":/icons/cut.svg"), tr("Razor Tool"), this);
    actToolRazor_->setShortcuts({QKeySequence("X"), QKeySequence("C")});
    actToolRazor_->setCheckable(true);
    connect(actToolRazor_, &QAction::triggered, this, &MainWindow::onToolRazor);
    toolGroup_->addAction(actToolRazor_);

    actToolSpacer_ = new QAction(QIcon(":/icons/merge.svg"), tr("Spacer Tool"), this);
    actToolSpacer_->setShortcuts({QKeySequence("M"), QKeySequence("T")});
    actToolSpacer_->setCheckable(true);
    connect(actToolSpacer_, &QAction::triggered, this, &MainWindow::onToolSpacer);
    toolGroup_->addAction(actToolSpacer_);

    actToolHand_ = new QAction(tr("Hand Tool"), this);
    actToolHand_->setShortcut(QKeySequence("H"));
    actToolHand_->setCheckable(true);
    connect(actToolHand_, &QAction::triggered, this, &MainWindow::onToolHand);
    toolGroup_->addAction(actToolHand_);
}

void MainWindow::setupDockWidgets()
{
    // Project Bin (left)
    dockBin_ = new QDockWidget(tr("Project Bin"), this);
    dockBin_->setObjectName("ProjectBinDock");
    dockBin_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    mediaBrowser_ = new MediaBrowser(dockBin_);
    mediaBrowser_->setProber(prober_.get());
    dockBin_->setWidget(mediaBrowser_);
    addDockWidget(Qt::LeftDockWidgetArea, dockBin_);

    // Monitor (center top — uses central widget area)
    auto* central = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    preview_ = new PreviewWidget(central);
    preview_->setProber(prober_.get());
    centralLayout->addWidget(preview_);
    setCentralWidget(central);

    // Timeline (bottom)
    dockTimeline_ = new QDockWidget(tr("Timeline"), this);
    dockTimeline_->setObjectName("TimelineDock");
    dockTimeline_->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    timeline_ = new TimelineWidget(dockTimeline_, engine_.get(), project_->id());
    timeline_->setProber(prober_.get());
    timeline_->setProject(project_.get());
    project_->setTimeline(timeline_);
    dockTimeline_->setWidget(timeline_);
    addDockWidget(Qt::BottomDockWidgetArea, dockTimeline_);

    // Properties (right)
    dockProps_ = new QDockWidget(tr("Properties"), this);
    dockProps_->setObjectName("PropertiesDock");
    dockProps_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    properties_ = new PropertiesPanel(dockProps_);
    properties_->setEngine(engine_.get());
    properties_->setProjectId(project_->id());
    properties_->setTimeline(timeline_);
    properties_->setProject(project_.get());
    dockProps_->setWidget(properties_);
    addDockWidget(Qt::RightDockWidgetArea, dockProps_);

    // Effects panel (tabbed with Properties, like Kdenlive's effect stack)
    dockEffects_ = new QDockWidget(tr("Effects"), this);
    dockEffects_->setObjectName("EffectsDock");
    dockEffects_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    effectsPanel_ = new EffectsPanel(dockEffects_);
    dockEffects_->setWidget(effectsPanel_);
    addDockWidget(Qt::RightDockWidgetArea, dockEffects_);
    // Tabify Effects + Properties so they share the right dock area
    tabifyDockWidget(dockProps_, dockEffects_);
    dockProps_->raise();

    // Wire signals
    connect(mediaBrowser_, &MediaBrowser::mediaActivated,
            this, [this](const QString& path, const QString& name) {
        properties_->showMediaInfo(path, name);
    });
    connect(mediaBrowser_, &MediaBrowser::mediaActivated,
            preview_, &PreviewWidget::loadMedia);
    connect(timeline_, &TimelineWidget::clipSelected,
            properties_, &PropertiesPanel::showClipInfo);
    connect(timeline_, &TimelineWidget::timelineChanged,
            this, &MainWindow::onTimelineChanged);
    connect(timeline_, &TimelineWidget::playheadMoved,
            this, &MainWindow::onPlayheadMoved);
    connect(timeline_, &TimelineWidget::toolChanged,
            this, [this](Tool::Kind t) {
        switch (t) {
            case Tool::SelectTool:   actToolSelect_->setChecked(true); break;
            case Tool::RazorTool:    actToolRazor_->setChecked(true);  break;
            case Tool::SpacerTool:   actToolSpacer_->setChecked(true); break;
            case Tool::RippleTool:
            case Tool::RollTool:
            case Tool::SlipTool:
            case Tool::SlideTool:
            case Tool::MulticamTool:
                // Reserved tools default to select cursor
                actToolSelect_->setChecked(true); break;
        }
    });

    connect(exporter_.get(), &Exporter::progress,
            this, &MainWindow::onExportProgress);
    connect(exporter_.get(), &Exporter::finished,
            this, &MainWindow::onExportFinished);

    // Hook undo/redo to project's QUndoStack
    actUndo_->setEnabled(project_->undoStack()->canUndo());
    actRedo_->setEnabled(project_->undoStack()->canRedo());
    connect(project_->undoStack(), &QUndoStack::canUndoChanged,
            actUndo_, &QAction::setEnabled);
    connect(project_->undoStack(), &QUndoStack::canRedoChanged,
            actRedo_, &QAction::setEnabled);
}

void MainWindow::setupToolbars()
{
    mainToolbar_ = addToolBar(tr("Main"));
    mainToolbar_->setObjectName("MainToolbar");
    mainToolbar_->setMovable(false);
    mainToolbar_->setIconSize(QSize(20, 20));
    mainToolbar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
    mainToolbar_->addAction(actImport_);
    mainToolbar_->addSeparator();
    mainToolbar_->addAction(actSkipStart_);
    mainToolbar_->addAction(actPlayPause_);
    mainToolbar_->addAction(actStop_);
    mainToolbar_->addAction(actSkipEnd_);
    mainToolbar_->addAction(actPrevCut_);
    mainToolbar_->addAction(actNextCut_);
    mainToolbar_->addSeparator();
    mainToolbar_->addAction(actAddVideoTrack_);
    mainToolbar_->addAction(actAddAudioTrack_);
    mainToolbar_->addAction(actAddImageTrack_);
    mainToolbar_->addSeparator();
    mainToolbar_->addAction(actExport_);

    toolToolbar_ = addToolBar(tr("Tools"));
    toolToolbar_->setObjectName("ToolToolbar");
    toolToolbar_->setMovable(false);
    toolToolbar_->setIconSize(QSize(20, 20));
    toolToolbar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolToolbar_->addAction(actToolSelect_);
    toolToolbar_->addAction(actToolRazor_);
    toolToolbar_->addAction(actToolSpacer_);
    toolToolbar_->addAction(actToolHand_);

    editToolbar_ = addToolBar(tr("Edit"));
    editToolbar_->setObjectName("EditToolbar");
    editToolbar_->setMovable(false);
    editToolbar_->setIconSize(QSize(20, 20));
    editToolbar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
    editToolbar_->addAction(actUndo_);
    editToolbar_->addAction(actRedo_);
    editToolbar_->addSeparator();
    editToolbar_->addAction(actSplit_);
    editToolbar_->addAction(actCut_);
    editToolbar_->addAction(actMerge_);
    editToolbar_->addAction(actDelete_);
    editToolbar_->addSeparator();
    editToolbar_->addAction(actZoomIn_);
    editToolbar_->addAction(actZoomOut_);
}

void MainWindow::setupMenubar()
{
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(actImport_);
    fileMenu->addSeparator();
    fileMenu->addAction(actExport_);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Quit"), this, &QWidget::close, QKeySequence("Ctrl+Q"));

    auto* editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(actUndo_);
    editMenu->addAction(actRedo_);
    editMenu->addSeparator();
    editMenu->addAction(actSplit_);
    editMenu->addAction(actCut_);
    editMenu->addAction(actMerge_);
    editMenu->addAction(actDelete_);

    auto* viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(actZoomIn_);
    viewMenu->addAction(actZoomOut_);
    viewMenu->addSeparator();
    viewMenu->addAction(dockBin_->toggleViewAction());
    viewMenu->addAction(dockProps_->toggleViewAction());
    viewMenu->addAction(dockEffects_->toggleViewAction());
    viewMenu->addAction(dockTimeline_->toggleViewAction());

    auto* clipMenu = menuBar()->addMenu(tr("&Clip"));
    clipMenu->addAction(actSplit_);
    clipMenu->addAction(actCut_);
    clipMenu->addAction(actMerge_);
    clipMenu->addAction(actDelete_);

    auto* sequenceMenu = menuBar()->addMenu(tr("&Sequence"));
    sequenceMenu->addAction(actAddVideoTrack_);
    sequenceMenu->addAction(actAddAudioTrack_);
    sequenceMenu->addAction(actAddImageTrack_);

    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(actAbout_);
}

void MainWindow::setupStatusbar()
{
    statusLabel_ = new QLabel(tr("Engine v%1").arg(engine_->engineVersion()));
    statusBar()->addWidget(statusLabel_, 1);

    timecodeLabel_ = new QLabel("00:00:00:00");
    timecodeLabel_->setStyleSheet(
        "font-family: 'Menlo','Consolas','DejaVu Sans Mono',monospace;"
        "font-size: 13px;"
        "color: #ffaa55;"
        "padding: 2px 8px;"
        "background: #15161a;"
        "border-radius: 3px;");
    statusBar()->addPermanentWidget(timecodeLabel_);

    exportProgress_ = new QProgressBar(statusBar());
    exportProgress_->setRange(0, 100);
    exportProgress_->setValue(0);
    exportProgress_->setMaximumWidth(240);
    exportProgress_->setVisible(false);
    statusBar()->addPermanentWidget(exportProgress_);
}

void MainWindow::setTool(Tool::Kind t)
{
    timeline_->setTool(t);
    switch (t) {
        case Tool::SelectTool:   actToolSelect_->setChecked(true); break;
        case Tool::RazorTool:    actToolRazor_->setChecked(true);  break;
        case Tool::SpacerTool:   actToolSpacer_->setChecked(true); break;
        case Tool::RippleTool:
        case Tool::RollTool:
        case Tool::SlipTool:
        case Tool::SlideTool:
        case Tool::MulticamTool:
            actToolSelect_->setChecked(true); break;
    }
}

void MainWindow::onToolSelect() { setTool(Tool::SelectTool); }
void MainWindow::onToolRazor()  { setTool(Tool::RazorTool);  }
void MainWindow::onToolSpacer() { setTool(Tool::SpacerTool); }
void MainWindow::onToolHand()   { setTool(Tool::SelectTool); }

void MainWindow::onImportMedia()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Import Media"), QString(),
        tr("Media Files (*.mp4 *.mov *.mkv *.avi *.webm *.m4v *.wmv *.flv "
           "*.mp3 *.wav *.aac *.flac *.ogg *.m4a *.opus "
           "*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tiff);;All Files (*)"));
    for (const QString& f : files) {
        mediaBrowser_->addMedia(f);
        project_->importMedia(f);
    }
}

void MainWindow::onPlayPause() { preview_->togglePlayPause(); }
void MainWindow::onStop()      { preview_->stop(); }
void MainWindow::onSkipStart() { timeline_->setPlayheadFrame(0); }
void MainWindow::onSkipEnd()   { timeline_->setPlayheadFrame(UINT64_MAX); }
void MainWindow::onNextCut()   { timeline_->jumpToNextCut(); }
void MainWindow::onPrevCut()   { timeline_->jumpToPrevCut(); }

void MainWindow::onAddVideoTrack()
{
    int n = timeline_->trackCount();
    engine_->addTrack(project_->id(), EngineBridge::Video, tr("Video %1").arg(n + 1));
    timeline_->refreshTracks();
}

void MainWindow::onAddAudioTrack()
{
    int n = timeline_->trackCount();
    engine_->addTrack(project_->id(), EngineBridge::Audio, tr("Audio %1").arg(n + 1));
    timeline_->refreshTracks();
}

void MainWindow::onAddImageTrack()
{
    int n = timeline_->trackCount();
    engine_->addTrack(project_->id(), EngineBridge::Image, tr("Image %1").arg(n + 1));
    timeline_->refreshTracks();
}

void MainWindow::onSplit()
{
    if (timeline_->splitAtPlayhead()) {
        statusBar()->showMessage(tr("Split at playhead."), 2000);
    }
}

void MainWindow::onCut()
{
    if (!timeline_->splitAtPlayhead()) return;
    timeline_->setPlayheadFrame(timeline_->playheadFrame() + 1);
    timeline_->deleteSelectedClip();
    statusBar()->showMessage(tr("Cut at playhead."), 2000);
}

void MainWindow::onMerge()
{
    if (timeline_->mergeSelectedWithNext()) {
        statusBar()->showMessage(tr("Clips merged."), 2000);
    }
}

void MainWindow::onDelete()
{
    if (timeline_->deleteSelectedClip()) {
        statusBar()->showMessage(tr("Clip deleted."), 2000);
        properties_->clearSelection();
    }
}

void MainWindow::onUndo() { project_->undoStack()->undo(); }
void MainWindow::onRedo() { project_->undoStack()->redo(); }
void MainWindow::onZoomIn()  { timeline_->setZoom(timeline_->zoom() + 2); }
void MainWindow::onZoomOut() { timeline_->setZoom(timeline_->zoom() - 2); }

void MainWindow::onTimelineChanged()
{
    auto snap = engine_->snapshot(project_->id());
    int totalClips = 0;
    for (const auto& t : snap.tracks) totalClips += t.clips.size();
    statusLabel_->setText(tr("Engine v%1  •  %2 tracks  •  %3 clips")
                              .arg(engine_->engineVersion())
                              .arg(snap.tracks.size())
                              .arg(totalClips));
}

void MainWindow::onPlayheadMoved(uint64_t frame)
{
    timecodeLabel_->setText(Timecode::fromFrames(frame, 30.0));
}

void MainWindow::onExport()
{
    if (exporter_->isRunning()) {
        QMessageBox::information(this, tr("Export"),
            tr("An export is already in progress."));
        return;
    }

    auto snap = engine_->snapshot(project_->id());
    int totalClips = 0;
    for (const auto& t : snap.tracks) totalClips += t.clips.size();
    if (totalClips == 0) {
        QMessageBox::information(this, tr("Export"),
            tr("The timeline is empty. Add clips before exporting."));
        return;
    }

    ExportDialog dlg(this);
    dlg.setProjectInfo(snap.tracks.size(), totalClips,
                       snap.width, snap.height, snap.fps);
    if (dlg.exec() != QDialog::Accepted) return;

    auto s = dlg.settings();
    if (s.outputPath.isEmpty()) {
        QMessageBox::warning(this, tr("Export"),
            tr("Please choose an output file."));
        return;
    }

    QList<QList<Exporter::ClipItem>> clipsByTrack;
    for (const auto& t : snap.tracks) {
        QList<Exporter::ClipItem> track;
        for (const auto& c : t.clips) {
            QString kind;
            if (t.kind == 0)      kind = "video";
            else if (t.kind == 1) kind = "audio";
            else                   kind = "image";
            Exporter::ClipItem item;
            item.path            = c.mediaPath;
            item.name            = c.mediaName;
            item.kind            = kind;
            item.trimInFrames    = c.trimInFrames;
            item.durationFrames  = c.durationFrames;
            item.fps             = snap.fps > 0 ? snap.fps : 30.0;
            item.width           = c.mediaWidth;
            item.height          = c.mediaHeight;
            item.volume          = c.adjust.volume;
            item.opacity         = c.adjust.opacity;
            item.scale           = c.adjust.scale;
            item.muted           = t.state.muted;
            item.visible         = t.state.visible;
            track.append(item);
        }
        clipsByTrack.append(track);
    }

    exportProgress_->setVisible(true);
    exportProgress_->setValue(0);
    exportProgress_->setFormat(tr("Preparing..."));
    statusBar()->showMessage(tr("Exporting to %1").arg(s.outputPath), 0);
    exporter_->start(clipsByTrack, s, snap.fps > 0 ? snap.fps : s.fps);
}

void MainWindow::onExportProgress(int percent, const QString& stage)
{
    exportProgress_->setValue(percent);
    exportProgress_->setFormat(stage);
}

void MainWindow::onExportFinished(bool success, const QString& msg)
{
    exportProgress_->setValue(success ? 100 : 0);
    if (success) {
        statusBar()->showMessage(tr("Export complete."), 5000);
        QMessageBox::information(this, tr("Export"), msg);
    } else {
        statusBar()->showMessage(tr("Export failed."), 5000);
        QMessageBox::warning(this, tr("Export Failed"), msg);
    }
    QTimer::singleShot(2000, this, [this]() {
        exportProgress_->setVisible(false);
    });
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About Beta"),
        tr("<h3>Beta Video Editor</h3>"
           "<p>A hybrid Rust + C++/Qt6 video editor.</p>"
           "<p><b>Engine:</b> v%1</p>"
           "<p><b>v0.4 architecture:</b><br>"
           "• Undo/Redo via QUndoStack<br>"
           "• Tool-based editing (Select / Razor / Spacer / Hand)<br>"
           "• Dockable panels (Project Bin / Monitor / Timeline / Properties)<br>"
           "• Snapping to playhead, clip edges, ruler marks<br>"
           "• Clip thumbnails for video, waveforms for audio<br>"
           "• Timecode display<br>"
           "• Standard NLE shortcuts (V/C/T/H, J/K/L, I/O, Home/End, PgUp/PgDn, +/-)<br>"
           "• Drag &amp; drop media onto the timeline<br>"
           "• Color &amp; transform adjustments per clip<br>"
           "• FFmpeg-based export</p>"
           "<p>Repository: github.com/salom600/beta</p>")
            .arg(engine_->engineVersion()));
}

} // namespace beta
