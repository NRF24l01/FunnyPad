#include "main_window.hpp"
#include "ui_main.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    audio = soundpad::SoundpadAudio();

    auto sources = audio.getSourceList();
    for (const auto& source : sources) {
        ui->outputSelect->addItem(QString::fromStdString(source.second), QString::fromStdString(source.first));
    }

    connect(ui->outputSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        QString sourceName = ui->outputSelect->itemData(index).toString();
        if (!audio.mergeWithMic(sourceName.toStdString())) {
            QMessageBox::warning(this, "Error", "Failed to merge with selected audio source.");
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_playbackButton_clicked()
{
    qDebug() << "Кнопка нажата!";
    audio.playWav("test.wav");
    qDebug() << "Playback finished.";
}
