#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>

class Utils
{
public:
    static QString getExecutablePath();
    static QString getAppDataPath();
    static bool isValidMediaFile(const QString& filePath);
    static QString generateOutputFilename(const QString& inputFile, const QString& extension);
    
    static QStringList getAvailableCodecs();
    static QStringList getAvailableHardwareAccels();
    static QStringList getAvailableFormats();
    static bool isFFmpegAvailable();
    static QString getFFmpegVersion();
    
    static int getCPUCoreCount();
    static QString formatDuration(qint64 milliseconds);
    static QString formatFileSize(qint64 bytes);
    
    static QString escapePath(const QString& path);
    static QString sanitizeFilename(const QString& filename);
    
    static QString getCurrentTimestamp();
    
    static bool isValidResolution(const QString& resolution);
    static bool isValidFramerate(const QString& framerate);
};

namespace FFmpegParser {
    struct ProgressInfo {
        qint64 frame = 0;
        double fps = 0.0;
        double bitrate = 0.0;
        qint64 fileSize = 0;
        qint64 timeMs = 0;
        double speed = 0.0;
        int progressPercent = 0;
    };
    
    ProgressInfo parseProgressLine(const QString& line);
    qint64 parseTimeToMilliseconds(const QString& timeString);
    QString millisecondsToTimeString(qint64 milliseconds);
}

#endif // UTILS_H
