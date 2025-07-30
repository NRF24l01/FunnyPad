#include "playlist.hpp"
#include <QDebug>

Playlist::Playlist() {
    m_createdDate = QDateTime::currentDateTime();
}

Playlist::Playlist(const QString& name) : m_name(name) {
    m_createdDate = QDateTime::currentDateTime();
}

Playlist::~Playlist() {
    // Shared pointers will be automatically cleaned up
}

QString Playlist::getName() const {
    return m_name;
}

QDateTime Playlist::getCreatedDate() const {
    return m_createdDate;
}

QList<std::shared_ptr<Track>> Playlist::getTracks() const {
    return m_tracks;
}

std::shared_ptr<Track> Playlist::getTrack(int index) const {
    if (index >= 0 && index < m_tracks.size()) {
        return m_tracks[index];
    }
    return nullptr;
}

int Playlist::getTrackCount() const {
    return m_tracks.size();
}

void Playlist::setName(const QString& name) {
    m_name = name;
}

void Playlist::setCreatedDate(const QDateTime& date) {
    m_createdDate = date;
}

bool Playlist::addTrack(const QString& trackPath) {
    auto track = std::make_shared<Track>(trackPath);
    return addTrack(track);
}

bool Playlist::addTrack(std::shared_ptr<Track> track) {
    if (!track) {
        qWarning() << "Cannot add null track to playlist";
        return false;
    }
    
    // Process the track before adding it
    if (!track->processTrack()) {
        qWarning() << "Failed to process track:" << track->getOriginalPath();
        return false;
    }
    
    m_tracks.append(track);
    return true;
}

bool Playlist::addTrackWithoutProcessing(std::shared_ptr<Track> track) {
    if (!track) {
        qWarning() << "Cannot add null track to playlist";
        return false;
    }
    
    // Add without processing
    m_tracks.append(track);
    return true;
}

bool Playlist::removeTrack(int index) {
    if (index >= 0 && index < m_tracks.size()) {
        m_tracks.removeAt(index);
        return true;
    }
    return false;
}

void Playlist::clear() {
    m_tracks.clear();
}

bool Playlist::moveTrackUp(int index) {
    if (index > 0 && index < m_tracks.size()) {
        m_tracks.swapItemsAt(index, index - 1);
        return true;
    }
    return false;
}

bool Playlist::moveTrackDown(int index) {
    if (index >= 0 && index < m_tracks.size() - 1) {
        m_tracks.swapItemsAt(index, index + 1);
        return true;
    }
    return false;
}
