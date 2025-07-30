#include "main_window.hpp"
#include "ui_main.h"
#include <QMessageBox>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , audio() // initialize audio here
{
    ui->setupUi(this);

    // Set up drag & drop
    setAcceptDrops(true);
    ui->soundTable->setAcceptDrops(true);
    ui->soundTable->viewport()->setAcceptDrops(true);
    ui->soundTable->setDragDropMode(QAbstractItemView::DropOnly);

    settings = new QSettings("nrf24l01", "FunnyPad");
    qDebug() << "Settings path:" << settings->fileName();

    // Setup mic input selector
    auto sources = audio.getSourceList();

    ui->outputSelect->clear();
    ui->outputSelect->addItem("Select Audio Source", "");

    for (const auto& source : sources) {
        ui->outputSelect->addItem(QString::fromStdString(source.second), QString::fromStdString(source.first));
    }

    connect(ui->outputSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        QString sourceName = ui->outputSelect->itemData(index).toString();
        if (sourceName == "") {
            return;
        }
        if (ui->outputSelect->count() > 0 && ui->outputSelect->itemData(0).toString() == "") {
            ui->outputSelect->removeItem(0);
        }
        qDebug() << "Selected audio source:" << sourceName;
        if (!audio.mergeWithMic(sourceName.toStdString())) {
            QMessageBox::warning(this, "Error", "Failed to merge with selected audio source.");
        }
        settings->setValue("audio_source", sourceName);
    });

    QString savedSource = settings->value("audio_source", "").toString();
    if (!savedSource.isEmpty()) {
        int index = ui->outputSelect->findData(savedSource);
        if (index != -1) {
            ui->outputSelect->setCurrentIndex(index);
        }
    }
    
    // Setup output device selector
    ui->audioOutputSelect->clear();
    ui->audioOutputSelect->addItem("Default Output", "");
    
    auto sinks = audio.getSinkList();
    qDebug() << "Found" << sinks.size() << "audio output devices";
    for (const auto& sink : sinks) {
        qDebug() << "Adding sink to UI:" << QString::fromStdString(sink.first) << "-" << QString::fromStdString(sink.second);
        ui->audioOutputSelect->addItem(QString::fromStdString(sink.second), QString::fromStdString(sink.first));
    }
    
    connect(ui->audioOutputSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        QString sinkName = ui->audioOutputSelect->itemData(index).toString();
        qDebug() << "Selected output sink:" << sinkName;
        audio.setOutputSink(sinkName.toStdString());
        settings->setValue("audio_output_sink", sinkName);
    });
    
    // Set the output label to clarify that it's for headphones
    ui->outputLabel->setText(tr("Headphones Output:"));
    
    QString savedSink = settings->value("audio_output_sink", "").toString();
    if (!savedSink.isEmpty()) {
        int index = ui->audioOutputSelect->findData(savedSink);
        if (index != -1) {
            ui->audioOutputSelect->setCurrentIndex(index);
        }
    }

    connect(&audio, &soundpad::SoundpadAudio::playbackStarted, this, &MainWindow::on_playbackStarted);
    connect(&audio, &soundpad::SoundpadAudio::playbackProgress, this, &MainWindow::on_playbackProgress);
    connect(&audio, &soundpad::SoundpadAudio::playbackStopped, this, &MainWindow::on_playbackStopped);

    connect(ui->musicProgress, &QSlider::sliderMoved, this, &MainWindow::on_musicProgress_sliderMoved);

    data_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(data_path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    qDebug() << "Data path:" << data_path;

    // Set up soundTable
    ui->soundTable->setColumnCount(3);
    ui->soundTable->setHorizontalHeaderLabels(QStringList() << "ID" << "Title" << "Duration");
    ui->soundTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    
    // Clear playlistList
    ui->playlistList->clear();
    
    // Load playlists from settings
    loadPlaylistsFromSettings();
    
    // Update UI
    updatePlaylistsList();
    
    // If we have playlists, select the first one
    if (ui->playlistList->count() > 0) {
        ui->playlistList->setCurrentRow(0);
    }
}

MainWindow::~MainWindow()
{
    audio.stop();
    
    // Save playlists to settings
    savePlaylistsToSettings();
    
    delete ui;
    delete settings;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    
    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        
        // Process each URL (file)
        for (const QUrl& url : urlList) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QFileInfo fileInfo(filePath);
                
                // Check if it's an audio file (simple check by extension)
                QString extension = fileInfo.suffix().toLower();
                if (extension == "mp3" || extension == "wav" || extension == "ogg" || extension == "flac" || extension == "m4a") {
                    processAudioFile(filePath);
                }
            }
        }
        
        event->acceptProposedAction();
    }
}

