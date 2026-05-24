#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QObject>
#include <QString>
#include <atomic>
#include <memory>
#include <QProcess>

class MediaPlayer : public QObject
{
    Q_OBJECT

public:
    explicit MediaPlayer(QObject *parent = nullptr);
    ~MediaPlayer();
    
    bool playFile(const QString& filePath);
    void stop();
    void pause();
    void resume();
    void setVolume(int volume);
    bool isPlaying() const;
    bool isPaused() const;
    
signals:
    void playbackStarted();
    void playbackStopped();
    void playbackPaused();
    void playbackResumed();
    void errorOccurred(const QString& error);
    void positionChanged(int position);
    void durationChanged(int duration);
    void volumeChanged(int volume);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onProcessReadyRead();

private:
    QProcess* m_process;
    QString m_currentFile;
    std::atomic<bool> m_isPlaying;
    std::atomic<bool> m_isPaused;
    std::atomic<int> m_volume;
    std::atomic<int> m_position;
    std::atomic<int> m_duration;
    bool m_videoMode;
    QString m_lastError;
};

#endif // MEDIAPLAYER_H
