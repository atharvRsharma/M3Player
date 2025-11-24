#include "mainwindow.h"
#include "qaudiooutput.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QtWidgets>
#include <QDebug>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // defns
    player = new QMediaPlayer(this);
    audio = new QAudioOutput(this);
    video = new QVideoWidget(this);
    slider = new QSlider(this);
    bar = new QProgressBar(this);

    slider->setOrientation(Qt::Horizontal);
    ui->statusBar->addPermanentWidget(slider);
    ui->statusBar->addPermanentWidget(bar);

    // resizing
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(video);
    QWidget *container = new QWidget(this);
    container->setLayout(layout);
    setCentralWidget(container);





    // connect signals to slots, preferred the old SIGNAL SLOT stuff lwk
    connect(player, &QMediaPlayer::durationChanged, slider, &QSlider::setMaximum);
    connect(player, &QMediaPlayer::positionChanged, slider, &QSlider::setValue);
    connect(slider, &QSlider::sliderMoved, player, &QMediaPlayer::setPosition);
    //connect(slider, &QSlider::sliderPressed, player, &QMediaPlayer::setPosition);

    //connect(player, &QMediaPlayer::durationChanged, bar, &QProgressBar::setMaximum);
    //connect(player, &QMediaPlayer::positionChanged, bar, &QProgressBar::setValue);

    // giving the player the essentials
    player->setAudioOutput(audio);
    player->setVideoOutput(video);

    video->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    fileNames = QFileDialog::getOpenFileNames(
        this,
        tr("Select Media File"),
        QDir::homePath(),
        tr("Media Files (*.mp4 *.mp3 *.wav *.avi *.mkv);;All Files (*.*)")
        );

    if (!fileNames.isEmpty()) {
        for(uint32_t i = 0; i < fileNames.size(); ++i){
            player->setSource(QUrl::fromLocalFile(fileNames[i]));
            player->play();
        }
    }

    fileNames.clear();
}


void MainWindow::on_actionStop_triggered()
{
    player->stop();
    ui->statusBar->showMessage("stopped, file instance removed");
}



void MainWindow::on_actionPlay_triggered()
{
    player->play();
    ui->statusBar->showMessage("playing");
}


void MainWindow::on_actionPause_triggered()
{
    player->pause();
    ui->statusBar->showMessage("paused");
}




