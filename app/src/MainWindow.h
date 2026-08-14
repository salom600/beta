#pragma once

#include <QMainWindow>
#include <memory>

class QSplitter;
class QToolBar;
class QAction;
class QLabel;
class QProgressBar;
class QDialog;
class QPlainTextEdit;

namespace beta {

class EngineBridge;
class MediaBrowser;
class MediaProber;
class PreviewWidget;
class TimelineWidget;
class PropertiesPanel;
class Exporter;
class ExportDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;

private slots:
    void onImportMedia();
    void onPlayPause();
    void onStop();
    void onAddVideoTrack();
    void onAddAudioTrack();
    void onAddImageTrack();
    void onSplit();
    void onCut();
    void onMerge();
    void onDelete();
    void onExport();
    void onAbout();
    void onExportProgress(int percent, const QString& stage);
    void onExportFinished(bool success, const QString& msg);
    void onTimelineChanged();

private:
    void setupActions();
    void setupToolbar();
    void setupCentral();
    void setupStatusbar();
    void setupMenubar();
    void applyTheme();
    void openProject();

    std::unique_ptr<EngineBridge> engine_;
    std::unique_ptr<MediaProber>  prober_;
    std::unique_ptr<Exporter>     exporter_;
    uint64_t projectId_ = 0;

    QSplitter*    mainSplitter_  = nullptr;
    QSplitter*    centerSplitter_ = nullptr;
    QToolBar*     toolbar_       = nullptr;
    QToolBar*     editToolbar_   = nullptr;
    MediaBrowser* mediaBrowser_  = nullptr;
    PreviewWidget* preview_      = nullptr;
    TimelineWidget* timeline_    = nullptr;
    PropertiesPanel* properties_ = nullptr;
    QLabel*       statusLabel_   = nullptr;
    QProgressBar* exportProgress_ = nullptr;

    QAction* actImport_        = nullptr;
    QAction* actPlayPause_     = nullptr;
    QAction* actStop_          = nullptr;
    QAction* actAddVideoTrack_ = nullptr;
    QAction* actAddAudioTrack_ = nullptr;
    QAction* actAddImageTrack_ = nullptr;
    QAction* actSplit_         = nullptr;
    QAction* actCut_           = nullptr;
    QAction* actMerge_         = nullptr;
    QAction* actDelete_        = nullptr;
    QAction* actExport_        = nullptr;
    QAction* actAbout_         = nullptr;
};

} // namespace beta
