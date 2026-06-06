#include "MainWindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QDateTime>
#include <QScrollBar>
#include <QClipboard>
#include <QDirIterator>
#include <QFileInfo>
#include <QInputDialog>
#include <QTimer>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>


#include <QUrl>
#include "utils.h"

#include "MediaPlayer.h"



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_ffmpegProcessor(std::make_unique<FFmpegProcessor>())
    , m_presetManager(std::make_unique<PresetManager>())
    , m_lastCommand("")
    , m_outputFolder("")
    , m_isProcessingBatch(false)
    , m_currentBatchIndex(0)
    , m_probeProcess(nullptr)
    , m_customProcess(new QProcess(this))  
    , m_mediaPlayer(nullptr)
    , m_isMediaPlaying(false)
    
    , m_ytdlpProcess(new QProcess(this))
    , m_ytdlpPath("") 
    
    , m_whisperProcess(new QProcess(this))
    , m_whisperPath("")
    , m_whisperModelsPath("")
{
    ui->setupUi(this);
    m_probeProcess = new QProcess(this);
    setupUI();
    setupWhisper();
    connectSignals();
    loadSettings();
    loadPresets();
    updateHardwareAccelerationOptions();
    
    setupMediaPlayer();
    
    ui->detailsPanel->setVisible(false);
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::setupUI()
{
    setWindowTitle("FFGUI++");
    resize(1036, 848);
    
    ui->tableQueue->setColumnCount(3);
    ui->tableQueue->setHorizontalHeaderLabels({"Input File", "Output File", "Status"});
    ui->tableQueue->horizontalHeader()->setStretchLastSection(true);
    ui->tableQueue->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    ui->progressBar->setVisible(false);
    
    ui->textLog->setTextColor(Qt::black);
    
    QStringList hwAccelOptions = {
        "None", "CUDA", "NVENC", "DXVA2", "D3D11VA", "VAAPI", "Videotoolbox"
    };
    ui->comboHwAccel->addItems(hwAccelOptions);
    
    QStringList videoCodecs = {
        "libx264", "libx265", "h264_nvenc", "hevc_nvenc", "h264_qsv", "hevc_qsv",
        "libvpx-vp9", "libaom-av1", "libsvtav1", "h264_videotoolbox", "hevc_videotoolbox"
    };
    ui->comboVideoCodec->addItems(videoCodecs);
    
    QStringList audioCodecs = {
        "aac", "libmp3lame", "libopus", "libvorbis", "ac3", "dts", "flac"
    };
    ui->comboAudioCodec->addItems(audioCodecs);
    
    QStringList resolutions = {
        "Original", "1920x1080", "1280x720", "3840x2160", "2560x1440"
    };
    ui->comboResolution->addItems(resolutions);
    
    QStringList framerates = {"Original", "23.976", "24", "25", "29.97", "30", "50", "59.94", "60"};
    ui->comboFramerate->addItems(framerates);
    
    QStringList qualityPresets = {"Ultra Fast", "Super Fast", "Very Fast", "Faster", "Fast", "Medium", "Slow", "Slower", "Very Slow"};
    ui->comboQuality->addItems(qualityPresets);
    
    ui->comboQuality->setCurrentText("Medium");
    
    QStringList containers = {
        "mp4", "mkv", "mov", "avi", "webm", "flv", "ts", "m4v", "wmv", "3gp",
        "mp3", "m4a", "wav", "flac", "ogg", "aac", "wma", "opus", "alac", "mka",
        "jpg", "png", "gif", "bmp", "tiff", "webp", "tga", "psd", "svg", "ico"
    };
    ui->comboContainer->addItems(containers);
    
    ui->comboVideoCodec->setCurrentText("libx264");
    ui->comboAudioCodec->setCurrentText("aac");
    ui->comboContainer->setCurrentText("mp4");
    ui->spinCRF->setValue(23);
    ui->spinBitrate->setValue(2000);
    ui->spinAudioBitrate->setValue(128);
    ui->checkTwoPass->setChecked(false);
    
    ui->detailsPanel->setVisible(false);
    ui->btnShowDetails->setCheckable(true);
    ui->btnShowDetails->setChecked(false);
    
    ui->comboCustomDropdown->addItem("ffmpeg");
    ui->comboCustomDropdown->addItem("ffprobe");
    ui->comboCustomDropdown->addItem("ffplay");
    ui->comboCustomDropdown->addItem("yt-dlp");
    ui->comboCustomDropdown->setCurrentIndex(0);
    
    enableControls(true);
    setupAudioFilterControls();
    setupImageTools();
    
    setupMediaPlayer();
    
    QStringList qualities = {"best", "1080p", "720p", "480p", "360p", "worst"};
    ui->comboYtdlpQuality->addItems(qualities);
    ui->comboYtdlpQuality->setCurrentText("best");
    
    ui->lineYtdlpOutput->setText(QDir::homePath() + "/Downloads");
}


void MainWindow::connectSignals()
{
    disconnect(ui->actionOpen, nullptr, this, nullptr);
    disconnect(ui->actionSaveAs, nullptr, this, nullptr);
    disconnect(ui->actionExit, nullptr, this, nullptr);
    disconnect(ui->btnBrowseInput, nullptr, this, nullptr);
    disconnect(ui->btnBrowseOutput, nullptr, this, nullptr);
    disconnect(ui->btnStart, nullptr, this, nullptr);
    disconnect(ui->btnStop, nullptr, this, nullptr);
    disconnect(ui->btnLoadPreset, nullptr, this, nullptr);
    disconnect(ui->btnSavePreset, nullptr, this, nullptr);
    disconnect(ui->btnShowDetails, nullptr, this, nullptr);
    disconnect(ui->btnCopyCommand, nullptr, this, nullptr);
    disconnect(m_ffmpegProcessor.get(), nullptr, this, nullptr);
    disconnect(ui->tabWidget, nullptr, this, nullptr);
    
    disconnect(ui->btnBrowseImageInput, nullptr, this, nullptr);
    disconnect(ui->btnBrowseImageOutput, nullptr, this, nullptr);
    disconnect(ui->btnImageStart, nullptr, this, nullptr);
    disconnect(ui->btnImageStop, nullptr, this, nullptr);
    disconnect(ui->btnImagePreview, nullptr, this, nullptr);
    
    disconnect(ui->checkAudioNormalize, nullptr, this, nullptr);
    disconnect(ui->checkAudioEq, nullptr, this, nullptr);
    disconnect(ui->checkAudioCompress, nullptr, this, nullptr);
    disconnect(ui->checkAudioDenoise, nullptr, this, nullptr);
    disconnect(ui->btnBrowseProbe, nullptr, this, nullptr);
    disconnect(ui->btnRunProbe, nullptr, this, nullptr);
    disconnect(ui->btnCopyProbeOutput, nullptr, this, nullptr);
    disconnect(ui->btnClearProbeOutput, nullptr, this, nullptr);
    
    disconnect(ui->btnPlayFile, nullptr, this, nullptr);
    disconnect(ui->btnStopPlayback, nullptr, this, nullptr);
    disconnect(ui->btnPausePlayback, nullptr, this, nullptr);
    disconnect(ui->sliderVolume, nullptr, this, nullptr);

    disconnect(ui->btnRunCustomCommand, nullptr, this, nullptr);
    disconnect(ui->btnStopCustomCommand, nullptr, this, nullptr);
    disconnect(ui->btnClearCustomOutput, nullptr, this, nullptr);
    
    connect(ui->btnRunCustomCommand, &QPushButton::clicked, this, &MainWindow::on_btnRunCustomCommand_clicked);
    connect(ui->btnStopCustomCommand, &QPushButton::clicked, this, &MainWindow::on_btnStopCustomCommand_clicked);
    connect(ui->btnClearCustomOutput, &QPushButton::clicked, this, &MainWindow::on_btnClearCustomOutput_clicked);
    connect(m_customProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onCustomOutputReady);
    connect(m_customProcess, &QProcess::readyReadStandardError, this, &MainWindow::onCustomErrorReady);
    connect(m_customProcess, &QProcess::finished, this, &MainWindow::onCustomFinished);


    disconnect(ui->btnDownloadYtdlp, nullptr, this, nullptr);
    disconnect(ui->btnStopYtdlp, nullptr, this, nullptr);
    disconnect(ui->btnBrowseYtdlpOutput, nullptr, this, nullptr);
    
    connect(ui->btnDownloadYtdlp, &QPushButton::clicked, this, &MainWindow::on_btnDownloadYtdlp_clicked);
    connect(ui->btnStopYtdlp, &QPushButton::clicked, this, &MainWindow::on_btnStopYtdlp_clicked);
    connect(ui->btnBrowseYtdlpOutput, &QPushButton::clicked, this, &MainWindow::on_btnBrowseYtdlpOutput_clicked);
    connect(m_ytdlpProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onYtdlpOutputReady);
    connect(m_ytdlpProcess, &QProcess::readyReadStandardError, this, &MainWindow::onYtdlpErrorReady);
    connect(m_ytdlpProcess, &QProcess::finished, this, &MainWindow::onYtdlpFinished);

    connect(ui->btnBrowseProbe, &QPushButton::clicked, this, &MainWindow::on_btnBrowseProbe_clicked);
    connect(ui->btnRunProbe, &QPushButton::clicked, this, &MainWindow::on_btnRunProbe_clicked);
    connect(ui->btnCopyProbeOutput, &QPushButton::clicked, this, &MainWindow::on_btnCopyProbeOutput_clicked);
    connect(ui->btnClearProbeOutput, &QPushButton::clicked, this, &MainWindow::on_btnClearProbeOutput_clicked);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::on_actionOpen_triggered);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::on_actionSaveAs_triggered);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::on_actionExit_triggered);
    
    connect(ui->btnBrowseInput, &QPushButton::clicked, this, &MainWindow::on_btnBrowseInput_clicked);
    connect(ui->btnBrowseOutput, &QPushButton::clicked, this, &MainWindow::on_btnBrowseOutput_clicked);
    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::on_btnStart_clicked);
    connect(ui->btnStop, &QPushButton::clicked, this, &MainWindow::on_btnStop_clicked);
    connect(ui->btnLoadPreset, &QPushButton::clicked, this, &MainWindow::on_btnLoadPreset_clicked);
    connect(ui->btnSavePreset, &QPushButton::clicked, this, &MainWindow::on_btnSavePreset_clicked);
    
    connect(ui->btnShowDetails, &QPushButton::toggled, this, &MainWindow::on_btnShowDetails_toggled);
    connect(ui->btnCopyCommand, &QPushButton::clicked, this, &MainWindow::on_btnCopyCommand_clicked);
    
    connect(m_ffmpegProcessor.get(), &FFmpegProcessor::progressUpdated, 
            this, &MainWindow::updateProgress);
    connect(m_ffmpegProcessor.get(), &FFmpegProcessor::finished, 
            this, &MainWindow::processFinished);
    connect(m_ffmpegProcessor.get(), &FFmpegProcessor::errorOccurred, 
            this, &MainWindow::processError);
    connect(m_ffmpegProcessor.get(), &FFmpegProcessor::outputReceived, 
            this, &MainWindow::readProcessOutput);
    
    connect(ui->tabWidget, &QTabWidget::currentChanged,
            this, &MainWindow::on_tabWidget_currentChanged);
    connect(m_probeProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onProbeOutputReady);
    connect(m_probeProcess, &QProcess::readyReadStandardError, this, &MainWindow::onProbeErrorReady);
    connect(m_probeProcess, &QProcess::finished, this, &MainWindow::onProbeFinished);
            
    connect(ui->btnBrowseImageInput, &QPushButton::clicked, 
            this, &MainWindow::on_btnBrowseImageInput_clicked);
    connect(ui->btnBrowseImageOutput, &QPushButton::clicked, 
            this, &MainWindow::on_btnBrowseImageOutput_clicked);
    connect(ui->btnImageStart, &QPushButton::clicked, 
            this, &MainWindow::on_btnImageStart_clicked);
    connect(ui->btnImageStop, &QPushButton::clicked, 
            this, &MainWindow::on_btnImageStop_clicked);
    connect(ui->btnImagePreview, &QPushButton::clicked, 
            this, &MainWindow::on_btnImagePreview_clicked);
    
    connect(ui->checkAudioNormalize, &QCheckBox::stateChanged, 
            this, &MainWindow::on_checkAudioNormalize_stateChanged);
    connect(ui->checkAudioEq, &QCheckBox::stateChanged, 
            this, &MainWindow::on_checkAudioEq_stateChanged);
    connect(ui->checkAudioCompress, &QCheckBox::stateChanged, 
            this, &MainWindow::on_checkAudioCompress_stateChanged);
    connect(ui->checkAudioDenoise, &QCheckBox::stateChanged, 
            this, &MainWindow::on_checkAudioDenoise_stateChanged);
    
 
    connect(ui->btnPlayFile, &QPushButton::clicked, this, &MainWindow::on_btnPlayFile_clicked);
    connect(ui->btnStopPlayback, &QPushButton::clicked, this, &MainWindow::on_btnStopPlayback_clicked);
    connect(ui->btnPausePlayback, &QPushButton::clicked, this, &MainWindow::on_btnPausePlayback_clicked);
    connect(ui->sliderVolume, &QSlider::valueChanged, this, &MainWindow::on_sliderVolume_valueChanged);
    
    disconnect(ui->btnBrowseWhisperInput, nullptr, this, nullptr);
    disconnect(ui->btnBrowseWhisperOutput, nullptr, this, nullptr);
    disconnect(ui->btnRunWhisper, nullptr, this, nullptr);
    disconnect(ui->btnStopWhisper, nullptr, this, nullptr);

    connect(ui->btnBrowseWhisperInput, &QPushButton::clicked, this, &MainWindow::on_btnBrowseWhisperInput_clicked);
    connect(ui->btnBrowseWhisperOutput, &QPushButton::clicked, this, &MainWindow::on_btnBrowseWhisperOutput_clicked);
    connect(ui->btnRunWhisper, &QPushButton::clicked, this, &MainWindow::on_btnRunWhisper_clicked);
    connect(ui->btnStopWhisper, &QPushButton::clicked, this, &MainWindow::on_btnStopWhisper_clicked);
    connect(m_whisperProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onWhisperOutputReady);
    connect(m_whisperProcess, &QProcess::readyReadStandardError, this, &MainWindow::onWhisperErrorReady);
    connect(m_whisperProcess, &QProcess::finished, this, &MainWindow::onWhisperFinished);    

}

