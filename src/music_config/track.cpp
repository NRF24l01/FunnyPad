#include "track.hpp"
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QUuid>
#include <QDebug>

Track::Track() : m_duration(0) {
    m_addedDate = QDateTime::currentDateTime();
}

Track::Track(const QString& originalPath) : m_originalPath(originalPath), m_duration(0) {
    m_addedDate = QDateTime::currentDateTime();
    
    // Try to extract title from filename if not set
    QFileInfo fileInfo(originalPath);
    m_title = fileInfo.baseName();
}

Track::~Track() {
    // Nothing to clean up
}

QString Track::getTitle() const {
    return m_title;
}

QString Track::getArtist() const {
    return m_artist;
}

QString Track::getProcessedPath() const {
    return m_processedPath;
}

QString Track::getOriginalPath() const {
    return m_originalPath;
}

QDateTime Track::getAddedDate() const {
    return m_addedDate;
}

int Track::getDuration() const {
    return m_duration;
}

void Track::setTitle(const QString& title) {
    m_title = title;
}

void Track::setArtist(const QString& artist) {
    m_artist = artist;
}

bool Track::processTrack() {
    if (m_originalPath.isEmpty()) {
        qWarning() << "Cannot process track: original path is empty";
        return false;
    }

    // Create app data directory if it doesn't exist
    QString appDataPath = getAppDataPath();
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Generate a unique filename for the processed track
    QString outputFilename = generateUniqueFilename();
    m_processedPath = appDataPath + "/" + outputFilename;

    // Prepare ffmpeg command
    QProcess ffmpeg;
    QStringList arguments;
    
    // Input file
    arguments << "-i" << m_originalPath;
    
    // Output format: WAV, 44100Hz, 16-bit stereo
    arguments << "-acodec" << "pcm_s16le"
              << "-ar" << "44100"
              << "-ac" << "2"
              << "-y"  // Overwrite output file if it exists
              << m_processedPath;
    
    // Execute ffmpeg
    ffmpeg.start("ffmpeg", arguments);
    if (!ffmpeg.waitForStarted()) {
        qWarning() << "Failed to start ffmpeg process";
        return false;
    }
    
    if (!ffmpeg.waitForFinished(-1)) {
        qWarning() << "ffmpeg process failed:" << ffmpeg.errorString();
        return false;
    }
    
    if (ffmpeg.exitCode() != 0) {
        qWarning() << "ffmpeg exited with error:" << ffmpeg.readAllStandardError();
        return false;
    }
    
    return true;
}

QString Track::generateUniqueFilename() const {
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return uuid + ".wav";
}

QString Track::getAppDataPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tracks";
}
