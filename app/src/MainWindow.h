#pragma once

#include <QMainWindow>
#include <QMediaPlayer>
#include <memory>

class QSplitter;
class QToolBar;
class QAction;
class QLabel;

namespace beta {

class EngineBridge;
class MediaBrowser;
class PreviewWidget;
class TimelineWidget;
class PropertiesPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* e) override;

private slots:
    void onImportMedia();
    void onPlayPause();
    void onStop();
    void onAddVideoTrack();
    void onAddAudioTrack();
    void onAddImageTrack();
    void onAbout();

private:
    void setupActions();
    void setupToolbar();
    void setupCentral();
    void setupStatusbar();
    void openProject();

    std::unique_ptr<EngineBridge> engine_;
    uint64_t projectId_ = 0;

    QSplitter*    mainSplitter_  = nullptr;
    QSplitter*    centerSplitter_ = nullptr;
    QToolBar*     toolbar_       = nullptr;
    MediaBrowser* mediaBrowser_  = nullptr;
    PreviewWidget* preview_      = nullptr;
    TimelineWidget* timeline_    = nullptr;
    PropertiesPanel* properties_ = nullptr;
    QLabel*       statusLabel_   = nullptr;

    QAction* actImport_        = nullptr;
    QAction* actPlayPause_     = nullptr;
    QAction* actStop_          = nullptr;
    QAction* actAddVideoTrack_ = nullptr;
    QAction* actAddAudioTrack_ = nullptr;
    QAction* actAddImageTrack_ = nullptr;
    QAction* actAbout_         = nullptr;
};

} // namespace beta
