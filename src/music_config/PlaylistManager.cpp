#include "PlaylistManager.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

PlaylistManager::PlaylistManager() {
    // Initialize with an empty list of playlists
}

PlaylistManager::~PlaylistManager() {
    // Shared pointers will be automatically cleaned up
}

std::shared_ptr<Playlist> PlaylistManager::createPlaylist(const QString& name) {
    auto playlist = std::make_shared<Playlist>(name);
    m_playlists.append(playlist);
    return playlist;
}

bool PlaylistManager::removePlaylist(int index) {
    if (index >= 0 && index < m_playlists.size()) {
        m_playlists.removeAt(index);
        return true;
    }
    return false;
}

bool PlaylistManager::renamePlaylist(int index, const QString& newName) {
    if (index >= 0 && index < m_playlists.size()) {
        m_playlists[index]->setName(newName);
        return true;
    }
    return false;
}

QList<std::shared_ptr<Playlist>> PlaylistManager::getPlaylists() const {
    return m_playlists;
}

std::shared_ptr<Playlist> PlaylistManager::getPlaylist(int index) const {
    if (index >= 0 && index < m_playlists.size()) {
        return m_playlists[index];
    }
    return nullptr;
}

int PlaylistManager::getPlaylistCount() const {
    return m_playlists.size();
}

bool PlaylistManager::savePlaylists(const QString& filePath) {
    QJsonArray playlistsArray;
    
    // Serialize each playlist
    for (const auto& playlist : m_playlists) {
        QJsonObject playlistObj;
        playlistObj["name"] = playlist->getName();
        playlistObj["created"] = playlist->getCreatedDate().toString(Qt::ISODate);
        
        QJsonArray tracksArray;
        for (const auto& track : playlist->getTracks()) {
            QJsonObject trackObj;
            trackObj["title"] = track->getTitle();
            trackObj["artist"] = track->getArtist();
            trackObj["originalPath"] = track->getOriginalPath();
            trackObj["processedPath"] = track->getProcessedPath();
            trackObj["addedDate"] = track->getAddedDate().toString(Qt::ISODate);
            trackObj["duration"] = track->getDuration();
            
            tracksArray.append(trackObj);
        }
        
        playlistObj["tracks"] = tracksArray;
        playlistsArray.append(playlistObj);
    }
    
    QJsonDocument doc(playlistsArray);
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }
    
    file.write(doc.toJson());
    return true;
}

bool PlaylistManager::loadPlaylists(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for reading:" << filePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isArray()) {
        qWarning() << "Invalid JSON format in file:" << filePath;
        return false;
    }
    
    // Clear existing playlists
    m_playlists.clear();
    
    QJsonArray playlistsArray = doc.array();
    for (const auto& playlistValue : playlistsArray) {
        QJsonObject playlistObj = playlistValue.toObject();
        
        QString name = playlistObj["name"].toString();
        QDateTime createdDate = QDateTime::fromString(playlistObj["created"].toString(), Qt::ISODate);
        auto playlist = std::make_shared<Playlist>(name);
        
        // Load tracks
        QJsonArray tracksArray = playlistObj["tracks"].toArray();
        for (const auto& trackValue : tracksArray) {
            QJsonObject trackObj = trackValue.toObject();
            
            // Create a track with original path
            auto track = std::make_shared<Track>(trackObj["originalPath"].toString());
            
            // Set properties from saved data
            track->setTitle(trackObj["title"].toString());
            track->setArtist(trackObj["artist"].toString());
            
            // Important: Set the processed path to avoid reprocessing
            if (trackObj.contains("processedPath")) {
                track->setProcessedPath(trackObj["processedPath"].toString());
            }
            
            if (trackObj.contains("duration")) {
                track->setDuration(trackObj["duration"].toInt());
            }
            
            if (trackObj.contains("addedDate")) {
                track->setAddedDate(QDateTime::fromString(trackObj["addedDate"].toString(), Qt::ISODate));
            }
            
            // Add track to playlist without reprocessing
            playlist->addTrackWithoutProcessing(track);
        }
        
        m_playlists.append(playlist);
    }
    
    return true;
}
