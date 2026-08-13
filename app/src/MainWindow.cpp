#include "MainWindow.h"
#include "EngineBridge.h"
#include "MediaBrowser.h"
#include "MediaProber.h"
#include "PreviewWidget.h"
#include "TimelineWidget.h"
#include "PropertiesPanel.h"
#include "ExportDialog.h"
#include "Exporter.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QStyleFactory>
#include <QTimer>
#include <QToolBar>
#include <QFile>
#include <QPixmap>
#include <QSettings>

namespace beta {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Beta — Video Editor"));
    resize(1600, 950);
    setMinimumSize(1200, 700);
    setWindowIcon(QIcon(":/icons/app-icon.svg"));

    engine_  = std::make_unique<EngineBridge>();
    prober_  = std::make_unique<MediaProber>();
    exporter_ = std::make_unique<Exporter>();
    openProject();

    applyTheme();
    setupActions();
    setupMenubar();
    setupToolbar();
    setupCentral();
    setupStatusbar();
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* e)
{
    e->accept();
}

void MainWindow::openProject()
{
    projectId_ = engine_->createProject(tr("Untitled Project"));
    engine_->addTrack(projectId_, EngineBridge::Video, tr("Video 1"));
    engine_->addTrack(projectId_, EngineBridge::Audio, tr("Audio 1"));
}

void MainWindow::applyTheme()
{
    if (QStyleFactory::keys().contains("Fusion")) {
        qApp->setStyle(QStyleFactory::create("Fusion"));
    }
    QPalette p = qApp->palette();
    p.setColor(QPalette::Window,          QColor( 30,  31,  34));
    p.setColor(QPalette::WindowText,      QColor(230, 231, 236));
    p.setColor(QPalette::Base,            QColor( 30,  31,  34));
    p.setColor(QPalette::AlternateBase,   QColor( 37,  38,  42));
    p.setColor(QPalette::Text,            QColor(230, 231, 236));
    p.setColor(QPalette::Button,          QColor( 45,  46,  51));
    p.setColor(QPalette::ButtonText,      QColor(230, 231, 236));
    p.setColor(QPalette::BrightText,      Qt::white);
    p.setColor(QPalette::Highlight,       QColor( 14,  99, 212));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase,     QColor( 45,  46,  51));
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
    actPlayPause_->setToolTip(tr("Play / Pause (Space)"));
    connect(actPlayPause_, &QAction::triggered, this, &MainWindow::onPlayPause);

    actStop_ = new QAction(QIcon(":/icons/stop.svg"), tr("&Stop"), this);
    actStop_->setToolTip(tr("Stop"));
    connect(actStop_, &QAction::triggered, this, &MainWindow::onStop);

    actAddVideoTrack_ = new QAction(QIcon(":/icons/video-track.svg"), tr("Add Video Track"), this);
    actAddVideoTrack_->setToolTip(tr("Add a video track"));
    connect(actAddVideoTrack_, &QAction::triggered, this, &MainWindow::onAddVideoTrack);

    actAddAudioTrack_ = new QAction(QIcon(":/icons/audio-track.svg"), tr("Add Audio Track"), this);
    actAddAudioTrack_->setToolTip(tr("Add an audio track"));
    connect(actAddAudioTrack_, &QAction::triggered, this, &MainWindow::onAddAudioTrack);

    actAddImageTrack_ = new QAction(QIcon(":/icons/image-track.svg"), tr("Add Image Track"), this);
    actAddImageTrack_->setToolTip(tr("Add an image track"));
    connect(actAddImageTrack_, &QAction::triggered, this, &MainWindow::onAddImageTrack);

    actExport_ = new QAction(QIcon(":/icons/export.svg"), tr("&Export..."), this);
    actExport_->setShortcut(QKeySequence("Ctrl+E"));
    actExport_->setToolTip(tr("Export project to a video file (Ctrl+E)"));
    connect(actExport_, &QAction::triggered, this, &MainWindow::onExport);

    actAbout_ = new QAction(tr("About Beta..."), this);
    connect(actAbout_, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupMenubar()
{
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(actImport_);
    fileMenu->addSeparator();
    fileMenu->addAction(actExport_);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Quit"), this, &QWidget::close, QKeySequence("Ctrl+Q"));

    auto* trackMenu = menuBar()->addMenu(tr("&Track"));
    trackMenu->addAction(actAddVideoTrack_);
    trackMenu->addAction(actAddAudioTrack_);
    trackMenu->addAction(actAddImageTrack_);

    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(actAbout_);
}

void MainWindow::setupToolbar()
{
    toolbar_ = addToolBar(tr("Main"));
    toolbar_->setObjectName("MainToolbar");
    toolbar_->setMovable(false);
    toolbar_->setIconSize(QSize(20, 20));
    toolbar_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    toolbar_->addAction(actImport_);
    toolbar_->addSeparator();
    toolbar_->addAction(actPlayPause_);
    toolbar_->addAction(actStop_);
    toolbar_->addSeparator();
    toolbar_->addAction(actAddVideoTrack_);
    toolbar_->addAction(actAddAudioTrack_);
    toolbar_->addAction(actAddImageTrack_);
    toolbar_->addSeparator();
    toolbar_->addAction(actExport_);
}

void MainWindow::setupCentral()
{
    mainSplitter_ = new QSplitter(Qt::Horizontal, this);
    mainSplitter_->setHandleWidth(2);
    centerSplitter_ = new QSplitter(Qt::Vertical, mainSplitter_);

    mediaBrowser_ = new MediaBrowser(mainSplitter_);
    mediaBrowser_->setProber(prober_.get());

    preview_ = new PreviewWidget(centerSplitter_);
    preview_->setProber(prober_.get());
    timeline_ = new TimelineWidget(centerSplitter_, engine_.get(), projectId_);
    timeline_->setProber(prober_.get());
    properties_ = new PropertiesPanel(mainSplitter_);
    properties_->setEngine(engine_.get());
    properties_->setProjectId(projectId_);

    centerSplitter_->addWidget(preview_);
    centerSplitter_->addWidget(timeline_);
    centerSplitter_->setStretchFactor(0, 3);
    centerSplitter_->setStretchFactor(1, 2);

    mainSplitter_->addWidget(mediaBrowser_);
    mainSplitter_->addWidget(centerSplitter_);
    mainSplitter_->addWidget(properties_);
    mainSplitter_->setStretchFactor(0, 1);
    mainSplitter_->setStretchFactor(1, 5);
    mainSplitter_->setStretchFactor(2, 1);

    // Restore splitter sizes
    QSettings s;
    if (s.contains("mainSplitter")) {
        mainSplitter_->restoreState(s.value("mainSplitter").toByteArray());
    } else {
        mainSplitter_->setSizes({ 260, 1100, 280 });
    }
    if (s.contains("centerSplitter")) {
        centerSplitter_->restoreState(s.value("centerSplitter").toByteArray());
    } else {
        centerSplitter_->setSizes({ 480, 380 });
    }

    setCentralWidget(mainSplitter_);

    connect(mediaBrowser_, &MediaBrowser::mediaActivated,
            this, [this](const QString& path, const QString& name) {
        properties_->showMediaInfo(path, name);
    });
    connect(mediaBrowser_, &MediaBrowser::mediaActivated,
            preview_, &PreviewWidget::loadMedia);
    connect(timeline_, &TimelineWidget::clipSelected,
            properties_, &PropertiesPanel::showClipInfo);

    connect(exporter_.get(), &Exporter::progress,
            this, &MainWindow::onExportProgress);
    connect(exporter_.get(), &Exporter::finished,
            this, &MainWindow::onExportFinished);
}

void MainWindow::setupStatusbar()
{
    statusLabel_ = new QLabel(tr("Engine v%1  •  Project #%2")
                                  .arg(engine_->engineVersion())
                                  .arg(projectId_));
    statusBar()->addWidget(statusLabel_, 1);

    exportProgress_ = new QProgressBar(statusBar());
    exportProgress_->setRange(0, 100);
    exportProgress_->setValue(0);
    exportProgress_->setMaximumWidth(240);
    exportProgress_->setVisible(false);
    exportProgress_->setTextVisible(true);
    statusBar()->addPermanentWidget(exportProgress_);
}

void MainWindow::onImportMedia()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Import Media"), QString(),
        tr("Media Files (*.mp4 *.mov *.mkv *.avi *.webm *.m4v *.wmv *.flv "
           "*.mp3 *.wav *.aac *.flac *.ogg *.m4a *.opus "
           "*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tiff);;All Files (*)"));
    for (const QString& f : files) {
        mediaBrowser_->addMedia(f);
    }
}

