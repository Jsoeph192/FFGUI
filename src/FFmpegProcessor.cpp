#include "FFmpegProcessor.h"
#include <QRegularExpression>
#include <QDebug>

FFmpegProcessor::FFmpegProcessor(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_timer(new QTimer(this))
    , m_totalBytes(0)
    , m_processedBytes(0)
{
    connect(m_process, &QProcess::started, this, &FFmpegProcessor::onProcessStarted);
    connect(m_process, &QProcess::finished, this, &FFmpegProcessor::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &FFmpegProcessor::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &FFmpegProcessor::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &FFmpegProcessor::onReadyReadStandardError);
    
    connect(m_timer, &QTimer::timeout, this, &FFmpegProcessor::onTimeout);
    m_timer->setInterval(1000); // Update every second
}

FFmpegProcessor::~FFmpegProcessor()
{
    if (m_process->state() == QProcess::Running) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
        }
    }
}

void FFmpegProcessor::start(const QString& command)
{
    QStringList parts = command.split(" ", Qt::SkipEmptyParts);
    if (parts.isEmpty()) return;
    
    QString program = parts.takeFirst(); // "ffmpeg"
    QStringList arguments;
    
    QString currentArg;
    bool inQuotes = false;
    
    for (int i = 0; i < parts.size(); ++i) {
        QString part = parts[i];
        
        if (!inQuotes && part.startsWith("\"")) {
            inQuotes = true;
            currentArg = part.mid(1);
            if (part.endsWith("\"") && part.length() > 1) {
                currentArg = currentArg.left(currentArg.length() - 1);
                arguments << currentArg;
                currentArg = "";
                inQuotes = false;
            }
        } else if (inQuotes && part.endsWith("\"")) {
            currentArg += " " + part.left(part.length() - 1);
            arguments << currentArg;
            currentArg = "";
            inQuotes = false;
        } else if (inQuotes) {
            currentArg += " " + part;
        } else {
            arguments << part;
        }
    }
    
    m_process->start(program, arguments);
}


void FFmpegProcessor::stop()
{
    m_timer->stop();
    if (m_process->state() == QProcess::Running) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
        }
    }
}

bool FFmpegProcessor::isRunning() const
{
    return m_process->state() == QProcess::Running;
}

QByteArray FFmpegProcessor::readAllStandardOutput()
{
    return m_process->readAllStandardOutput();
}

QByteArray FFmpegProcessor::readAllStandardError()
{
    return m_process->readAllStandardError();
}

void FFmpegProcessor::onProcessStarted()
{
    emit progressUpdated(0, 100);
}

void FFmpegProcessor::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_timer->stop();
    emit finished(exitCode, exitStatus);
}

void FFmpegProcessor::onProcessError(QProcess::ProcessError error)
{
    m_timer->stop();
    emit errorOccurred(error);
}

void FFmpegProcessor::onReadyReadStandardOutput()
{
    emit outputReceived();
}

void FFmpegProcessor::onReadyReadStandardError()
{
    QByteArray data = m_process->readAllStandardError();
    QString output = QString::fromLocal8Bit(data);
    
    QRegularExpression re("time=(\\d{2}):(\\d{2}):(\\d{2}\\.\\d{2})");
    QRegularExpressionMatch match = re.match(output);
    
    if (match.hasMatch()) {
        int hours = match.captured(1).toInt();
        int minutes = match.captured(2).toInt();
        double seconds = match.captured(3).toDouble();
        double totalTime = hours * 3600 + minutes * 60 + seconds;
        
        emit progressUpdated(static_cast<qint64>(totalTime * 1000), 100000);
    }
    
    emit outputReceived();
}

void FFmpegProcessor::onTimeout()
{
    emit progressUpdated(m_processedBytes, m_totalBytes);
}
