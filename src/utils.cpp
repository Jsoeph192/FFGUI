#include "utils.h"
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QRegularExpression>
#include <QDateTime>
#include <QMimeDatabase>
#include <QMimeType>
#include <QSysInfo>
#include <QDebug>
#include <QThread>

QString Utils::getExecutablePath()
{
    return QCoreApplication::applicationDirPath();
}

QString Utils::getAppDataPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

bool Utils::isValidMediaFile(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || fileInfo.isDir()) {
        return false;
    }
    
    QMimeDatabase mimeDB;
    QMimeType mimeType = mimeDB.mimeTypeForFile(filePath);
    
    QStringList allowedTypes = {
        "video/mp4", "video/x-matroska", "video/avi", "video/quicktime",
        "video/x-flv", "video/webm", "audio/mpeg", "audio/x-wav",
        "audio/flac", "audio/aac", "audio/mp4"
    };
    
    return mimeType.isValid() && 
           (mimeType.name().startsWith("video/") || mimeType.name().startsWith("audio/")) &&
           allowedTypes.contains(mimeType.name());
}

QString Utils::generateOutputFilename(const QString& inputFile, const QString& extension)
{
    QFileInfo fileInfo(inputFile);
    QString baseName = fileInfo.completeBaseName();
    QString outputPath = fileInfo.dir().absolutePath();
    
    QString outputName = baseName + "_converted." + extension;
    QString outputPathName = outputPath + "/" + outputName;
    
    int counter = 1;
    while (QFile::exists(outputPathName)) {
        outputName = baseName + "_converted_" + QString::number(counter) + "." + extension;
        outputPathName = outputPath + "/" + outputName;
        counter++;
    }
    
    return outputPathName;
}

bool Utils::isFFmpegAvailable()
{
    QProcess process;
    process.start("ffmpeg", QStringList() << "-version");
    
    if (!process.waitForStarted(3000)) {
        return false;
    }
    
    if (!process.waitForFinished(3000)) {
        return false;
    }
    
    return process.exitCode() == 0;
}

QString Utils::getFFmpegVersion()
{
    QProcess process;
    process.start("ffmpeg", QStringList() << "-version");
    
    if (!process.waitForFinished(3000)) {
        return "Unknown";
    }
    
    QString output = process.readAllStandardOutput();
    QRegularExpression re("ffmpeg version ([^\\s]+)");
    QRegularExpressionMatch match = re.match(output);
    
    if (match.hasMatch()) {
        return match.captured(1);
    }
    
    return "Unknown";
}

int Utils::getCPUCoreCount()
{
    return QThread::idealThreadCount();
}

QString Utils::formatDuration(qint64 milliseconds)
{
    int hours = milliseconds / (1000 * 60 * 60);
    int minutes = (milliseconds % (1000 * 60 * 60)) / (1000 * 60);
    int seconds = (milliseconds % (1000 * 60)) / 1000;
    int ms = milliseconds % 1000;
    
    return QString("%1:%2:%3.%4")
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'))
        .arg(ms, 3, 10, QLatin1Char('0'));
}

QString Utils::formatFileSize(qint64 bytes)
{
    QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = bytes;
    
    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        unitIndex++;
    }
    
    return QString("%1 %2").arg(size, 0, 'f', 2).arg(units[unitIndex]);
}

QString Utils::escapePath(const QString& path)
{
    QString escaped = path;
    if (escaped.contains(" ")) {
        escaped = "\"" + escaped + "\"";
    }
    return escaped;
}

QString Utils::sanitizeFilename(const QString& filename)
{
    QString sanitized = filename;
    QStringList invalidChars = {"<", ">", ":", "\"", "/", "\\", "|", "?", "*"};
    
    for (const QString& ch : invalidChars) {
        sanitized.replace(ch, "_");
    }
    
    return sanitized;
}

QString Utils::getCurrentTimestamp()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}

bool Utils::isValidResolution(const QString& resolution)
{
    if (resolution == "Original" || resolution.isEmpty()) {
        return true;
    }
    
    QRegularExpression re("^\\d+x\\d+$");
    return re.match(resolution).hasMatch();
}

bool Utils::isValidFramerate(const QString& framerate)
{
    if (framerate == "Original" || framerate.isEmpty()) {
        return true;
    }
    
    bool ok;
    double rate = framerate.toDouble(&ok);
    return ok && rate > 0;
}

FFmpegParser::ProgressInfo FFmpegParser::parseProgressLine(const QString& line)
{
    ProgressInfo info;
    
    QRegularExpression frameRe("frame=(\\d+)");
    QRegularExpressionMatch frameMatch = frameRe.match(line);
    if (frameMatch.hasMatch()) {
        info.frame = frameMatch.captured(1).toLongLong();
    }
    
    QRegularExpression fpsRe("fps=([\\d.]+)");
    QRegularExpressionMatch fpsMatch = fpsRe.match(line);
    if (fpsMatch.hasMatch()) {
        info.fps = fpsMatch.captured(1).toDouble();
    }
    
    QRegularExpression timeRe("time=(\\d{2}):(\\d{2}):(\\d{2}\\.\\d{2})");
    QRegularExpressionMatch timeMatch = timeRe.match(line);
    if (timeMatch.hasMatch()) {
        int hours = timeMatch.captured(1).toInt();
        int minutes = timeMatch.captured(2).toInt();
        double seconds = timeMatch.captured(3).toDouble();
        info.timeMs = static_cast<qint64>((hours * 3600 + minutes * 60 + seconds) * 1000);
    }
    
    QRegularExpression bitrateRe("bitrate=([\\d.]+)kbits/s");
    QRegularExpressionMatch bitrateMatch = bitrateRe.match(line);
    if (bitrateMatch.hasMatch()) {
        info.bitrate = bitrateMatch.captured(1).toDouble();
    }
    
    QRegularExpression sizeRe("size=(\\d+)kB");
    QRegularExpressionMatch sizeMatch = sizeRe.match(line);
    if (sizeMatch.hasMatch()) {
        info.fileSize = sizeMatch.captured(1).toLongLong() * 1024;
    }
    
    QRegularExpression speedRe("speed=([\\d.]+)x");
    QRegularExpressionMatch speedMatch = speedRe.match(line);
    if (speedMatch.hasMatch()) {
        info.speed = speedMatch.captured(1).toDouble();
    }
    
    return info;
}

qint64 FFmpegParser::parseTimeToMilliseconds(const QString& timeString)
{
    QRegularExpression re("(\\d{2}):(\\d{2}):(\\d{2}\\.\\d{2})");
    QRegularExpressionMatch match = re.match(timeString);
    
    if (match.hasMatch()) {
        int hours = match.captured(1).toInt();
        int minutes = match.captured(2).toInt();
        double seconds = match.captured(3).toDouble();
        return static_cast<qint64>((hours * 3600 + minutes * 60 + seconds) * 1000);
    }
    
    return 0;
}

QString FFmpegParser::millisecondsToTimeString(qint64 milliseconds)
{
    int hours = milliseconds / (1000 * 60 * 60);
    int minutes = (milliseconds % (1000 * 60 * 60)) / (1000 * 60);
    int seconds = (milliseconds % (1000 * 60)) / 1000;
    int ms = milliseconds % 1000;
    
    return QString("%1:%2:%3.%4")
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'))
        .arg(ms, 3, 10, QLatin1Char('0'));
}
