#include "main_window.hpp"
#include "ui_main.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_playbackButton_clicked()
{
    qDebug() << "Кнопка нажата!";
}