void MainWindow::on_playbackButton_clicked()
{
    if (!isPlaying) {
        // If we have a selected track, play it
        if (currentPlaylistIndex >= 0 && currentTrackIndex >= 0) {
            playTrack(currentTrackIndex);
        }
    } else {
        audio.stop();
        ui->playbackButton->setIcon(QIcon(":/icons/resources/icons/play.png"));
        isPlaying = false;
    }
}

void MainWindow::on_musicProgress_sliderMoved(int value)
{
    // value is in ms
    audio.seek(value);
}

void MainWindow::on_playbackStarted(qint64 totalMs)
{
    ui->musicProgress->setMaximum(totalMs);
    ui->musicProgress->setValue(0);
    ui->currentMusicTime->setText("00:00");
    int sec = totalMs / 1000;
    ui->totalMusicTime->setText(QString("%1:%2").arg(sec/60,2,10,QChar('0')).arg(sec%60,2,10,QChar('0')));
    ui->playbackButton->setIcon(QIcon(":/icons/resources/icons/stop.png"));
    isPlaying = true;
}

void MainWindow::on_playbackProgress(qint64 currentMs)
{
    ui->musicProgress->setValue(currentMs);
    int sec = currentMs / 1000;
    ui->currentMusicTime->setText(QString("%1:%2").arg(sec/60,2,10,QChar('0')).arg(sec%60,2,10,QChar('0')));
}

void MainWindow::on_playbackStopped()
{
    ui->playbackButton->setIcon(QIcon(":/icons/resources/icons/play.png"));
    isPlaying = false;
    
    // Auto-play next track
    if (currentPlaylistIndex >= 0 && currentTrackIndex >= 0) {
        auto playlist = playlistManager.getPlaylist(currentPlaylistIndex);
        if (playlist && currentTrackIndex < playlist->getTrackCount() - 1) {
            playTrack(currentTrackIndex + 1);
        }
    }
}

void MainWindow::on_previousButton_clicked()
{
    if (currentPlaylistIndex >= 0 && currentTrackIndex > 0) {
        playTrack(currentTrackIndex - 1);
    }
}

void MainWindow::on_nextButton_clicked()
{
    if (currentPlaylistIndex >= 0) {
        auto playlist = playlistManager.getPlaylist(currentPlaylistIndex);
        if (playlist && currentTrackIndex < playlist->getTrackCount() - 1) {
            playTrack(currentTrackIndex + 1);
        }
    }
}

void MainWindow::on_playlistList_currentRowChanged(int currentRow)
{
    // Don't stop playback when changing playlists
    currentPlaylistIndex = currentRow;
    currentTrackIndex = -1; // Reset track index but don't stop current playback
    updateTracksList();
}

void MainWindow::on_soundTable_cellDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    
    if (currentPlaylistIndex >= 0 && row >= 0) {
        playTrack(row);
    }
}

void MainWindow::on_addPlaylistButton_clicked()
{
    bool ok;
    QString playlistName = QInputDialog::getText(this, 
        tr("New Playlist"), 
        tr("Enter playlist name:"), 
        QLineEdit::Normal, 
        tr("New Playlist"), 
        &ok);
    
    if (ok && !playlistName.isEmpty()) {
        // Create new playlist
        auto playlist = playlistManager.createPlaylist(playlistName);
        
        // Update UI
        updatePlaylistsList();
        
        // Select the new playlist
        ui->playlistList->setCurrentRow(playlistManager.getPlaylistCount() - 1);
        
        // Save playlists to settings
        savePlaylistsToSettings();
    }
}

void MainWindow::on_importButton_clicked()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        tr("Import Audio Files"),
        QDir::homePath(),
        tr("Audio Files (*.mp3 *.wav *.ogg *.flac *.m4a)")
    );
    
    for (const QString& filePath : filePaths) {
        processAudioFile(filePath);
    }
}

void MainWindow::updatePlaylistsList()
{
    ui->playlistList->clear();
    
    for (int i = 0; i < playlistManager.getPlaylistCount(); i++) {
        auto playlist = playlistManager.getPlaylist(i);
        ui->playlistList->addItem(playlist->getName());
    }
    
    // If there are no playlists, create a default one
    if (playlistManager.getPlaylistCount() == 0) {
        auto playlist = playlistManager.createPlaylist("Default Playlist");
        ui->playlistList->addItem(playlist->getName());
    }
}