void MainWindow::on_actionExit_triggered()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Exit Application"), 
                                 tr("Are you sure you want to exit?"),
                                 QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        close();
    }
}

void MainWindow::loadPresets()
{
    auto presets = m_presetManager->getAvailablePresets();
    ui->comboPresets->clear();
    ui->comboPresets->addItem("Custom");
    for (const auto& preset : presets) {
        ui->comboPresets->addItem(preset);
    }
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

void MainWindow::loadSettings()
{
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void MainWindow::on_actionOpen_triggered()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open Media Files"), 
                                                         QDir::homePath(),
                                                         tr("Media Files (*.mp4 *.mkv *.avi *.mov *.wmv *.flv *.webm *.m4v *.mp3 *.wav *.flac *.aac *.m4a);;All Files (*)"));
    if (!fileNames.isEmpty()) {
        if (fileNames.size() == 1) {
            QString fileName = fileNames.first();
            ui->lineInputFile->setText(fileName);
            
            QFileInfo fileInfo(fileName);
            ui->lineOutputFile->setText(fileInfo.completeBaseName());
            ui->lineOutputFile->setEnabled(true);
        } else {
            ui->lineInputFile->setText(QString("%1 files selected").arg(fileNames.size()));
            ui->lineOutputFile->setText("Multiple files selected");
            ui->lineOutputFile->setEnabled(false);
            
            ui->tableQueue->setRowCount(0);
            
            for (const QString& fileName : fileNames) {
                addFileToQueue(fileName);
            }
            
            ui->tabWidget->setCurrentIndex(1);
        }
    }
}

void MainWindow::addFileToQueue(const QString& inputFile)
{
    QFileInfo inputInfo(inputFile);
    QString outputExtension = ui->comboContainer->currentText();
    QString outputFileName = inputInfo.completeBaseName() + "." + outputExtension;
    QString outputFile = (m_outputFolder.isEmpty() ? inputInfo.dir().absolutePath() : m_outputFolder) + "/" + outputFileName;
    
    int row = ui->tableQueue->rowCount();
    ui->tableQueue->insertRow(row);
    
    QTableWidgetItem* inputItem = new QTableWidgetItem(inputFile);
    QTableWidgetItem* outputItem = new QTableWidgetItem(outputFile);
    QTableWidgetItem* statusItem = new QTableWidgetItem("Queued");
    
    ui->tableQueue->setItem(row, 0, inputItem);
    ui->tableQueue->setItem(row, 1, outputItem);
    ui->tableQueue->setItem(row, 2, statusItem);
}

void MainWindow::on_actionSaveAs_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), 
                                                   QDir::homePath(),
                                                   tr("All Files (*.*)"));
    if (!fileName.isEmpty()) {
        ui->lineOutputFile->setText(fileName);
    }
}

 
void MainWindow::setupMediaPlayer()
{
    qDebug() << "Setting up MediaPlayer in MainWindow...";
    m_mediaPlayer = new MediaPlayer(this);
    
    connect(m_mediaPlayer, &MediaPlayer::playbackStarted, this, &MainWindow::onPlaybackStarted);
    connect(m_mediaPlayer, &MediaPlayer::playbackStopped, this, &MainWindow::onPlaybackStopped);
    connect(m_mediaPlayer, &MediaPlayer::playbackPaused, this, &MainWindow::onPlaybackPaused);
    connect(m_mediaPlayer, &MediaPlayer::playbackResumed, this, &MainWindow::onPlaybackResumed);
    connect(m_mediaPlayer, &MediaPlayer::errorOccurred, this, &MainWindow::onErrorOccurred);
    connect(m_mediaPlayer, &MediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_mediaPlayer, &MediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_mediaPlayer, &MediaPlayer::volumeChanged, this, &MainWindow::on_sliderVolume_valueChanged);
    
    qDebug() << "MediaPlayer setup complete";
}

void MainWindow::setupWhisper()
{
    QString appDir = QCoreApplication::applicationDirPath();
    m_whisperPath = appDir + "/main"; 
    m_whisperModelsPath = appDir + "/models";
    
    QDir modelsDir(m_whisperModelsPath);
    if (!modelsDir.exists()) {
        modelsDir.mkpath(".");
    }
    
    QStringList availableModels = {
        "tiny", "tiny.en", "base", "base.en", "small", "small.en", 
        "medium", "medium.en", "large-v1", "large-v2", "large-v3"
    };
    ui->comboWhisperModel->addItems(availableModels);
    ui->comboWhisperModel->setCurrentText("base");
    
    if (!QFile::exists(m_whisperPath)) {
        ui->textWhisperOutput->append("Whisper not found. Please download whisper.cpp executable and place it as 'whisper.exe' in the application directory.");
    }
    
    ui->lineWhisperOutput->setText(QDir::homePath() + "/transcription.txt");
}

void MainWindow::on_btnBrowseWhisperInput_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Audio/Video File"), 
                                                   QDir::homePath(),
                                                   tr("Media Files (*.mp3 *.wav *.mp4 *.mkv *.avi *.mov *.m4a *.flac *.aac *.wma *.ogg);;All Files (*.*)"));
    if (!fileName.isEmpty()) {
        ui->lineWhisperInput->setText(fileName);
        
        QFileInfo fileInfo(fileName);
        QString baseName = fileInfo.completeBaseName();
        ui->lineWhisperOutput->setText(fileInfo.dir().absolutePath() + "/" + baseName + "_transcription.txt");
    }
}

void MainWindow::on_btnBrowseWhisperOutput_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Transcription As"), 
                                                   QDir::homePath(),
                                                   tr("Text Files (*.txt);;All Files (*.*)"));
    if (!fileName.isEmpty()) {
        ui->lineWhisperOutput->setText(fileName);
    }
}

void MainWindow::on_btnRunWhisper_clicked()
{
    QString inputFile = ui->lineWhisperInput->text().trimmed();
    if (inputFile.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please select an input file."));
        return;
    }
    
    if (!QFile::exists(inputFile)) {
        QMessageBox::warning(this, tr("Warning"), tr("Input file does not exist."));
        return;
    }
    
    QString outputFile = ui->lineWhisperOutput->text().trimmed();
    if (outputFile.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please specify an output file."));
        return;
    }
    
    QString model = ui->comboWhisperModel->currentText();
    bool translate = ui->checkWhisperTranslate->isChecked();
    
    QString modelPath = m_whisperModelsPath + "/ggml-" + model + ".bin";
    if (!QFile::exists(modelPath)) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, tr("Model Missing"), 
                                     tr("The selected model file (%1) is not found. Download it from https://huggingface.co/ggerganov/whisper.cpp and place it in the 'models' folder. Continue anyway?").arg(model),
                                     QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            return;
        }
    }
    
    ui->textWhisperOutput->clear();
    ui->textWhisperOutput->append(QString("[%1] Starting transcription...").arg(QTime::currentTime().toString()));
    ui->textWhisperOutput->append(QString("Input: %1").arg(inputFile));
    ui->textWhisperOutput->append(QString("Output: %1").arg(outputFile));
    ui->textWhisperOutput->append(QString("Model: %1").arg(model));
    ui->textWhisperOutput->append(QString("Translate: %1").arg(translate ? "Yes" : "No"));
    ui->textWhisperOutput->append("-------------------");
    
    ui->btnRunWhisper->setEnabled(false);
    ui->btnStopWhisper->setEnabled(true);
    ui->progressWhisper->setVisible(true);
    ui->progressWhisper->setValue(0);
    
    QStringList arguments;
    arguments << "-m" << modelPath;
    arguments << "-f" << inputFile;
    arguments << "-of" << outputFile;
    
    if (translate) {
        arguments << "-tr";
    }
    
    arguments << "-otxt";
    
    ui->textWhisperOutput->append(QString("Running: whisper %1").arg(arguments.join(" ")));
    
    m_whisperProcess->start(m_whisperPath, arguments);
    
    if (!m_whisperProcess->waitForStarted(5000)) {
        ui->textWhisperOutput->append(QString("Failed to start Whisper: %1").arg(m_whisperProcess->errorString()));
        ui->btnRunWhisper->setEnabled(true);
        ui->btnStopWhisper->setEnabled(false);
        ui->progressWhisper->setVisible(false);
    }
}

