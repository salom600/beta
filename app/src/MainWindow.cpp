#include "MainWindow.h"
#include "EngineBridge.h"
#include "MediaBrowser.h"
#include "PreviewWidget.h"
#include "TimelineWidget.h"
#include "PropertiesPanel.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>

namespace beta {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Beta — Video Editor"));
    resize(1440, 900);

    engine_ = std::make_unique<EngineBridge>();
    openProject();

    setupActions();
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
    // Seed with a sensible default layout: V1, A1
    engine_->addTrack(projectId_, EngineBridge::Video, tr("Video 1"));
    engine_->addTrack(projectId_, EngineBridge::Audio, tr("Audio 1"));
}

void MainWindow::setupActions()
{
    const QIcon importIcon = style()->standardIcon(QStyle::SP_DialogOpenButton);
    actImport_ = new QAction(importIcon, tr("&Import Media..."), this);
    actImport_->setShortcut(QKeySequence("Ctrl+I"));
    connect(actImport_, &QAction::triggered, this, &MainWindow::onImportMedia);

    actPlayPause_ = new QAction(style()->standardIcon(QStyle::SP_MediaPlay), tr("&Play / Pause"), this);
    actPlayPause_->setShortcut(QKeySequence("Space"));
    connect(actPlayPause_, &QAction::triggered, this, &MainWindow::onPlayPause);

    actStop_ = new QAction(style()->standardIcon(QStyle::SP_MediaStop), tr("&Stop"), this);
    connect(actStop_, &QAction::triggered, this, &MainWindow::onStop);

    actAddVideoTrack_ = new QAction(tr("Add Video Track"), this);
    connect(actAddVideoTrack_, &QAction::triggered, this, &MainWindow::onAddVideoTrack);

    actAddAudioTrack_ = new QAction(tr("Add Audio Track"), this);
    connect(actAddAudioTrack_, &QAction::triggered, this, &MainWindow::onAddAudioTrack);

    actAddImageTrack_ = new QAction(tr("Add Image Track"), this);
    connect(actAddImageTrack_, &QAction::triggered, this, &MainWindow::onAddImageTrack);

    actAbout_ = new QAction(tr("About Beta..."), this);
    connect(actAbout_, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupToolbar()
{
    toolbar_ = addToolBar(tr("Main"));
    toolbar_->setMovable(false);
    toolbar_->addAction(actImport_);
    toolbar_->addSeparator();
    toolbar_->addAction(actPlayPause_);
    toolbar_->addAction(actStop_);
    toolbar_->addSeparator();
    toolbar_->addAction(actAddVideoTrack_);
    toolbar_->addAction(actAddAudioTrack_);
    toolbar_->addAction(actAddImageTrack_);
}

void MainWindow::setupCentral()
{
    mainSplitter_ = new QSplitter(Qt::Horizontal, this);
    centerSplitter_ = new QSplitter(Qt::Vertical, mainSplitter_);

    mediaBrowser_ = new MediaBrowser(mainSplitter_);
    preview_      = new PreviewWidget(centerSplitter_);
    timeline_     = new TimelineWidget(centerSplitter_, engine_.get(), projectId_);
    properties_   = new PropertiesPanel(mainSplitter_);

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

    setCentralWidget(mainSplitter_);

    // Wire signals
    connect(mediaBrowser_, &MediaBrowser::mediaActivated,
            this, [this](const QString& path, const QString& name) {
        properties_->showMediaInfo(path, name);
    });
    connect(mediaBrowser_, &MediaBrowser::mediaActivated,
            preview_, &PreviewWidget::loadMedia);
    connect(timeline_, &TimelineWidget::clipSelected,
            properties_, &PropertiesPanel::showClipInfo);
}

void MainWindow::setupStatusbar()
{
    statusLabel_ = new QLabel(tr("Engine v%1  •  Project #%2")
                                  .arg(engine_->engineVersion())
                                  .arg(projectId_));
    statusBar()->addWidget(statusLabel_);
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

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About Beta"),
        tr("<h3>Beta Video Editor</h3>"
           "<p>A hybrid Rust + C++/Qt6 video editor.</p>"
           "<p>Engine: v%1</p>"
           "<p>Repository: github.com/salom600/beta</p>")
            .arg(engine_->engineVersion()));
}

} // namespace beta
