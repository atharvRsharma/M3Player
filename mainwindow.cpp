#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAcceptDrops(true);


    player = new QMediaPlayer(this);
    audio = new QAudioOutput(this);
    video = new QVideoWidget(this);


    dropZone = new QLabel(this);
    dropZone->setAcceptDrops(true);

    dropZone->setStyleSheet("background-color: transparent;");

    dropZone->installEventFilter(this);


    QStackedLayout *stack = new QStackedLayout();
    stack->setStackingMode(QStackedLayout::StackAll);

    stack->addWidget(video);
    stack->addWidget(dropZone);

    QWidget *container = new QWidget(this);
    container->setLayout(stack);
    setCentralWidget(container);


    player->setAudioOutput(audio);
    player->setVideoOutput(video);
    audio->setVolume(1.0);

    slider = new QSlider(Qt::Horizontal, this);
    //slider->setFixedWidth(400);
    ui->statusBar->addPermanentWidget(slider, Qt::AlignHCenter);


    connect(player, &QMediaPlayer::durationChanged, slider, &QSlider::setMaximum);
    connect(player, &QMediaPlayer::positionChanged, slider, &QSlider::setValue);
    connect(slider, &QSlider::sliderMoved, player, &QMediaPlayer::setPosition);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::dropEvent(QDropEvent *e)
{
    foreach (const QUrl &url, e->mimeData()->urls()) {
        QString fileName = url.toLocalFile();
        if (fileName.isEmpty()) return;

        ui->statusBar->showMessage("Playing: " + fileName);
        player->setSource(QUrl::fromLocalFile(fileName));
        player->play();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}


bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{

    if (watched == dropZone) {

        if (event->type() == QEvent::DragEnter) {
            QDragEnterEvent *dragEvt = static_cast<QDragEnterEvent*>(event);
            dragEvt->acceptProposedAction();
            return true;
        }

        if (event->type() == QEvent::Drop) {
            QDropEvent *dropEvt = static_cast<QDropEvent*>(event);

            MainWindow::dropEvent(dropEvt);
            return true;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}


void MainWindow::on_actionStop_triggered() { player->stop(); ui->statusBar->showMessage("stopped"); }
void MainWindow::on_actionPlay_triggered() { player->play(); ui->statusBar->showMessage("playing"); }
void MainWindow::on_actionPause_triggered() { player->pause(); ui->statusBar->showMessage("paused"); }

void MainWindow::on_actionOpen_triggered()
{
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Select Media"), QDir::homePath());
    if (!files.isEmpty()) {
        player->setSource(QUrl::fromLocalFile(files.first()));
        on_actionPlay_triggered();
    }
}

void MainWindow::on_actionNext_triggered()
{

}