void MainWindow::onPlayPause()
{
    preview_->togglePlayPause();
}

void MainWindow::onStop()
{
    preview_->stop();
}

void MainWindow::onAddVideoTrack()
{
    int n = timeline_->trackCount();
    engine_->addTrack(projectId_, EngineBridge::Video, tr("Video %1").arg(n + 1));
    timeline_->refreshTracks();
}

void MainWindow::onAddAudioTrack()
{
    int n = timeline_->trackCount();
    engine_->addTrack(projectId_, EngineBridge::Audio, tr("Audio %1").arg(n + 1));
    timeline_->refreshTracks();
}

void MainWindow::onAddImageTrack()
{
    int n = timeline_->trackCount();
    engine_->addTrack(projectId_, EngineBridge::Image, tr("Image %1").arg(n + 1));
    timeline_->refreshTracks();
}

void MainWindow::onExport()
{
    if (exporter_->isRunning()) {
        QMessageBox::information(this, tr("Export"),
            tr("An export is already in progress. Please wait for it to finish."));
        return;
    }

    auto snap = engine_->snapshot(projectId_);
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

    // Build clip list for exporter
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
            item.volume          = c.volume;
            item.opacity         = c.opacity;
            item.scale           = c.scale;
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
           "<p><b>Features:</b><br>"
           "• Drag &amp; drop media onto the timeline<br>"
           "• Move / trim clips with the mouse<br>"
           "• Real media probing (duration, dimensions)<br>"
           "• FFmpeg-based export to MP4 / MKV / WebM<br>"
           "• Modern dark UI with custom icons</p>"
           "<p>Repository: github.com/salom600/beta</p>")
            .arg(engine_->engineVersion()));
}

} // namespace beta
