#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "SoundpadAudio.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_playbackButton_clicked();
    void on_musicProgress_sliderMoved(int value);
    void on_playbackStarted(qint64 totalMs);
    void on_playbackProgress(qint64 currentMs);
    void on_playbackStopped();

private:
    Ui::MainWindow *ui;
    soundpad::SoundpadAudio audio;
    bool isPlaying = false;
};

#endif // MAINWINDOW_H
