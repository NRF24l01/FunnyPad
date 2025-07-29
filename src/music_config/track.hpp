#pragma once

#include <QString>
#include <QDateTime>

class Track {
public:
    Track();
    Track(const QString& originalPath);
    ~Track();

    // Getters
    QString getTitle() const;
    QString getArtist() const;
    QString getProcessedPath() const;
    QString getOriginalPath() const;
    QDateTime getAddedDate() const;
    int getDuration() const; // in seconds

    // Setters
    void setTitle(const QString& title);
    void setArtist(const QString& artist);
    
    // Process the track using ffmpeg and store it in the app data location
    // Returns true if processing was successful
    bool processTrack();

private:
    QString m_title;
    QString m_artist;
    QString m_originalPath;  // Original file path
    QString m_processedPath; // Path after processing with ffmpeg
    QDateTime m_addedDate;
    int m_duration;          // Track duration in seconds
    
    // Helper methods
    QString generateUniqueFilename() const;
    QString getAppDataPath() const;
};
