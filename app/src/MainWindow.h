#pragma once

#include <QMainWindow>
#include <memory>
#include "Tool.h"

class QDockWidget;
class QToolBar;
class QAction;
class QActionGroup;
class QLabel;
class QProgressBar;

namespace beta {

class EngineBridge;
class MediaBrowser;
class MediaProber;
class PreviewWidget;
class TimelineWidget;
class PropertiesPanel;
class EffectsPanel;
class Exporter;
class ExportDialog;
class Project;

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
    void onSkipStart();
    void onSkipEnd();
    void onNextCut();
    void onPrevCut();
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
    void onPlayheadMoved(uint64_t frame);
    void onToolSelect();
    void onToolRazor();
    void onToolSpacer();
    void onToolHand();
    void onUndo();
    void onRedo();
    void onZoomIn();
    void onZoomOut();

private:
    void setupActions();
    void setupToolbars();
    void setupDockWidgets();
    void setupMenubar();
    void setupStatusbar();
    void applyTheme();
    void openProject();
    void setTool(Tool::Kind t);

    std::unique_ptr<EngineBridge> engine_;
    std::unique_ptr<MediaProber>  prober_;
    std::unique_ptr<Exporter>     exporter_;
    std::unique_ptr<Project>      project_;

    QDockWidget*   dockBin_       = nullptr;
    QDockWidget*   dockMonitor_   = nullptr;
    QDockWidget*   dockTimeline_  = nullptr;
    QDockWidget*   dockProps_     = nullptr;
    QDockWidget*   dockEffects_   = nullptr;
    QToolBar*      mainToolbar_   = nullptr;
    QToolBar*      toolToolbar_   = nullptr;
    QToolBar*      editToolbar_   = nullptr;
    MediaBrowser*  mediaBrowser_  = nullptr;
    PreviewWidget* preview_       = nullptr;
    TimelineWidget* timeline_     = nullptr;
    PropertiesPanel* properties_  = nullptr;
    EffectsPanel*  effectsPanel_  = nullptr;
    QLabel*        statusLabel_   = nullptr;
    QLabel*        timecodeLabel_ = nullptr;
    QProgressBar*  exportProgress_ = nullptr;

    QActionGroup* toolGroup_ = nullptr;
    QAction* actImport_        = nullptr;
    QAction* actPlayPause_     = nullptr;
    QAction* actStop_          = nullptr;
    QAction* actSkipStart_     = nullptr;
    QAction* actSkipEnd_       = nullptr;
    QAction* actNextCut_       = nullptr;
    QAction* actPrevCut_       = nullptr;
    QAction* actAddVideoTrack_ = nullptr;
    QAction* actAddAudioTrack_ = nullptr;
    QAction* actAddImageTrack_ = nullptr;
    QAction* actSplit_         = nullptr;
    QAction* actCut_           = nullptr;
    QAction* actMerge_         = nullptr;
    QAction* actDelete_        = nullptr;
    QAction* actExport_        = nullptr;
    QAction* actAbout_         = nullptr;
    QAction* actUndo_          = nullptr;
    QAction* actRedo_          = nullptr;
    QAction* actZoomIn_        = nullptr;
    QAction* actZoomOut_       = nullptr;
    QAction* actToolSelect_    = nullptr;
    QAction* actToolRazor_     = nullptr;
    QAction* actToolSpacer_    = nullptr;
    QAction* actToolHand_      = nullptr;
};

} // namespace beta