void MainWindow::on_btnStopWhisper_clicked()
{
    if (m_whisperProcess->state() == QProcess::Running) {
        m_whisperProcess->terminate();
        if (!m_whisperProcess->waitForFinished(3000)) {
            m_whisperProcess->kill();
            m_whisperProcess->waitForFinished(1000);
        }
        ui->textWhisperOutput->append("Transcription terminated by user.");
    }
    ui->btnRunWhisper->setEnabled(true);
    ui->btnStopWhisper->setEnabled(false);
    ui->progressWhisper->setVisible(false);
}

void MainWindow::onWhisperOutputReady()
{
    QByteArray output = m_whisperProcess->readAllStandardOutput();
    QString outputStr = QString::fromLocal8Bit(output);
    if (!outputStr.trimmed().isEmpty()) {
        ui->textWhisperOutput->append(outputStr.trimmed());
    }
}

void MainWindow::onWhisperErrorReady()
{
    QByteArray errorOutput = m_whisperProcess->readAllStandardError();
    QString errorStr = QString::fromLocal8Bit(errorOutput);
    if (!errorStr.trimmed().isEmpty()) {
        ui->textWhisperOutput->append(QString("ERROR: %1").arg(errorStr.trimmed()));
    }
}

void MainWindow::onWhisperFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    
    ui->btnRunWhisper->setEnabled(true);
    ui->btnStopWhisper->setEnabled(false);
    ui->progressWhisper->setVisible(false);
    
    if (exitCode == 0) {
        ui->textWhisperOutput->append(QString("[%1] Transcription completed successfully!").arg(QTime::currentTime().toString()));
        QMessageBox::information(this, tr("Success"), tr("Transcription completed successfully!"));
        
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("Open Result"), 
            tr("Transcription completed. Open the result file?"),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            QString outputFile = ui->lineWhisperOutput->text();
            if (!outputFile.isEmpty() && QFile::exists(outputFile)) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(outputFile));
            }
        }
    } else {
        ui->textWhisperOutput->append(QString("[%1] Transcription failed with exit code: %2").arg(QTime::currentTime().toString()).arg(exitCode));
        QMessageBox::warning(this, tr("Error"), tr("Transcription failed! Check the output for details."));
    }
}


void MainWindow::on_btnPlayFile_clicked()
{
    QString inputFile = ui->lineInputFile->text();
    
    if (inputFile.isEmpty()) {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Select Media File"), 
                                                       QDir::homePath(),
                                                       tr("Media Files (*.mp4 *.mkv *.avi *.mov *.wmv *.flv *.webm *.m4v *.mp3 *.wav *.flac *.aac *.m4a *.wav *.ogg);;All Files (*)"));
        if (!fileName.isEmpty()) {
            inputFile = fileName;
        } else {
            return;
        }
    }
    
    if (!QFile::exists(inputFile)) {
        QMessageBox::warning(this, tr("Warning"), tr("File does not exist."));
        return;
    }
    
    if (m_mediaPlayer->playFile(inputFile)) {
        m_isMediaPlaying = true;
        updatePlaybackControls();
    }
}

void MainWindow::on_btnStopPlayback_clicked()
{
    m_mediaPlayer->stop();
}

void MainWindow::on_btnPausePlayback_clicked()
{
    if (m_mediaPlayer->isPlaying() && !m_mediaPlayer->isPaused()) {
        m_mediaPlayer->pause();
    } else {
        m_mediaPlayer->resume();
    }
}

void MainWindow::on_sliderVolume_valueChanged(int value)
{
    ui->labelVolumeValue->setText(QString::number(value));
 
    if (m_mediaPlayer) {
        m_mediaPlayer->setVolume(value);
    }

}

void MainWindow::onPlaybackStarted()
{
    m_isMediaPlaying = true;
    updatePlaybackControls();
    addToLog("Media playback started", Qt::darkGreen);
}


void MainWindow::on_btnRunCustomCommand_clicked()
{
    QString command = ui->textCustomCommand->toPlainText().trimmed();
    if (command.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a command to execute."));
        return;
    }
    
    QString selectedTool = ui->comboCustomDropdown->currentText();
    QString commandPrefix = ui->comboCustomDropdown->currentData().toString();
    
    QString workingDir = ui->lineCustomWorkingDir->text().trimmed();
    
    ui->textCustomOutput->append(QString("[%1] Running custom command...").arg(QTime::currentTime().toString()));
    
    QString fullCommand = command;
    
    if (!commandPrefix.isEmpty() && !selectedTool.isEmpty() && selectedTool != "Custom Command") {
        ui->textCustomOutput->append(QString("Tool: %1").arg(selectedTool));
        fullCommand = commandPrefix + " " + command;
    } else {
        ui->textCustomOutput->append(QString("Custom command mode"));
    }
    
    ui->textCustomOutput->append(QString("Command: %1").arg(fullCommand));
    if (!workingDir.isEmpty()) {
        ui->textCustomOutput->append(QString("Working Directory: %1").arg(workingDir));
    }
    ui->textCustomOutput->append("-------------------");
    
    runCustomCommand(fullCommand, workingDir);
}


void MainWindow::runCustomCommand(const QString& command, const QString& workingDir)
{
    QStringList args = splitCommand(command);
    
    QString program;
    if (!args.isEmpty()) {
        program = args.takeFirst();
    }
    
    if (!workingDir.isEmpty()) {
        m_customProcess->setWorkingDirectory(workingDir);
    } else {
        m_customProcess->setWorkingDirectory(QDir::currentPath());
    }
    
    ui->btnRunCustomCommand->setEnabled(false);
    ui->btnStopCustomCommand->setEnabled(true);
    
    m_customProcess->start(program, args);
    
    if (!m_customProcess->waitForStarted(5000)) {
        ui->textCustomOutput->append(QString("Failed to start: %1").arg(m_customProcess->errorString()));
        ui->btnRunCustomCommand->setEnabled(true);
        ui->btnStopCustomCommand->setEnabled(false);
    }
}

QStringList MainWindow::splitCommand(const QString& command)
{
    QStringList args;
    QString currentArg;
    bool inQuotes = false;
    QChar quoteChar;
    
    for (int i = 0; i < command.length(); ++i) {
        QChar c = command[i];
        
        if (c == '"' || c == '\'') {
            if (!inQuotes) {
                inQuotes = true;
                quoteChar = c;
            } else if (quoteChar == c) {
                inQuotes = false;
            } else {
                currentArg += c;
            }
        } else if (c == ' ' && !inQuotes) {
            if (!currentArg.isEmpty()) {
                args << currentArg;
                currentArg.clear();
            }
        } else {
            currentArg += c;
        }
    }
    
    if (!currentArg.isEmpty()) {
        args << currentArg;
    }
    
    return args;
}

void MainWindow::on_btnStopCustomCommand_clicked()
{
    if (m_customProcess->state() == QProcess::Running) {
        m_customProcess->terminate();
        if (!m_customProcess->waitForFinished(3000)) {
            m_customProcess->kill();
            m_customProcess->waitForFinished(1000);
        }
        ui->textCustomOutput->append("Process terminated by user.");
    }
    ui->btnRunCustomCommand->setEnabled(true);
    ui->btnStopCustomCommand->setEnabled(false);
}

void MainWindow::on_btnClearCustomOutput_clicked()
{
    ui->textCustomOutput->clear();
}

void MainWindow::onCustomOutputReady()
{
    QByteArray output = m_customProcess->readAllStandardOutput();
    QString outputStr = QString::fromLocal8Bit(output);
    if (!outputStr.trimmed().isEmpty()) {
        ui->textCustomOutput->append(outputStr.trimmed());
    }
}


void MainWindow::checkAndDownloadYTDLPIfNeeded()
{
    QStringList possiblePaths;
    possiblePaths << QCoreApplication::applicationDirPath() + "/yt-dlp.exe";
    possiblePaths << "./yt-dlp.exe";
    possiblePaths << "yt-dlp.exe";
    
    m_ytdlpPath = "";
    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            m_ytdlpPath = path;
            ui->textYtdlpOutput->append("Found yt-dlp at: " + path);
            break;
        }
    }
    
    if (m_ytdlpPath.isEmpty()) {
        QString currentDirYtdlp = QDir::currentPath() + "/yt-dlp.exe";
        if (QFile::exists(currentDirYtdlp)) {
            m_ytdlpPath = currentDirYtdlp;
            ui->textYtdlpOutput->append("Found yt-dlp at (current dir): " + currentDirYtdlp);
        }
    }
    
    if (m_ytdlpPath.isEmpty()) {
        if (QFile::exists("yt-dlp.exe")) {
            m_ytdlpPath = "yt-dlp.exe";
            ui->textYtdlpOutput->append("Found yt-dlp at (relative): yt-dlp.exe");
        }
    }
    
    if (m_ytdlpPath.isEmpty()) {
        ui->textYtdlpOutput->append("yt-dlp not found in any expected location.");
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, tr("yt-dlp Required"), 
                                     tr("yt-dlp is required for YouTube downloading functionality. Would you like to download it now?"),
                                     QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            m_ytdlpPath = QCoreApplication::applicationDirPath() + "/yt-dlp.exe";
            downloadYTDLPLatest();
        } else {
            int ytdlpTabIndex = ui->tabWidget->indexOf(ui->tabYtdlp);
            if (ytdlpTabIndex != -1) {
                ui->tabWidget->setTabEnabled(ytdlpTabIndex, false);
            }
        }
    } else {
        ui->textYtdlpOutput->append("yt-dlp confirmed available at: " + m_ytdlpPath);
        int ytdlpTabIndex = ui->tabWidget->indexOf(ui->tabYtdlp);
        if (ytdlpTabIndex != -1) {
            ui->tabWidget->setTabEnabled(ytdlpTabIndex, true);
        }
    }
}


