#include "main_window.hpp"
#include "ui_main.h"
#include <QMessageBox>
#include <QFile>
#include <QStandardPaths>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , audio() // initialize audio here
{
    ui->setupUi(this);

    settings = new QSettings("nrf24l01", "FunnyPad");
    qDebug() << "Settings path:" << settings->fileName();

    auto sources = audio.getSourceList();

    ui->outputSelect->clear();
    ui->outputSelect->addItem("Select Audio Source", "");

    for (const auto& source : sources) {
        ui->outputSelect->addItem(QString::fromStdString(source.second), QString::fromStdString(source.first));
    }

    connect(ui->outputSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        QString sourceName = ui->outputSelect->itemData(index).toString();
        if (sourceName == "") {
            return;
        }
        if (ui->outputSelect->count() > 0 && ui->outputSelect->itemData(0).toString() == "") {
            ui->outputSelect->removeItem(0);
        }
        qDebug() << "Selected audio source:" << sourceName;
        if (!audio.mergeWithMic(sourceName.toStdString())) {
            QMessageBox::warning(this, "Error", "Failed to merge with selected audio source.");
        }
        settings->setValue("audio_source", sourceName);
    });

    QString savedSource = settings->value("audio_source", "").toString();
    if (!savedSource.isEmpty()) {
        int index = ui->outputSelect->findData(savedSource);
        if (index != -1) {
            ui->outputSelect->setCurrentIndex(index);
        }
    }

    connect(&audio, &soundpad::SoundpadAudio::playbackStarted, this, &MainWindow::on_playbackStarted);
    connect(&audio, &soundpad::SoundpadAudio::playbackProgress, this, &MainWindow::on_playbackProgress);
    connect(&audio, &soundpad::SoundpadAudio::playbackStopped, this, &MainWindow::on_playbackStopped);

    connect(ui->musicProgress, &QSlider::sliderMoved, this, &MainWindow::on_musicProgress_sliderMoved);

    data_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    qDebug() << "Data path:" << data_path;
}

MainWindow::~MainWindow()
{
    audio.stop();
    delete ui;
    delete settings;
}

void MainWindow::on_playbackButton_clicked()
{
    if (!isPlaying) {
        QString filePath = "/home/Technik12345/Downloads/YUbochka_CHulochki_-_ya_CHVK_fembojjchik_76781912.wav";
        if (!QFile::exists(filePath)) {
            QMessageBox::warning(this, "Ошибка", "Файл не найден:\n" + filePath);
            return;
        }
        if (!audio.playWav(filePath.toStdString())) {
            QMessageBox::warning(this, "Ошибка", "Не удалось воспроизвести файл:\n" + filePath);
            return;
        }
        ui->playbackButton->setIcon(QIcon(":/icons/resources/icons/stop.png"));
        isPlaying = true;
    } else {
        audio.stop();
        ui->playbackButton->setIcon(QIcon(":/icons/resources/icons/play.png"));
        isPlaying = false;
    }
}

void MainWindow::on_musicProgress_sliderMoved(int value)
{
    // value is in ms
    audio.seek(value);
}

void MainWindow::on_playbackStarted(qint64 totalMs)
{
    ui->musicProgress->setMaximum(totalMs);
    ui->musicProgress->setValue(0);
    ui->currentMusicTime->setText("00:00");
    int sec = totalMs / 1000;
    ui->totalMusicTime->setText(QString("%1:%2").arg(sec/60,2,10,QChar('0')).arg(sec%60,2,10,QChar('0')));
    ui->playbackButton->setIcon(QIcon(":/icons/resources/icons/stop.png"));
    isPlaying = true;
}

void MainWindow::on_playbackProgress(qint64 currentMs)
{
    ui->musicProgress->setValue(currentMs);
    int sec = currentMs / 1000;
    ui->currentMusicTime->setText(QString("%1:%2").arg(sec/60,2,10,QChar('0')).arg(sec%60,2,10,QChar('0')));
}

void MainWindow::on_playbackStopped()
{
    ui->playbackButton->setIcon(QIcon(":/icons/resources/icons/play.png"));
    isPlaying = false;
}