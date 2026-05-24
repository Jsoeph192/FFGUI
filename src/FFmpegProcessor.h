#ifndef FFMPEGPROCESSOR_H
#define FFMPEGPROCESSOR_H

#include <QObject>
#include <QProcess>
#include <QTimer>

class FFmpegProcessor : public QObject
{
    Q_OBJECT

public:
    explicit FFmpegProcessor(QObject *parent = nullptr);
    ~FFmpegProcessor();
    
    void start(const QString& command);
    void stop();
    bool isRunning() const;
    QByteArray readAllStandardOutput();
    QByteArray readAllStandardError();

signals:
    void progressUpdated(qint64 bytesProcessed, qint64 bytesTotal);
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void errorOccurred(QProcess::ProcessError error);
    void outputReceived();

private slots:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onTimeout();

private:
    QProcess* m_process;
    QTimer* m_timer;
    QString m_command;
    qint64 m_totalBytes;
    qint64 m_processedBytes;
};

#endif // FFMPEGPROCESSOR_H