void MainWindow::downloadYTDLPLatest()
{
    ui->textYtdlpOutput->append("Downloading latest yt-dlp...");
    
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl("https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe"));
    
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    
    QNetworkReply* reply = manager->get(request);
    
    QFile* file = new QFile(m_ytdlpPath);
    if (!file->open(QIODevice::WriteOnly)) {
        ui->textYtdlpOutput->append("Failed to create yt-dlp.exe file");
        delete file;
        reply->deleteLater();
        manager->deleteLater();
        return;
    }
    
    connect(reply, &QNetworkReply::readyRead, this, [reply, file]() {
        file->write(reply->readAll());
    });
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, file, manager]() {
        file->close();
        
        if (reply->error() == QNetworkReply::NoError) {
            ui->textYtdlpOutput->append("yt-dlp downloaded successfully!");
            ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabYtdlp), true);
        } else {
            ui->textYtdlpOutput->append(QString("Download failed: %1").arg(reply->errorString()));
            QFile::remove(m_ytdlpPath);
        }
        
        file->deleteLater();
        reply->deleteLater();
        manager->deleteLater();
    });
}

void MainWindow::on_btnDownloadYtdlp_clicked()
{
    QString url = ui->lineYtdlpUrl->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a YouTube URL."));
        return;
    }
    
    if (!url.contains("youtube.com") && !url.contains("youtu.be")) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a valid YouTube URL."));
        return;
    }
    
    QString outputPath = ui->lineYtdlpOutput->text().trimmed();
    if (outputPath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please specify an output path."));
        return;
    }
    
    bool audioOnly = ui->checkYtdlpAudioOnly->isChecked();
    QString quality = ui->comboYtdlpQuality->currentText();
    
    ui->textYtdlpOutput->clear();
    ui->textYtdlpOutput->append(QString("[%1] Starting download...").arg(QTime::currentTime().toString()));
    ui->textYtdlpOutput->append(QString("URL: %1").arg(url));
    ui->textYtdlpOutput->append(QString("Output: %1").arg(outputPath));
    ui->textYtdlpOutput->append(QString("Quality: %1").arg(quality));
    ui->textYtdlpOutput->append(QString("Audio Only: %1").arg(audioOnly ? "Yes" : "No"));
    ui->textYtdlpOutput->append("-------------------");
    
    runYTDLPOptions(url, outputPath, audioOnly, quality);
}

void MainWindow::runYTDLPOptions(const QString& url, const QString& outputPath, bool audioOnly, const QString& quality)
{
    QString ytdlpExecutable = "yt-dlp.exe";
    
    if (!QFile::exists(ytdlpExecutable)) {
        ui->textYtdlpOutput->append("Error: yt-dlp.exe not found in current directory");
        ui->textYtdlpOutput->append("Current working directory: " + QDir::currentPath());
        ui->textYtdlpOutput->append("Looking for: " + ytdlpExecutable);
        return;
    }
    
    QStringList arguments;
    arguments << "--newline";
    arguments << "--no-check-certificate";
    
    if (audioOnly) {
        arguments << "-x";
        arguments << "--audio-format" << "mp3";
        arguments << "--audio-quality" << "0";
    } else {
        if (quality != "best") {
            QString qualityNum = quality;
            qualityNum.replace("p", "");
            arguments << "-f" << QString("bestvideo[height<=%1]+bestaudio/best[height<=%1]").arg(qualityNum);
        }
    }
    
    arguments << "-o" << (outputPath + "/%(title)s.%(ext)s");
    arguments << url;
    
    m_ytdlpProcess->setWorkingDirectory(outputPath);
    
    ui->btnDownloadYtdlp->setEnabled(false);
    ui->btnStopYtdlp->setEnabled(true);
    ui->progressYtdlp->setVisible(true);
    ui->progressYtdlp->setValue(0);
    
    ui->textYtdlpOutput->append(QString("Running: %1 %2").arg(ytdlpExecutable, arguments.join(" ")));
    
    m_ytdlpProcess->start(ytdlpExecutable, arguments);
    
    if (!m_ytdlpProcess->waitForStarted(5000)) {
        ui->textYtdlpOutput->append(QString("Failed to start yt-dlp: %1").arg(m_ytdlpProcess->errorString()));
        ui->btnDownloadYtdlp->setEnabled(true);
        ui->btnStopYtdlp->setEnabled(false);
        ui->progressYtdlp->setVisible(false);
    }
}



void MainWindow::on_btnStopYtdlp_clicked()
{
    if (m_ytdlpProcess->state() == QProcess::Running) {
        m_ytdlpProcess->terminate();
        if (!m_ytdlpProcess->waitForFinished(3000)) {
            m_ytdlpProcess->kill();
            m_ytdlpProcess->waitForFinished(1000);
        }
        ui->textYtdlpOutput->append("Download terminated by user.");
    }
    ui->btnDownloadYtdlp->setEnabled(true);
    ui->btnStopYtdlp->setEnabled(false);
    ui->progressYtdlp->setVisible(false);
}

void MainWindow::on_btnBrowseYtdlpOutput_clicked()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, tr("Select Output Folder"), 
                                                          QDir::homePath());
    if (!folderPath.isEmpty()) {
        ui->lineYtdlpOutput->setText(folderPath);
    }
}

void MainWindow::onYtdlpOutputReady()
{
    QByteArray output = m_ytdlpProcess->readAllStandardOutput();
    QString outputStr = QString::fromLocal8Bit(output);
    if (!outputStr.trimmed().isEmpty()) {
        ui->textYtdlpOutput->append(outputStr.trimmed());
        parseYTDLPOutput(outputStr);
    }
}

void MainWindow::onYtdlpErrorReady()
{
    QByteArray errorOutput = m_ytdlpProcess->readAllStandardError();
    QString errorStr = QString::fromLocal8Bit(errorOutput);
    if (!errorStr.trimmed().isEmpty()) {
        ui->textYtdlpOutput->append(QString("ERROR: %1").arg(errorStr.trimmed()));
    }
}

void MainWindow::onYtdlpFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    
    ui->btnDownloadYtdlp->setEnabled(true);
    ui->btnStopYtdlp->setEnabled(false);
    ui->progressYtdlp->setVisible(false);
    
    if (exitCode == 0) {
        ui->textYtdlpOutput->append(QString("[%1] Download completed successfully!").arg(QTime::currentTime().toString()));
        QMessageBox::information(this, tr("Success"), tr("Download completed successfully!"));
    } else {
        ui->textYtdlpOutput->append(QString("[%1] Download failed with exit code: %2").arg(QTime::currentTime().toString()).arg(exitCode));
        QMessageBox::warning(this, tr("Error"), tr("Download failed! Check the output for details."));
    }
    
    m_ytdlpProcess->setWorkingDirectory(QDir::currentPath());
}

QString MainWindow::parseYTDLPOutput(const QString& output)
{
    QRegularExpression re("\\[download\\]\\s+(\\d+\\.\\d+)%");
    QRegularExpressionMatch match = re.match(output);
    if (match.hasMatch()) {
        QString percentStr = match.captured(1);
        bool ok;
        double percent = percentStr.toDouble(&ok);
        if (ok) {
            ui->progressYtdlp->setValue(static_cast<int>(percent));
            return QString("Progress: %1%").arg(percentStr);
        }
    }
    return "";
}


void MainWindow::onCustomErrorReady()
{
    QByteArray errorOutput = m_customProcess->readAllStandardError();
    QString errorStr = QString::fromLocal8Bit(errorOutput);
    if (!errorStr.trimmed().isEmpty()) {
        ui->textCustomOutput->append(QString("ERROR: %1").arg(errorStr.trimmed()));
    }
}

void MainWindow::onCustomFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    
    ui->btnRunCustomCommand->setEnabled(true);
    ui->btnStopCustomCommand->setEnabled(false);
    
    if (exitCode == 0) {
        ui->textCustomOutput->append(QString("[%1] Command completed successfully!").arg(QTime::currentTime().toString()));
    } else {
        ui->textCustomOutput->append(QString("[%1] Command failed with exit code: %2").arg(QTime::currentTime().toString()).arg(exitCode));
    }
    
    m_customProcess->setWorkingDirectory(QDir::currentPath());
}

void MainWindow::onPlaybackStopped()
{
    m_isMediaPlaying = false;
    updatePlaybackControls();
    addToLog("Media playback stopped", Qt::darkBlue);
}

void MainWindow::onPlaybackPaused()
{
    updatePlaybackControls();
    addToLog("Media playback paused", Qt::darkBlue);
}

void MainWindow::onPlaybackResumed()
{
    updatePlaybackControls();
    addToLog("Media playback resumed", Qt::darkGreen);
}

void MainWindow::onErrorOccurred(const QString& error)
{
    QMessageBox::warning(this, tr("Playback Error"), error);
    addToLog(QString("Playback error: %1").arg(error), Qt::red);
    m_isMediaPlaying = false;
    updatePlaybackControls();
}

void MainWindow::onPositionChanged(int position)
{
    Q_UNUSED(position);
}

void MainWindow::onDurationChanged(int duration)
{
    Q_UNUSED(duration);
}

void MainWindow::updatePlaybackControls()
{

}


void MainWindow::on_btnBrowseInput_clicked()
{
    on_actionOpen_triggered();
}

void MainWindow::on_btnBrowseOutput_clicked()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, tr("Select Output Folder"), 
                                                          QDir::homePath());
    if (!folderPath.isEmpty()) {
        m_outputFolder = folderPath;
        addToLog(QString("Output folder set to: %1").arg(folderPath));
        
        updateQueueWithNewOutputFolder();
    }
}

void MainWindow::on_btnBrowseDVDDevice_clicked()
{
    QString device = QFileDialog::getExistingDirectory(this, tr("Select DVD Device"), 
                                                      "/dev");
    if (!device.isEmpty()) {
        ui->lineDVDDevice->setText(device);
    }
}

void MainWindow::on_btnBrowseDVDFolder_clicked()
{
    QString folder = QFileDialog::getExistingDirectory(this, tr("Select Output Folder"), 
                                                      QDir::homePath());
    if (!folder.isEmpty()) {
        ui->lineDVDFolder->setText(folder);
    }
}

