#pragma once

#include "playlist.hpp"
#include <QList>
#include <memory>

class PlaylistManager {
public:
    PlaylistManager();
    ~PlaylistManager();

    // Playlist management
    std::shared_ptr<Playlist> createPlaylist(const QString& name);
    bool removePlaylist(int index);
    bool renamePlaylist(int index, const QString& newName);
    
    // Getters
    QList<std::shared_ptr<Playlist>> getPlaylists() const;
    std::shared_ptr<Playlist> getPlaylist(int index) const;
    int getPlaylistCount() const;
    
    // Serialization
    bool savePlaylists(const QString& filePath);
    bool loadPlaylists(const QString& filePath);
    
private:
    QList<std::shared_ptr<Playlist>> m_playlists;
};
