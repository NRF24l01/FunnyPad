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
    });

    connect(&audio, &soundpad::SoundpadAudio::playbackStarted, this, &MainWindow::on_playbackStarted);
    connect(&audio, &soundpad::SoundpadAudio::playbackProgress, this, &MainWindow::on_playbackProgress);
    connect(&audio, &soundpad::SoundpadAudio::playbackStopped, this, &MainWindow::on_playbackStopped);

    connect(ui->musicProgress, &QSlider::sliderMoved, this, &MainWindow::on_musicProgress_sliderMoved);
}

MainWindow::~MainWindow()
{
    audio.stop();
    delete ui;
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
        ui->playbackButton->setText("Stop");
        isPlaying = true;
    } else {
        audio.stop();
        ui->playbackButton->setText("Play");
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
    ui->playbackButton->setText("Stop");
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
    ui->playbackButton->setText("Play");
    isPlaying = false;
}