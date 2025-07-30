#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QComboBox>
#include "SoundpadAudio.hpp"
#include "../music_config/PlaylistManager.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void on_playbackButton_clicked();
    void on_musicProgress_sliderMoved(int value);
    void on_playbackStarted(qint64 totalMs);
    void on_playbackProgress(qint64 currentMs);
    void on_playbackStopped();
    void on_previousButton_clicked();
    void on_nextButton_clicked();
    void on_playlistList_currentRowChanged(int currentRow);
    void on_soundTable_cellDoubleClicked(int row, int column);
    void on_addPlaylistButton_clicked();
    void on_importButton_clicked();

private:
    Ui::MainWindow *ui;
    soundpad::SoundpadAudio audio;
    bool isPlaying = false;

    QSettings* settings;
    QString data_path;
    
    // Playlist management
    PlaylistManager playlistManager;
    int currentPlaylistIndex = -1;
    int currentTrackIndex = -1;

    // Helper methods
    void updatePlaylistsList();
    void updateTracksList();
    void loadPlaylistsFromSettings();
    void savePlaylistsToSettings();
    void playTrack(int trackIndex);
    void processAudioFile(const QString& filePath);
};

#endif // MAINWINDOW_H