void MainWindow::on_btnDVDReadStart_clicked()
{
    QString device = ui->lineDVDDevice->text();
    QString outputFolder = ui->lineDVDFolder->text();
    
    if (device.isEmpty() || outputFolder.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please specify DVD device and output folder."));
        return;
    }
    
    if (!QFile::exists(device)) {
        QMessageBox::warning(this, tr("Warning"), tr("DVD device not found."));
        return;
    }
    
    enableControls(false);
    ui->progressBar->setVisible(true);
    ui->progressBar->setValue(0);
    
    addToLog(QString("[%1] Starting DVD read...").arg(QTime::currentTime().toString()));
    
    QStringList args;
    args << "ffmpeg";
    
    if (ui->checkDVDReadCSS->isChecked()) {
        args << "-css" << "libdvdcss";
    }
    
    args << "-i" << QString("dvd://%1").arg(device);
    
    QString outputBase = outputFolder + "/dvd_output";
    if (ui->checkDVDReadMain->isChecked()) {
        args << "-title" << "1";
        args << outputBase + "_main.iso";
    } else {
        args << outputBase + "_full.iso";
    }
    
    args << "-y";
    
    QString command = args.join(" ");
    m_lastCommand = command;
    
    ui->textDetailedLog->append("=== DVD Read Command ===");
    ui->textDetailedLog->append(command);
    ui->textDetailedLog->append("========================\n");
    
    m_ffmpegProcessor->start(command);
}

void MainWindow::on_btnBrowseDVDVideoFiles_clicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Select Video Files"), 
                                                      QDir::homePath(),
                                                      tr("Video Files (*.mp4 *.mkv *.avi *.mov *.m4v *.mpg *.mpeg);;All Files (*)"));
    if (!files.isEmpty()) {
        ui->lineDVDVideoFiles->setText(files.join(";"));
    }
}

void MainWindow::on_btnBrowseDVDDestination_clicked()
{
    QString device = QFileDialog::getExistingDirectory(this, tr("Select DVD Writer"), 
                                                      "/dev");
    if (!device.isEmpty()) {
        ui->lineDVDDestination->setText(device);
    }
}

void MainWindow::setupAudioFilterControls()
{
    m_audioNormalizeEnabled = false;
    m_audioEqEnabled = false;
    m_audioCompressEnabled = false;
    m_audioDenoiseEnabled = false;
    
    connect(ui->checkAudioNormalize, &QCheckBox::stateChanged, 
            this, &MainWindow::on_checkAudioNormalize_stateChanged);
    connect(ui->checkAudioEq, &QCheckBox::stateChanged, 
            this, &MainWindow::on_checkAudioEq_stateChanged);
    connect(ui->checkAudioCompress, &QCheckBox::stateChanged, 
            this, &MainWindow::on_checkAudioCompress_stateChanged);
    connect(ui->checkAudioDenoise, &QCheckBox::stateChanged, 
            this, &MainWindow::on_checkAudioDenoise_stateChanged);
    
    ui->widgetAudioEQ->setVisible(false);
    ui->widgetAudioCompress->setVisible(false);
    
    ui->spinLowFreq->setValue(100);
    ui->spinHighFreq->setValue(8000);
    ui->spinLowGain->setValue(0.0);
    ui->spinMidGain->setValue(0.0);
    ui->spinHighGain->setValue(0.0);
    ui->spinThreshold->setValue(-20.0);
    ui->spinRatio->setValue(4.0);
    ui->spinAttack->setValue(5.0);
    ui->spinRelease->setValue(50.0);
}

void MainWindow::on_checkAudioNormalize_stateChanged(int state)
{
    m_audioNormalizeEnabled = (state == Qt::Checked);
}

void MainWindow::on_checkAudioEq_stateChanged(int state)
{
    m_audioEqEnabled = (state == Qt::Checked);
    ui->widgetAudioEQ->setVisible(m_audioEqEnabled);
}

void MainWindow::on_checkAudioCompress_stateChanged(int state)
{
    m_audioCompressEnabled = (state == Qt::Checked);
    ui->widgetAudioCompress->setVisible(m_audioCompressEnabled);
}

void MainWindow::on_checkAudioDenoise_stateChanged(int state)
{
    m_audioDenoiseEnabled = (state == Qt::Checked);
}

QString MainWindow::buildAudioFilterChain() const
{
    QStringList filters;
    
    if (m_audioDenoiseEnabled) {
        filters << "afftdn=nf=-25:rf=30:tn=1";
    }
    
    if (m_audioEqEnabled) {
        double lowGain = ui->spinLowGain->value();
        double midGain = ui->spinMidGain->value();
        double highGain = ui->spinHighGain->value();
        int lowFreq = ui->spinLowFreq->value();
        int highFreq = ui->spinHighFreq->value();
        
        QStringList eqFilters;
        if (lowGain != 0.0) {
            eqFilters << QString("equalizer=f=%1:t=h:width_type=h:w=100:g=%2")
                         .arg(lowFreq).arg(lowGain);
        }
        if (midGain != 0.0) {
            eqFilters << QString("equalizer=f=%1:t=h:width_type=h:w=1000:g=%2")
                         .arg((lowFreq + highFreq) / 2).arg(midGain);
        }
        if (highGain != 0.0) {
            eqFilters << QString("equalizer=f=%1:t=h:width_type=h:w=2000:g=%2")
                         .arg(highFreq).arg(highGain);
        }
        
        if (!eqFilters.isEmpty()) {
            filters << eqFilters.join(",");
        }
    }
    
    if (m_audioCompressEnabled) {
        double threshold = ui->spinThreshold->value();
        double ratio = ui->spinRatio->value();
        double attack = ui->spinAttack->value();
        double release = ui->spinRelease->value();
        
        filters << QString("acompressor=threshold=%1dB:ratio=%2:attack=%3:release=%4")
                   .arg(threshold).arg(ratio).arg(attack).arg(release);
    }
    
    if (m_audioNormalizeEnabled) {
        filters << "loudnorm=I=-16:TP=-1.5:LRA=11";
    }
    
    if (!filters.isEmpty()) {
        return " -af \"" + filters.join(",") + "\"";
    }
    
    return "";
}
QString MainWindow::buildFFmpegCommand(const QString& inputFile, const QString& outputFile)
{
    QStringList args;
    
    args << "-i" << QString("\"%1\"").arg(inputFile);
    
    bool audioOnly = isAudioOnlyFile(inputFile);
    QFileInfo outputInfo(outputFile);
    QString outputExt = outputInfo.suffix().toLower();
    
    QStringList audioExtensions = {"mp3", "m4a", "wav", "flac", "ogg", "wma", "opus", "alac", "aac", "mka"};
    bool isAudioOutput = audioExtensions.contains(outputExt);
    
    if (audioOnly || isAudioOutput) {
        args << "-vn";
        
        QString audioCodec = ui->comboAudioCodec->currentText();
        args << "-c:a" << audioCodec;
        args << "-b:a" << QString("%1k").arg(ui->spinAudioBitrate->value());
        
        QString audioFilters = buildAudioFilterChain();
        if (!audioFilters.isEmpty()) {
            QStringList filterArgs = audioFilters.split(" ", Qt::SkipEmptyParts);
            for (const QString& arg : filterArgs) {
                if (!arg.isEmpty() && arg != "-af") {
                    args << arg;
                }
            }
        }
                
        if (audioCodec == "copy") {
            args << "-c:a" << "copy";
        }
    } else {
        QString hwAccel = ui->comboHwAccel->currentText();
        if (hwAccel != "None") {
            args << "-hwaccel" << hwAccel.toLower();
        }
        
        args << "-c:v" << ui->comboVideoCodec->currentText();
        
        QString quality = ui->comboQuality->currentText().toLower().replace(" ", "");
        args << "-preset" << quality;
        
        if (ui->radioCRF->isChecked()) {
            args << "-crf" << QString::number(ui->spinCRF->value());
        } else {
            args << "-b:v" << QString("%1k").arg(ui->spinBitrate->value());
        }
        
        QString resolution = ui->comboResolution->currentText();
        if (resolution != "Original") {
            args << "-s" << resolution;
        }
        
        QString framerate = ui->comboFramerate->currentText();
        if (framerate != "Original") {
            args << "-r" << framerate;
        }
        
        args << "-c:a" << ui->comboAudioCodec->currentText();
        args << "-b:a" << QString("%1k").arg(ui->spinAudioBitrate->value());
        
        QString audioFilters = buildAudioFilterChain();
        if (!audioFilters.isEmpty()) {
            args << "-af" << audioFilters.trimmed().replace("\"", "");
        }
    }
    
    args << QString("\"%1\"").arg(outputFile);
    
    if (!ui->overwriteBox->isChecked()) { 
    } else {
        args << "-y";
    }
    
    QString command = "ffmpeg " + args.join(" ");
    return command;
}


void MainWindow::setupImageTools()
{
    QStringList imageFormats = {
        "jpg", "jpeg", "png", "bmp", "tiff", "tif", "webp", "gif", "ico",
        "jp2", "j2k", "pbm", "pgm", "ppm", "pnm", "pcx", "tga", "sgi", 
        "exr", "hdr", "rgba", "rgb", "xbm", "xpm", "dds", "pdf"
    };
    ui->comboImageFormat->addItems(imageFormats);
    ui->comboImageFormat->setCurrentText("jpg");
    
    connect(ui->comboImageFormat, &QComboBox::currentTextChanged,
            this, &MainWindow::on_comboImageFormat_currentTextChanged);
    
    ui->comboResizeMethod->addItem("lanczos", "Lanczos resampling - highest quality");
    ui->comboResizeMethod->addItem("bicubic", "Bicubic - good quality");
    ui->comboResizeMethod->addItem("bilinear", "Bilinear - fast");
    ui->comboResizeMethod->addItem("neighbor", "Nearest neighbor - fastest");
    ui->comboResizeMethod->addItem("sinc", "Sinc - high quality for downscaling");
    ui->comboResizeMethod->addItem("gauss", "Gaussian - smooth");
    ui->comboResizeMethod->setCurrentIndex(0);
    
    connect(ui->sliderBrightness, &QSlider::valueChanged, 
            this, &MainWindow::on_sliderBrightness_valueChanged);
    connect(ui->sliderContrast, &QSlider::valueChanged, 
            this, &MainWindow::on_sliderContrast_valueChanged);
    connect(ui->sliderImageQuality, &QSlider::valueChanged, 
            this, &MainWindow::on_sliderImageQuality_valueChanged);
    
    ui->sliderBrightness->setValue(0);
    ui->sliderContrast->setValue(0);
    ui->sliderImageQuality->setValue(90);
    
    ui->sliderImageQuality->setMinimum(1);
    ui->sliderImageQuality->setMaximum(100);
    
    ui->spinWidth->setValue(1920);
    ui->spinHeight->setValue(1080);
    ui->spinWidth->setMaximum(10000);
    ui->spinHeight->setMaximum(10000);
}
void MainWindow::on_btnBrowseProbe_clicked()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select Files to Probe"), 
                                                         QDir::homePath(),
                                                         tr("All Files (*.*)"));
    if (!fileNames.isEmpty()) {
        ui->lineProbeInput->setText(fileNames.join("; "));
    }
}