void MainWindow::updateTracksList()
{
    ui->soundTable->setRowCount(0);
    
    if (currentPlaylistIndex >= 0) {
        auto playlist = playlistManager.getPlaylist(currentPlaylistIndex);
        if (playlist) {
            ui->soundTable->setRowCount(playlist->getTrackCount());
            
            for (int i = 0; i < playlist->getTrackCount(); i++) {
                auto track = playlist->getTrack(i);
                
                // ID
                QTableWidgetItem* idItem = new QTableWidgetItem(QString::number(i + 1));
                ui->soundTable->setItem(i, 0, idItem);
                
                // Title
                QTableWidgetItem* titleItem = new QTableWidgetItem(track->getTitle());
                ui->soundTable->setItem(i, 1, titleItem);
                
                // Duration
                int sec = track->getDuration();
                QString duration = QString("%1:%2").arg(sec/60,2,10,QChar('0')).arg(sec%60,2,10,QChar('0'));
                QTableWidgetItem* durationItem = new QTableWidgetItem(duration);
                ui->soundTable->setItem(i, 2, durationItem);
                
                // Highlight the current track
                if (i == currentTrackIndex) {
                    idItem->setBackground(QColor(200, 230, 255));
                    titleItem->setBackground(QColor(200, 230, 255));
                    durationItem->setBackground(QColor(200, 230, 255));
                }
            }
        }
    }
}

void MainWindow::loadPlaylistsFromSettings()
{
    QString playlistsPath = data_path + "/playlists.json";
    QFile file(playlistsPath);
    
    if (file.exists()) {
        playlistManager.loadPlaylists(playlistsPath);
    } else {
        // Create a default playlist if none exists
        playlistManager.createPlaylist("Default Playlist");
    }
}

void MainWindow::savePlaylistsToSettings()
{
    QString playlistsPath = data_path + "/playlists.json";
    playlistManager.savePlaylists(playlistsPath);
}

void MainWindow::playTrack(int trackIndex)
{
    // Always stop any currently playing audio, regardless of isPlaying flag
    audio.stop();
    
    if (currentPlaylistIndex >= 0 && trackIndex >= 0) {
        auto playlist = playlistManager.getPlaylist(currentPlaylistIndex);
        if (playlist && trackIndex < playlist->getTrackCount()) {
            auto track = playlist->getTrack(trackIndex);
            
            QString filePath = track->getProcessedPath();
            if (QFile::exists(filePath)) {
                if (audio.playWav(filePath.toStdString())) {
                    currentTrackIndex = trackIndex;
                    updateTracksList(); // Update to highlight the current track
                    // Note: isPlaying will be set to true by the playbackStarted signal
                } else {
                    QMessageBox::warning(this, tr("Error"), tr("Failed to play file:\n") + filePath);
                }
            } else {
                // If processed file doesn't exist, try to process it again
                if (track->processTrack()) {
                    filePath = track->getProcessedPath();
                    if (audio.playWav(filePath.toStdString())) {
                        currentTrackIndex = trackIndex;
                        updateTracksList();
                    } else {
                        QMessageBox::warning(this, tr("Error"), tr("Failed to play file:\n") + filePath);
                    }
                } else {
                    QMessageBox::warning(this, tr("Error"), tr("File not found and processing failed:\n") + filePath);
                }
            }
        }
    }
}

void MainWindow::processAudioFile(const QString& filePath)
{
    if (currentPlaylistIndex < 0) {
        // Create a default playlist if none exists
        if (playlistManager.getPlaylistCount() == 0) {
            auto playlist = playlistManager.createPlaylist("Default Playlist");
            ui->playlistList->addItem(playlist->getName());
            currentPlaylistIndex = 0;
        } else {
            ui->playlistList->setCurrentRow(0);
        }
    }
    
    auto playlist = playlistManager.getPlaylist(currentPlaylistIndex);
    if (playlist) {
        // Create a track and add it to the playlist
        if (playlist->addTrack(filePath)) {
            updateTracksList();
            
            // Select the newly added track
            int newTrackIndex = playlist->getTrackCount() - 1;
            ui->soundTable->selectRow(newTrackIndex);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to add track:\n") + filePath);
        }
    }
}