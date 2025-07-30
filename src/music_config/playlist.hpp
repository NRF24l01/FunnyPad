#pragma once

#include "track.hpp"
#include <QString>
#include <QList>
#include <QDateTime>
#include <memory>

class Playlist {
public:
    Playlist();
    Playlist(const QString& name);
    ~Playlist();

    // Getters
    QString getName() const;
    QDateTime getCreatedDate() const;
    QList<std::shared_ptr<Track>> getTracks() const;
    std::shared_ptr<Track> getTrack(int index) const;
    int getTrackCount() const;

    // Setters
    void setName(const QString& name);
    void setCreatedDate(const QDateTime& date);

    // Track management
    bool addTrack(const QString& trackPath);
    bool addTrack(std::shared_ptr<Track> track);
    bool addTrackWithoutProcessing(std::shared_ptr<Track> track);
    bool removeTrack(int index);
    void clear();

    // Playlist operations
    bool moveTrackUp(int index);
    bool moveTrackDown(int index);

private:
    QString m_name;
    QDateTime m_createdDate;
    QList<std::shared_ptr<Track>> m_tracks;
};