void MainWindow::on_btnRunProbe_clicked()
{
    QString filesText = ui->lineProbeInput->text();
    if (filesText.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please select files to probe."));
        return;
    }
    
    QStringList files = filesText.split("; ", Qt::SkipEmptyParts);
    if (files.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please select valid files to probe."));
        return;
    }
    
    for (const QString& file : files) {
        if (!QFile::exists(file.trimmed())) {
            QMessageBox::warning(this, tr("Warning"), 
                               tr("File does not exist: %1").arg(file.trimmed()));
            return;
        }
    }
    
    runFFprobe(files);
}

void MainWindow::runFFprobe(const QStringList& files)
{
    ui->textProbeOutput->clear();
    ui->textProbeOutput->append("Running FFprobe on selected files...\n");
    ui->textProbeOutput->append("===============================\n");
    
    QStringList arguments;
    arguments << "-v" << "quiet";
    arguments << "-print_format" << "json";
    arguments << "-show_format";
    arguments << "-show_streams";
    arguments << "-show_error";
    
    for (const QString& file : files) {
        QStringList fileArgs = arguments;
        fileArgs << file.trimmed();
        
        ui->textProbeOutput->append(QString("Probing: %1").arg(file.trimmed()));
        ui->textProbeOutput->append("-------------------");
        
        m_probeProcess->start("ffprobe", fileArgs);
        
        if (m_probeProcess->waitForFinished(10000)) { 
        } else {
            ui->textProbeOutput->append(QString("Timeout or error probing: %1\n").arg(file.trimmed()));
        }
    }
    
    ui->textProbeOutput->append("===============================\n");
    ui->textProbeOutput->append("FFprobe analysis completed.\n");
}

void MainWindow::onProbeOutputReady()
{
    QByteArray output = m_probeProcess->readAllStandardOutput();
    QString outputStr = QString::fromLocal8Bit(output);
    if (!outputStr.trimmed().isEmpty()) {
        ui->textProbeOutput->append(outputStr);
    }
}

void MainWindow::onProbeErrorReady()
{
    QByteArray errorOutput = m_probeProcess->readAllStandardError();
    QString errorStr = QString::fromLocal8Bit(errorOutput);
    if (!errorStr.trimmed().isEmpty()) {
        ui->textProbeOutput->append(QString("Error: %1").arg(errorStr));
    }
}

void MainWindow::onProbeFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
}

void MainWindow::on_btnCopyProbeOutput_clicked()
{
    QString output = ui->textProbeOutput->toPlainText();
    if (!output.isEmpty()) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(output);
        addToLog("FFprobe output copied to clipboard.", Qt::darkGreen);
    } else {
        addToLog("No FFprobe output to copy.", Qt::red);
    }
}

void MainWindow::on_btnClearProbeOutput_clicked()
{
    ui->textProbeOutput->clear();
}

void MainWindow::on_comboImageFormat_currentTextChanged(const QString &text)
{
    QString currentOutput = ui->lineImageOutput->text();
    if (!currentOutput.isEmpty()) {
        QFileInfo fileInfo(currentOutput);
        QString basePath = fileInfo.path() + "/" + fileInfo.completeBaseName();
        QString newOutput = basePath + "." + text;
        ui->lineImageOutput->setText(newOutput);
    }
}

void MainWindow::on_btnBrowseImageInput_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), 
                                                   QDir::homePath(),
                                                   tr("Images (*.png *.jpg *.jpeg *.bmp *.tiff *.tif *.webp *.gif *.ico *.jp2 *.j2k *.pbm *.pgm *.ppm *.pnm *.pcx *.tga *.sgi *.exr *.hdr *.rgba *.rgb *.xbm *.xpm *.dds);;All Files (*)"));
    if (!fileName.isEmpty()) {
        ui->lineImageInput->setText(fileName);
        m_imageInputFile = fileName;
        
        QFileInfo fileInfo(fileName);
        QString baseName = fileInfo.completeBaseName();
        QString outputExtension = ui->comboImageFormat->currentText();
        ui->lineImageOutput->setText(baseName + "_converted." + outputExtension);
        
        addToLog(QString("Loaded image: %1").arg(fileName));
    }
}

void MainWindow::on_btnBrowseImageOutput_clicked()
{
    QString currentFormat = ui->comboImageFormat->currentText();
    QString filter = QString("Images (*.%1);;All Files (*)").arg(currentFormat);
    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"), 
                                                   QDir::homePath(),
                                                   filter);
    if (!fileName.isEmpty()) {
        ui->lineImageOutput->setText(fileName);
        m_imageOutputFile = fileName;
    }
}

void MainWindow::on_sliderBrightness_valueChanged(int value)
{
    ui->labelBrightnessValue->setText(QString::number(value));
}

void MainWindow::on_sliderContrast_valueChanged(int value)
{
    ui->labelContrastValue->setText(QString::number(value));
}

void MainWindow::on_sliderImageQuality_valueChanged(int value)
{
    ui->labelQualityNumber->setText(QString::number(value));
}

QString MainWindow::getImageFormatFromFilename(const QString& filename)
{
    QFileInfo fileInfo(filename);
    return fileInfo.suffix().toLower();
}

QString MainWindow::buildImageCommand(const QString& inputFile, const QString& outputFile)
{
    QStringList args;
    
    args << "-i" << QString("\"%1\"").arg(inputFile);
    
    QString outputFormat = getImageFormatFromFilename(outputFile);
    if (outputFormat.isEmpty()) {
        outputFormat = ui->comboImageFormat->currentText();
    }
    
    if (!ui->checkPreserveMetadata->isChecked()) {
        args << "-map_metadata" << "-1";
    }
    
    QStringList filters;
    
    int width = ui->spinWidth->value();
    int height = ui->spinHeight->value();
    
    if (width > 0 && height > 0) {
        QString resizeMethod = ui->comboResizeMethod->currentText();
        QString resizeMode = ui->checkKeepAspect->isChecked() ? 
                            "force_original_aspect_ratio=decrease" : "";
        
        QString scaleFilter = QString("scale=%1:%2").arg(width).arg(height);
        if (!resizeMode.isEmpty()) {
            scaleFilter += ":" + resizeMode;
        }
        if (!resizeMethod.isEmpty()) {
            scaleFilter += ":flags=" + resizeMethod;
        }
        
        filters << scaleFilter;
    }
    
    if (outputFormat == "jpg" || outputFormat == "jpeg") {
        filters << "format=yuv420p";
    } else if (outputFormat == "png") {
        filters << "format=rgba";
    } else if (outputFormat == "webp") {
        filters << "format=rgba";
    }
    
    if (ui->checkGrayscale->isChecked()) {
        filters << "hue=s=0"; 
    }
    
    if (ui->checkSepia->isChecked()) {
        filters << "colorchannelmixer=.393:.769:.189:.349:.686:.168:.272:.534:.131";
    }
    
    if (ui->checkNegative->isChecked()) {
        filters << "negate";
    }
    
    if (ui->checkBlur->isChecked()) {
        filters << "boxblur=2:1";
    }
    
    if (ui->checkSharpen->isChecked()) {
        filters << "unsharp=5:5:1.0:5:5:0.0";
    }
    
    if (ui->checkEdgeDetect->isChecked()) {
        filters << "convolution=-1 -1 -1 -1 8 -1 -1 -1 -1";
    }
    
    if (ui->checkVignette->isChecked()) {
        filters << "vignette=PI/4";
    }
    
    int brightness = ui->sliderBrightness->value();
    int contrast = ui->sliderContrast->value();
    if (brightness != 0 || contrast != 0) {
        double b = brightness / 100.0;
        double c = (contrast / 100.0) + 1.0;
        filters << QString("eq=brightness=%1:contrast=%2").arg(b).arg(c);
    }
    
    if (!filters.isEmpty()) {
        args << "-vf" << filters.join(",");
    }
    
    if (outputFormat == "jpg" || outputFormat == "jpeg") {
        int jpegQuality = qBound(1, 32 - (ui->sliderImageQuality->value() / 3), 32);
        args << "-q:v" << QString::number(jpegQuality);
        args << "-pix_fmt" << "yuv420p";
    } 
    else if (outputFormat == "png") {
        int pngCompression = qBound(0, (100 - ui->sliderImageQuality->value()) / 10, 9);
        args << "-compression_level" << QString::number(pngCompression);
        args << "-pix_fmt" << "rgba";
    } 
    else if (outputFormat == "webp") {
        args << "-quality" << QString::number(ui->sliderImageQuality->value());
        args << "-pix_fmt" << "yuva420p";
    } 
    else if (outputFormat == "tiff") {
        args << "-compression_algo" << "lzw";
    } 
    else if (outputFormat == "bmp") {
        args << "-pix_fmt" << "bgr24";
    } 
    else if (outputFormat == "gif") {
        args << "-pix_fmt" << "pal8";
    }
    
    if (ui->checkProgressive->isChecked()) {
        if (outputFormat == "jpg" || outputFormat == "jpeg") {
            args << "-interlace" << "plane";
        }
    }
    
    args << QString("\"%1\"").arg(outputFile);
    
    args << "-y";
    
    args << "-vsync" << "0";
    args << "-an";
    
    QString command = "ffmpeg " + args.join(" ");
    return command;
}


void MainWindow::on_btnImageStart_clicked()
{
    QString inputFile = ui->lineImageInput->text();
    QString outputFile = ui->lineImageOutput->text();
    
    if (inputFile.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please specify input file."));
        return;
    }
    
    if (outputFile.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please specify output file."));
        return;
    }
    
    if (!QFile::exists(inputFile)) {
        QMessageBox::warning(this, tr("Warning"), tr("Input file does not exist."));
        return;
    }
    
    QFileInfo outputInfo(outputFile);
    QDir outputDir = outputInfo.dir();
    if (!outputDir.exists()) {
        if (!outputDir.mkpath(".")) {
            QMessageBox::warning(this, tr("Warning"), tr("Cannot create output directory."));
            return;
        }
    }
    
    enableControls(false);
    ui->progressBar->setVisible(true);
    ui->progressBar->setValue(0);
    
    addToLog(QString("[%1] Starting image conversion...").arg(QTime::currentTime().toString()));
    addToLog(QString("Input: %1").arg(inputFile));
    addToLog(QString("Output: %1").arg(outputFile));
    
    QString command = buildImageCommand(inputFile, outputFile);
    m_lastCommand = command;
    
    ui->textDetailedLog->append("=== Image Conversion Command ===");
    ui->textDetailedLog->append(command);
    ui->textDetailedLog->append("================================\n");
    
    m_ffmpegProcessor->start(command);
}

