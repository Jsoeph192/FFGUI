#include "MediaPlayer.h"
#include <QFileInfo>
#include <QTimer>
#include <QApplication>
#include <QDebug>
#include <QProcess>

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_isPlaying(false)
    , m_isPaused(false)
    , m_volume(100)
    , m_position(0)
    , m_duration(0)
    , m_videoMode(false)
    , m_lastError("")
{
    connect(m_process, &QProcess::finished, this, &MediaPlayer::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &MediaPlayer::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MediaPlayer::onProcessReadyRead);
    connect(m_process, &QProcess::readyReadStandardError, this, &MediaPlayer::onProcessReadyRead);
}

MediaPlayer::~MediaPlayer()
{
    stop();
}

bool MediaPlayer::playFile(const QString& filePath)
{
    qDebug() << "Attempting to play file with ffplay:" << filePath;
    
    if (!QFileInfo::exists(filePath)) {
        QString error = "File does not exist: " + filePath;
        qDebug() << error;
        emit errorOccurred(error);
        return false;
    }
    
    stop();
    
    m_currentFile = filePath;
    m_lastError = "";
    
    QStringList videoExtensions = {"mp4", "mkv", "avi", "mov", "wmv", "flv", "webm", "m4v", "mpeg", "mpg"};
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    m_videoMode = videoExtensions.contains(extension);
    
    QStringList arguments;
    arguments << filePath;
    arguments << "-autoexit";
    arguments << "-window_title" << "Media Player";
    arguments << "-volume" << QString::number(m_volume.load());
    
    if (m_videoMode) {
        qDebug() << "Playing video file, enabling display";
    } else {
        arguments << "-nodisp";
        qDebug() << "Playing audio file, disabling display";
    }
    
    qDebug() << "Starting ffplay with arguments:" << arguments;
    
    m_process->start("ffplay", arguments);
    
    if (m_process->waitForStarted(5000)) {
        m_isPlaying.store(true);
        m_isPaused.store(false);
        emit playbackStarted();
        qDebug() << "ffplay started successfully";
        return true;
    } else {
        QString error = "Failed to start ffplay. Make sure it's installed and in your PATH. Error: " + m_process->errorString();
        qDebug() << error;
        emit errorOccurred(error);
        return false;
    }
}

void MediaPlayer::stop()
{
    if (m_process && (m_process->state() == QProcess::Running || 
                      m_process->state() == QProcess::Starting)) {
        qDebug() << "Stopping ffplay process...";
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
        qDebug() << "ffplay process stopped";
    }
    
    m_isPlaying.store(false);
    m_isPaused.store(false);
    m_position.store(0);
    emit playbackStopped();
}

void MediaPlayer::pause()
{
    if (m_isPlaying.load() && !m_isPaused.load()) {
        qDebug() << "Pausing playback by stopping process";
        stop();
        m_isPaused.store(true);
        emit playbackPaused();
    }
}

void MediaPlayer::resume()
{
    if (m_isPaused.load() && !m_isPlaying.load()) {
        qDebug() << "Resuming playback by restarting process";
        if (!m_currentFile.isEmpty()) {
            playFile(m_currentFile);
            emit playbackResumed();
        }
    }
}

void MediaPlayer::setVolume(int volume)
{
    volume = qBound(0, volume, 100);
    m_volume.store(volume);
    emit volumeChanged(volume);
}

bool MediaPlayer::isPlaying() const
{
    return m_isPlaying.load();
}

bool MediaPlayer::isPaused() const
{
    return m_isPaused.load();
}

void MediaPlayer::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "ffplay process finished with exit code:" << exitCode << "status:" << exitStatus;
    
    if (!m_lastError.isEmpty()) {
        qDebug() << "Last error was:" << m_lastError;
    }
    
    if (m_isPlaying.load()) {
        m_isPlaying.store(false);
        m_isPaused.store(false);
        emit playbackStopped();
        qDebug() << "Emitting playbackStopped signal";
    }
}

void MediaPlayer::onProcessError(QProcess::ProcessError error)
{
    QString errorStr;
    switch(error) {
        case QProcess::FailedToStart:
            errorStr = "Failed to start ffplay process";
            break;
        case QProcess::Crashed:
            errorStr = "ffplay process crashed";
            break;
        case QProcess::Timedout:
            errorStr = "ffplay process timed out";
            break;
        case QProcess::WriteError:
            errorStr = "Write error to ffplay process";
            break;
        case QProcess::ReadError:
            errorStr = "Read error from ffplay process";
            break;
        default:
            errorStr = "Unknown error with ffplay process";
            break;
    }
    
    m_lastError = errorStr;
    qDebug() << "Process error occurred:" << errorStr;
    
    if (m_isPlaying.load()) {
        m_isPlaying.store(false);
        m_isPaused.store(false);
        emit errorOccurred(errorStr);
        emit playbackStopped();
        qDebug() << "Emitting error and playbackStopped signals";
    }
}

void MediaPlayer::onProcessReadyRead()
{
    QByteArray stdoutData = m_process->readAllStandardOutput();
    QByteArray stderrData = m_process->readAllStandardError();
    
    if (!stdoutData.isEmpty()) {
        qDebug() << "ffplay stdout:" << stdoutData.trimmed();
    }
    if (!stderrData.isEmpty()) {
        QString errorStr = QString::fromLocal8Bit(stderrData.trimmed());
        qDebug() << "ffplay stderr:" << errorStr;
        if (errorStr.contains("error", Qt::CaseInsensitive) || 
            errorStr.contains("fail", Qt::CaseInsensitive)) {
            m_lastError = errorStr;
        }
    }
}
