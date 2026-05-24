#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QTableWidgetItem>
#include <memory>
#include "FFmpegProcessor.h"
#include "PresetManager.h"
#include "MediaPlayer.h"

class MediaPlayer;
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionOpen_triggered();
    void on_actionSaveAs_triggered();
    void on_actionExit_triggered();
    void on_btnBrowseInput_clicked();
    void on_btnBrowseOutput_clicked();
    void on_btnStart_clicked();
    void on_btnStop_clicked();
    void on_btnLoadPreset_clicked();
    void on_btnSavePreset_clicked();
    void on_btnShowDetails_toggled(bool checked);
    void on_btnCopyCommand_clicked();
    void on_tabWidget_currentChanged(int index);
    void updateProgress(qint64 bytesProcessed, qint64 bytesTotal);
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(QProcess::ProcessError error);
    void readProcessOutput();
    
    void on_btnBrowseDVDDevice_clicked();
    void on_btnBrowseDVDFolder_clicked();
    void on_btnDVDReadStart_clicked();
    void on_btnBrowseDVDVideoFiles_clicked();
    void on_btnBrowseDVDDestination_clicked();
    void on_btnDVDWriteStart_clicked();
    void on_btnBrowseDVDCopySource_clicked();
    void on_btnBrowseDVDCopyDest_clicked();
    void on_btnDVDCopyStart_clicked();
    
    void on_checkAudioNormalize_stateChanged(int state);
    void on_checkAudioEq_stateChanged(int state);
    void on_checkAudioCompress_stateChanged(int state);
    void on_checkAudioDenoise_stateChanged(int state);
    
    void on_btnBrowseImageInput_clicked();
    void on_btnBrowseImageOutput_clicked();
    void on_btnImageStart_clicked();
    void on_btnImageStop_clicked();
    void on_btnImagePreview_clicked();
    void on_comboImageFormat_currentTextChanged(const QString &text);
    void on_sliderBrightness_valueChanged(int value);
    void on_sliderContrast_valueChanged(int value);
    void on_sliderImageQuality_valueChanged(int value);
	
	void on_btnBrowseProbe_clicked();
    void on_btnRunProbe_clicked();
    void on_btnCopyProbeOutput_clicked();
    void on_btnClearProbeOutput_clicked();
    void onProbeOutputReady();
    void onProbeErrorReady();
    void onProbeFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
    void setupMediaPlayer();
    void on_btnPlayFile_clicked();
    void on_btnStopPlayback_clicked();
    void on_btnPausePlayback_clicked();
    void on_sliderVolume_valueChanged(int value);
    void onPlaybackStarted();
    void onPlaybackStopped();
    void onPlaybackPaused();
    void onPlaybackResumed();
    void onErrorOccurred(const QString& error);
    void onPositionChanged(int position);
    void onDurationChanged(int duration);
    void updatePlaybackControls();
    
    void on_btnRunCustomCommand_clicked();
    void on_btnStopCustomCommand_clicked();
    void on_btnClearCustomOutput_clicked();
    void onCustomOutputReady();
    void onCustomErrorReady();
    void onCustomFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
    void on_btnDownloadYtdlp_clicked();
    void on_btnStopYtdlp_clicked();
    void on_btnBrowseYtdlpOutput_clicked();
    void onYtdlpOutputReady();
    void onYtdlpErrorReady();
    void onYtdlpFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
    void on_btnBrowseWhisperInput_clicked();
    void on_btnBrowseWhisperOutput_clicked();
    void on_btnRunWhisper_clicked();
    void on_btnStopWhisper_clicked();
    void onWhisperOutputReady();
    void onWhisperErrorReady();
    void onWhisperFinished(int exitCode, QProcess::ExitStatus exitStatus);


private:
    Ui::MainWindow *ui;
    std::unique_ptr<FFmpegProcessor> m_ffmpegProcessor;
    std::unique_ptr<PresetManager> m_presetManager;
    QString m_lastCommand;
    QString m_outputFolder;
    bool m_isProcessingBatch;
    int m_currentBatchIndex;
    QString buildImageCommand(const QString& inputFile, const QString& outputFile);
    void setupImageTools();
    bool isImageFile(const QString& filePath);
    QString getImageFormatFromFilename(const QString& filename);

    QString m_imageInputFile;
    QString m_imageOutputFile;
    
    MediaPlayer* m_mediaPlayer;
    bool m_isMediaPlaying;

    QProcess* m_probeProcess;
    void runFFprobe(const QStringList& files);
    bool m_audioNormalizeEnabled;
    bool m_audioEqEnabled;
    bool m_audioCompressEnabled;
    bool m_audioDenoiseEnabled;


    QProcess* m_ytdlpProcess;
    QString m_ytdlpPath;
    
    void checkAndDownloadYTDLPIfNeeded();
    void downloadYTDLPLatest();
    void runYTDLPOptions(const QString& url, const QString& outputPath, bool audioOnly, const QString& quality);
    QString parseYTDLPOutput(const QString& output);
    
    void setupUI();
    void connectSignals();
    void loadPresets();
    void saveSettings();
    void loadSettings();
    QString buildFFmpegCommand(const QString& inputFile, const QString& outputFile);
    void addToLog(const QString& message, const QColor& color = Qt::black);
    void enableControls(bool enabled);
    bool isAudioOnlyFile(const QString& filePath);
    void updateHardwareAccelerationOptions();
    void addFileToQueue(const QString& inputFile);
    void processNextBatchItem();
    void updateQueueWithNewOutputFolder();
    
    void setupAudioFilterControls();
    QString buildAudioFilterChain() const;
    void updateAudioFilterVisibility();
    
    QString getDVDDriveList();
    
    QProcess* m_customProcess;
    
    void runCustomCommand(const QString& command, const QString& workingDir);
    
    QProcess* m_whisperProcess;
    QString m_whisperPath;
    QString m_whisperModelsPath;
    
    void setupWhisper();
    void downloadWhisperIfMissing();
    void runWhisperTranscription(const QString& inputFile, const QString& outputFile, const QString& model, bool translateToEnglish);
};

#endif // MAINWINDOW_H