void MainWindow::on_btnImageStop_clicked()
{
    if (m_ffmpegProcessor && m_ffmpegProcessor->isRunning()) {
        m_ffmpegProcessor->stop();
        addToLog(tr("Image processing stopped by user."), Qt::red);
    }
    enableControls(true);
    ui->progressBar->setVisible(false);
}

void MainWindow::on_btnImagePreview_clicked()
{
    QString inputFile = ui->lineImageInput->text();
    if (inputFile.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please load an image first."));
        return;
    }
    

    QString previewInfo = "Preview Settings:\n";
    previewInfo += QString("Format: %1\n").arg(ui->comboImageFormat->currentText());
    previewInfo += QString("Size: %1x%2\n").arg(ui->spinWidth->value()).arg(ui->spinHeight->value());
    
    QStringList activeFilters;
    if (ui->checkGrayscale->isChecked()) activeFilters << "Grayscale";
    if (ui->checkSepia->isChecked()) activeFilters << "Sepia";
    if (ui->checkNegative->isChecked()) activeFilters << "Negative";
    if (ui->checkBlur->isChecked()) activeFilters << "Blur";
    if (ui->checkSharpen->isChecked()) activeFilters << "Sharpen";
    if (ui->checkEdgeDetect->isChecked()) activeFilters << "Edge Detection";
    
    if (!activeFilters.isEmpty()) {
        previewInfo += "Filters: " + activeFilters.join(", ") + "\n";
    }
    
    previewInfo += QString("Brightness: %1\n").arg(ui->sliderBrightness->value());
    previewInfo += QString("Contrast: %1\n").arg(ui->sliderContrast->value());
    previewInfo += QString("Quality: %1\n").arg(ui->sliderImageQuality->value());
    
    QMessageBox::information(this, tr("Image Preview"), previewInfo);
}

bool MainWindow::isImageFile(const QString& filePath)
{
    QStringList imageExtensions = {
        "png", "jpg", "jpeg", "bmp", "tiff", "tif", "webp", "gif", "ico",
        "jp2", "j2k", "pbm", "pgm", "ppm", "pnm", "pcx", "tga", "sgi", 
        "exr", "hdr", "rgba", "rgb", "xbm", "xpm", "dds"
    };
    QFileInfo fileInfo(filePath);
    return imageExtensions.contains(fileInfo.suffix().toLower());
}


void MainWindow::on_btnDVDWriteStart_clicked()
{
    QString videoFiles = ui->lineDVDVideoFiles->text();
    QString destination = ui->lineDVDDestination->text();
    
    if (videoFiles.isEmpty() || destination.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please specify video files and DVD writer."));
        return;
    }
    
    enableControls(false);
    ui->progressBar->setVisible(true);
    ui->progressBar->setValue(0);
    
    addToLog(QString("[%1] Starting DVD write...").arg(QTime::currentTime().toString()));
    
    QStringList args;
    args << "ffmpeg";
    
    QStringList fileList = videoFiles.split(";");
    for (const QString& file : fileList) {
        if (QFile::exists(file)) {
            args << "-i" << file;
        }
    }
    
    args << "-target" << "dvd";
    
    if (ui->checkDVDWriteMenu->isChecked()) {
        args << "-dvd-menu";
    }
    
    QString speed = ui->comboDVDWriteSpeed->currentText();
    if (speed != "Auto") {
        args << "-speed" << speed.replace("x", "");
    }
    
    args << "-o" << destination;
    args << "-y";
    
    QString command = args.join(" ");
    m_lastCommand = command;
    
    ui->textDetailedLog->append("=== DVD Write Command ===");
    ui->textDetailedLog->append(command);
    ui->textDetailedLog->append("=========================\n");
    
    m_ffmpegProcessor->start(command);
}

void MainWindow::on_btnBrowseDVDCopySource_clicked()
{
    QString device = QFileDialog::getExistingDirectory(this, tr("Select Source DVD"), 
                                                      "/dev");
    if (!device.isEmpty()) {
        ui->lineDVDCopySource->setText(device);
    }
}


void MainWindow::on_btnDVDCopyStart_clicked()
{
    QString source = ui->lineDVDCopySource->text();
    QString dest = ui->lineDVDCopyDest->text();
    
    if (source.isEmpty() || dest.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please specify source DVD drive and destination file."));
        return;
    }
    
#ifdef Q_OS_WIN
    if (!source.contains(":") || source.length() > 3) {
        QMessageBox::warning(this, tr("Warning"), tr("Please specify a valid drive letter (e.g., F:)"));
        return;
    }
#else
    if (!source.startsWith("/") || source.length() < 2) {
        QMessageBox::warning(this, tr("Warning"), tr("Please specify a valid source path (e.g., /mnt/dvd)"));
        return;
    }
#endif
    
    QFileInfo destInfo(dest);
    QDir destDir = destInfo.dir();
    if (!destDir.exists()) {
        QMessageBox::warning(this, tr("Warning"), tr("Destination folder does not exist."));
        return;
    }
    
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Confirm DVD Copy"), 
                                 tr("This will copy the DVD to the specified file. Continue?"),
                                 QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) {
        return;
    }
    
    enableControls(false);
    ui->progressBar->setVisible(true);
    ui->progressBar->setValue(0);
    
    addToLog(QString("[%1] Starting DVD copy...").arg(QTime::currentTime().toString()));
    
    QStringList args;
    args << "-f" << "dvdvideo";
    args << "-i" << source;
    args << "-c" << "copy";
    args << dest;
    args << "-y";
    
    QString command = "ffmpeg " + args.join(" ");
    m_lastCommand = command;
    
    ui->textDetailedLog->append("=== DVD Copy Command ===");
    ui->textDetailedLog->append(command);
    ui->textDetailedLog->append("========================\n");
    
    m_ffmpegProcessor->start(command);
}


void MainWindow::on_btnBrowseDVDCopyDest_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select Output File"), 
                                                   QDir::homePath(),
                                                   tr("ISO Files (*.iso);;MKV Files (*.mkv);;MP4 Files (*.mp4);;All Files (*.*)"));
    if (!fileName.isEmpty()) {
        ui->lineDVDCopyDest->setText(fileName);
    }
}

void MainWindow::updateQueueWithNewOutputFolder()
{
    for (int row = 0; row < ui->tableQueue->rowCount(); ++row) {
        QString inputFile = ui->tableQueue->item(row, 0)->text();
        QFileInfo inputInfo(inputFile);
        QString outputExtension = ui->comboContainer->currentText();
        QString outputFileName = inputInfo.completeBaseName() + "." + outputExtension;
        QString outputFile = m_outputFolder + "/" + outputFileName;
        
        ui->tableQueue->item(row, 1)->setText(outputFile);
    }
}


void MainWindow::on_btnStart_clicked()
{
    QString inputFile = ui->lineInputFile->text();
    
    if (inputFile.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please specify input file(s)."));
        return;
    }
    
    if (inputFile.contains("files selected")) {
        if (m_outputFolder.isEmpty()) {
            QMessageBox::warning(this, tr("Warning"), tr("Please select an output folder."));
            return;
        }
        
        if (ui->tableQueue->rowCount() == 0) {
            QMessageBox::warning(this, tr("Warning"), tr("No files in queue."));
            return;
        }
        
        updateQueueWithNewOutputFolder();
        
        m_isProcessingBatch = true;
        m_currentBatchIndex = 0;
        processNextBatchItem();
    } else {
        QString outputFileName = ui->lineOutputFile->text();
        
        if (outputFileName.isEmpty()) {
            QMessageBox::warning(this, tr("Warning"), tr("Please specify output file name."));
            return;
        }
        
        if (!QFile::exists(inputFile)) {
            QMessageBox::warning(this, tr("Warning"), tr("Input file does not exist."));
            return;
        }
        
        if (m_outputFolder.isEmpty()) {
            QFileInfo inputInfo(inputFile);
            m_outputFolder = inputInfo.dir().absolutePath();
        }
        
        QString outputExtension = ui->comboContainer->currentText();
        QString outputFile = m_outputFolder + "/" + outputFileName + "." + outputExtension;
        
        enableControls(false);
        ui->progressBar->setVisible(true);
        ui->progressBar->setValue(0);
        
        addToLog(QString("[%1] Starting conversion...").arg(QTime::currentTime().toString()));
        
        QString command = buildFFmpegCommand(inputFile, outputFile);
        m_lastCommand = command;
        
        ui->textDetailedLog->clear();
        ui->textDetailedLog->append("=== FFmpeg Command ===");
        ui->textDetailedLog->append(command);
        ui->textDetailedLog->append("=====================\n");
        
        m_ffmpegProcessor->start(command);
    }
}

void MainWindow::processNextBatchItem()
{
    addToLog(QString("Processing batch item %1 of %2").arg(m_currentBatchIndex + 1).arg(ui->tableQueue->rowCount()));
    
    if (m_currentBatchIndex >= ui->tableQueue->rowCount()) {
        m_isProcessingBatch = false;
        addToLog(QString("Batch conversion completed! Processed %1 items.").arg(m_currentBatchIndex));
        QMessageBox::information(this, tr("Success"), tr("Batch conversion completed!"));
        enableControls(true);
        return;
    }
    
    QString inputPath = ui->tableQueue->item(m_currentBatchIndex, 0)->text();
    QString outputPath = ui->tableQueue->item(m_currentBatchIndex, 1)->text();
    
    if (!QFile::exists(inputPath)) {
        ui->tableQueue->item(m_currentBatchIndex, 2)->setText("File Not Found");
        addToLog(QString("File not found: %1").arg(inputPath), Qt::red);
        m_currentBatchIndex++;
        QTimer::singleShot(10, this, &MainWindow::processNextBatchItem);
        return;
    }
    
    ui->tableQueue->item(m_currentBatchIndex, 2)->setText("Processing...");
    
    QString command = buildFFmpegCommand(inputPath, outputPath);
    m_lastCommand = command;
    
    ui->textDetailedLog->append(QString("\n=== Processing Item %1/%2 ===").arg(m_currentBatchIndex + 1).arg(ui->tableQueue->rowCount()));
    ui->textDetailedLog->append(command);
    ui->textDetailedLog->append("===============================\n");
    
    addToLog(QString("Converting (%1/%2): %3 to %4").arg(m_currentBatchIndex + 1).arg(ui->tableQueue->rowCount()).arg(inputPath).arg(outputPath));
    
    m_ffmpegProcessor->start(command);
}



void MainWindow::on_btnStop_clicked()
{
    if (m_ffmpegProcessor && m_ffmpegProcessor->isRunning()) {
        m_ffmpegProcessor->stop();
        addToLog(tr("Process stopped by user."), Qt::red);
    }
    enableControls(true);
    ui->progressBar->setVisible(false);
    m_isProcessingBatch = false;
    m_currentBatchIndex = 0;
}

void MainWindow::on_btnLoadPreset_clicked()
{
    QString presetName = ui->comboPresets->currentText();
    if (presetName == "Custom") return;
    
    auto preset = m_presetManager->loadPreset(presetName);
    if (!preset.isEmpty()) {
        if (preset.contains("videoCodec")) 
            ui->comboVideoCodec->setCurrentText(preset["videoCodec"].toString());
        if (preset.contains("audioCodec")) 
            ui->comboAudioCodec->setCurrentText(preset["audioCodec"].toString());
        if (preset.contains("container")) 
            ui->comboContainer->setCurrentText(preset["container"].toString());
        if (preset.contains("crf")) 
            ui->spinCRF->setValue(preset["crf"].toInt());
        if (preset.contains("bitrate")) 
            ui->spinBitrate->setValue(preset["bitrate"].toInt());
        if (preset.contains("resolution")) 
            ui->comboResolution->setCurrentText(preset["resolution"].toString());
        if (preset.contains("framerate")) 
            ui->comboFramerate->setCurrentText(preset["framerate"].toString());
        if (preset.contains("quality")) 
            ui->comboQuality->setCurrentText(preset["quality"].toString());
            
        addToLog(QString("Loaded preset: %1").arg(presetName));
        
        updateQueueWithNewOutputFolder();
    }
}

void MainWindow::on_btnSavePreset_clicked()
{
    bool ok;
    QString presetName = QInputDialog::getText(this, tr("Save Preset"), 
                                              tr("Enter preset name:"), 
                                              QLineEdit::Normal, 
                                              "", &ok);
    if (ok && !presetName.isEmpty()) {
        QVariantMap preset;
        preset["videoCodec"] = ui->comboVideoCodec->currentText();
        preset["audioCodec"] = ui->comboAudioCodec->currentText();
        preset["container"] = ui->comboContainer->currentText();
        preset["crf"] = ui->spinCRF->value();
        preset["bitrate"] = ui->spinBitrate->value();
        preset["resolution"] = ui->comboResolution->currentText();
        preset["framerate"] = ui->comboFramerate->currentText();
        preset["quality"] = ui->comboQuality->currentText();
        
        if (m_presetManager->savePreset(presetName, preset)) {
            ui->comboPresets->addItem(presetName);
            addToLog(QString("Saved preset: %1").arg(presetName));
        } else {
            QMessageBox::warning(this, tr("Warning"), tr("Failed to save preset."));
        }
    }
}

void MainWindow::on_btnShowDetails_toggled(bool checked)
{
    ui->detailsPanel->setVisible(checked);
    ui->btnShowDetails->setText(checked ? "Hide Details" : "Show Details");
}

void MainWindow::on_btnCopyCommand_clicked()
{
    if (!m_lastCommand.isEmpty()) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(m_lastCommand);
        addToLog("Command copied to clipboard.", Qt::darkGreen);
    } else {
        addToLog("No command available to copy.", Qt::red);
    }
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    Q_UNUSED(index)
}

bool MainWindow::isAudioOnlyFile(const QString& filePath)
{
    QStringList audioExtensions = {"mp3", "wav", "flac", "aac", "ogg", "wma", "m4a", "opus", "ape", "alac", "mka"};
    QFileInfo fileInfo(filePath);
    return audioExtensions.contains(fileInfo.suffix().toLower());
}



void MainWindow::updateProgress(qint64 bytesProcessed, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int percent = static_cast<int>((bytesProcessed * 100) / bytesTotal);
        ui->progressBar->setValue(percent);
    }
}

void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    ui->progressBar->setVisible(false);
    bool isCustomProcessing = (ui->tabWidget->currentIndex() == 4);
    
    if (isCustomProcessing) {
        return;
    }
    bool isImageProcessing = (ui->tabWidget->currentIndex() == 3);
    bool isDVDProcessing = (ui->tabWidget->currentIndex() == 2);
    
    if (isImageProcessing) {
        enableControls(true);
        
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            addToLog(tr("Image processing completed successfully!"), Qt::darkGreen);
            QMessageBox::information(this, tr("Success"), tr("Image processing completed successfully!"));
            
            QString outputFile = ui->lineImageOutput->text();
            if (!outputFile.isEmpty() && QFile::exists(outputFile)) {
                QMessageBox::StandardButton reply = QMessageBox::question(
                    this, tr("Open Result"), 
                    tr("Image processing completed. Open the result file?"),
                    QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(outputFile));
                }
            }
        } else {
            addToLog(QString("Image processing failed! Exit code: %1").arg(exitCode), Qt::red);
            QMessageBox::critical(this, tr("Error"), tr("Image processing failed!"));
        }
    } else if (m_isProcessingBatch) {
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            ui->tableQueue->item(m_currentBatchIndex, 2)->setText("Completed");
            addToLog(QString("Item %1 completed successfully.").arg(m_currentBatchIndex + 1), Qt::darkGreen);
        } else {
            ui->tableQueue->item(m_currentBatchIndex, 2)->setText("Failed");
            addToLog(QString("Item %1 failed! Exit code: %2").arg(m_currentBatchIndex + 1).arg(exitCode), Qt::red);
        }
        
        m_currentBatchIndex++;
        QTimer::singleShot(50, this, &MainWindow::processNextBatchItem);
    } else if (isDVDProcessing) {
        enableControls(true);
        
        bool isDVDCopy = m_lastCommand.contains("-f dvdvideo") && m_lastCommand.contains("-c copy");
        
        if (exitStatus == QProcess::NormalExit) {
            if (exitCode == 0) {
                addToLog(tr("DVD operation completed successfully!"), Qt::darkGreen);
                QMessageBox::information(this, tr("Success"), tr("DVD operation completed successfully!"));
            } else {
                QString detailedLog = ui->textDetailedLog->toPlainText();
                bool hasProgressInfo = detailedLog.contains("frame=") || detailedLog.contains("speed=") || detailedLog.contains("fps=");
                
                if (isDVDCopy && hasProgressInfo) {
                    addToLog(QString("DVD copy completed with warnings (exit code: %1). Check output file.").arg(exitCode), Qt::darkYellow);
                    QMessageBox::warning(this, tr("Completed with Warnings"), 
                                       tr("DVD copy completed with warnings. Please check if the output file was created successfully."));
                } else {
                    addToLog(QString("DVD operation failed! Exit code: %1").arg(exitCode), Qt::red);
                    QMessageBox::critical(this, tr("Error"), 
                                        tr("DVD operation failed! Exit code: %1").arg(exitCode));
                }
            }
        } else {
            addToLog(QString("DVD operation crashed or was terminated abnormally!"), Qt::red);
            QMessageBox::critical(this, tr("Error"), tr("DVD operation crashed or was terminated abnormally!"));
        }
    } else {
        enableControls(true);
        
        if (exitCode == 0) {
            addToLog(tr("Conversion completed successfully!"), Qt::darkGreen);
            QMessageBox::information(this, tr("Success"), tr("Conversion completed successfully!"));
        } else {
            addToLog(QString("Conversion failed! Exit code: %1").arg(exitCode), Qt::red);
            QMessageBox::critical(this, tr("Error"), tr("Conversion failed!"));
        }
    }
}

void MainWindow::processError(QProcess::ProcessError error)
{
    QString errorMsg;
    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = tr("Failed to start FFmpeg process.");
        break;
    case QProcess::Crashed:
        errorMsg = tr("FFmpeg process crashed.");
        break;
    case QProcess::Timedout:
        errorMsg = tr("FFmpeg process timed out.");
        break;
    case QProcess::WriteError:
        errorMsg = tr("Error writing to FFmpeg process.");
        break;
    case QProcess::ReadError:
        errorMsg = tr("Error reading from FFmpeg process.");
        break;
    default:
        errorMsg = tr("Unknown error occurred.");
        break;
    }
    
    addToLog(errorMsg, Qt::red);
    ui->textDetailedLog->append(QString("[ERROR] %1").arg(errorMsg));
    
    if (m_isProcessingBatch) {
        ui->tableQueue->item(m_currentBatchIndex, 2)->setText("Error");
        m_currentBatchIndex++;
        QTimer::singleShot(50, this, &MainWindow::processNextBatchItem);
    } else {
        enableControls(true);
        ui->progressBar->setVisible(false);
    }
}

void MainWindow::readProcessOutput()
{
    QByteArray output = m_ffmpegProcessor->readAllStandardOutput();
    QByteArray errorOutput = m_ffmpegProcessor->readAllStandardError();
    
    QString outputStr = QString::fromLocal8Bit(output);
    QString errorStr = QString::fromLocal8Bit(errorOutput);
    
    if (!outputStr.trimmed().isEmpty()) {
        addToLog(outputStr.trimmed(), Qt::blue);
    }
    
    if (!outputStr.trimmed().isEmpty()) {
        ui->textDetailedLog->append(outputStr.trimmed());
    }
    if (!errorStr.trimmed().isEmpty()) {
        ui->textDetailedLog->append(errorStr.trimmed());
    }
    
    ui->textDetailedLog->verticalScrollBar()->setValue(
        ui->textDetailedLog->verticalScrollBar()->maximum());
}

void MainWindow::addToLog(const QString& message, const QColor& color)
{
    QTextCursor cursor(ui->textLog->textCursor());
    cursor.movePosition(QTextCursor::End);
    
    QTextCharFormat format;
    format.setForeground(color);
    cursor.setCharFormat(format);
    
    cursor.insertText(QString("[%1] %2\n").arg(QTime::currentTime().toString(), message));
    
    ui->textLog->verticalScrollBar()->setValue(ui->textLog->verticalScrollBar()->maximum());
}

void MainWindow::enableControls(bool enabled)
{
    ui->btnStart->setEnabled(enabled);
    ui->btnStop->setEnabled(!enabled);
    ui->groupEncoding->setEnabled(enabled);
    ui->groupInputOutput->setEnabled(enabled);
    ui->groupPresets->setEnabled(enabled);
    
    if (m_isProcessingBatch) {
        ui->btnBrowseInput->setEnabled(false);
        ui->btnBrowseOutput->setEnabled(false);
    } else {
        ui->btnBrowseInput->setEnabled(enabled);
        ui->btnBrowseOutput->setEnabled(enabled);
    }
}

void MainWindow::updateHardwareAccelerationOptions()
{
#ifdef Q_OS_WIN
    ui->comboHwAccel->addItem("DXVA2");
    ui->comboHwAccel->addItem("D3D11VA");
#endif

#ifdef Q_OS_LINUX
    ui->comboHwAccel->addItem("VAAPI");
#endif

#ifdef Q_OS_MAC
    ui->comboHwAccel->addItem("Videotoolbox");
#endif
    
    ui->comboHwAccel->addItem("CUDA");
    ui->comboHwAccel->addItem("NVENC");
}
