#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QKeyEvent>
#include <QDir>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    container = new QWidget(this);
    grid = new QGridLayout(container);
    grid->setSpacing(4);
    grid->setContentsMargins(0, 0, 0, 0);
    container->setAcceptDrops(true);
    container->installEventFilter(this);
    setCentralWidget(container);
    setFocusPolicy(Qt::StrongFocus);
}

MainWindow::~MainWindow()
{
    for (auto &s : videoSlots) {
        s.player->stop();
        delete s.audio;
        delete s.player;
        delete s.wrapper;
    }

    delete ui;
}

void MainWindow::addVideo(const QString &path)
{
    if (videoSlots.size() >= 12) {
        ui->statusBar->showMessage("limit reached b0ss");
        return;
    }

    VideoSlot s;


    s.player  = new QMediaPlayer(this);
    s.audio   = new QAudioOutput(this);
    s.wrapper = new QWidget(container);
    s.video   = new QVideoWidget(s.wrapper);
    s.slider  = new QSlider(Qt::Horizontal, s.wrapper);

    QWidget* wrapperPtr = s.wrapper;

    auto *layout = new QVBoxLayout(s.wrapper);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(s.video);
    layout->addWidget(s.slider);

    s.player->setAudioOutput(s.audio);
    s.player->setVideoOutput(s.video);
    s.audio->setVolume(1.0);

    s.slider->setRange(0, 0);
    s.wrapper->setContextMenuPolicy(Qt::CustomContextMenu);
    s.wrapper->installEventFilter(this);
    s.slider->setFocusPolicy(Qt::NoFocus);
    s.video->setFocusPolicy(Qt::NoFocus);

    connect(s.player, &QMediaPlayer::durationChanged,
            s.slider, &QSlider::setMaximum);

    connect(s.player, &QMediaPlayer::positionChanged,
            s.slider, &QSlider::setValue);

    connect(s.slider, &QSlider::sliderMoved,
            s.player, &QMediaPlayer::setPosition);

    connect(s.wrapper, &QWidget::customContextMenuRequested,
            this, [this, wrapperPtr](const QPoint &) {

                QMenu menu;
                QAction *closeAct  = menu.addAction("Close");
                QAction *replayAct = menu.addAction("Replay");

                QAction *selected = menu.exec(QCursor::pos());

                if (selected == closeAct) {
                    for (size_t i = 0; i < videoSlots.size(); ++i) {
                        if (videoSlots[i].wrapper == wrapperPtr) {
                            removeVideo(static_cast<int>(i));
                            break;
                        }
                    }
                }
                else if (selected == replayAct) {
                    for (size_t i = 0; i < videoSlots.size(); ++i) {
                        if (videoSlots[i].wrapper == wrapperPtr) {
                            on_actionReplay_triggered(static_cast<int>(i));
                            break;
                        }
                    }
                }
            });

    s.player->setSource(QUrl::fromLocalFile(path));
    s.player->play();

    videoSlots.push_back(s);
    rebuildGrid();
}

void MainWindow::removeVideo(int index)
{
    if (index < 0 || index >= static_cast<int>(videoSlots.size()))
        return;

    auto &s = videoSlots[index];
    s.player->stop();

    grid->removeWidget(s.wrapper);

    delete s.audio;
    delete s.player;
    delete s.wrapper;

    videoSlots.erase(videoSlots.begin() + index);

    rebuildGrid();
}

void MainWindow::rebuildGrid()
{
    for (auto &s : videoSlots) {
        grid->removeWidget(s.wrapper);
    }

    int n = static_cast<int>(videoSlots.size());
    if (n == 0)
        return;

    int cols = static_cast<int>(std::ceil(std::sqrt(n)));
    int rows = static_cast<int>(std::ceil(static_cast<double>(n) / cols));

    for (int i = 0; i < n; ++i) {
        int r = i / cols;
        int c = i % cols;
        grid->addWidget(videoSlots[i].wrapper, r, c);
        videoSlots[i].video->show();
    }

    for (int r = 0; r < rows; ++r) {
        grid->setRowStretch(r, 1);
    }

    for (int c = 0; c < cols; ++c) {
        grid->setColumnStretch(c, 1);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *e)
{
    for (const QUrl &url : e->mimeData()->urls()) {
        QString file = url.toLocalFile();
        if (!file.isEmpty())
            addVideo(file);
    }
}

void MainWindow::on_actionOpen_triggered()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Select Media"),
        QDir::homePath()
        );

    for (const QString &f : files)
        addVideo(f);
}

void MainWindow::on_actionPlay_triggered(int index)
{
    if (index != -1) {
        videoSlots[index].player->play();
        return;
    }

    if (selectedIndex != -1) {
        videoSlots[selectedIndex].player->play();
    }
    else {
        for (auto &s : videoSlots)
            s.player->play();
    }
}
void MainWindow::on_actionReplay_triggered(int index)
{
    if(index == -1){
        for (auto &s : videoSlots) {
            s.player->setPosition(0);
            s.player->play();
        }
    }
    else {
        videoSlots[index].player->setPosition(0);
        videoSlots[index].player->play();
    }

}

void MainWindow::on_actionPause_triggered()
{
    if(selectedIndex == -1){
        for (auto &s : videoSlots) {
            s.player->pause();
        }
    }
    else {
        videoSlots[selectedIndex].player->pause();
    }

}

void MainWindow::on_actionStop_triggered()
{
    for (auto &s : videoSlots)
        s.player->stop();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {

    case Qt::Key_Space:
    {
        if (videoSlots.empty())
            break;
        if (selectedIndex != -1)
        {
            auto &player = videoSlots[selectedIndex].player;
            if (player->playbackState() == QMediaPlayer::PlayingState)
                player->pause();
            else
                player->play();
        }
        else
        {
            bool anyPlaying = false;
            for (auto &s : videoSlots)
            {
                if (s.player->playbackState() == QMediaPlayer::PlayingState)
                {
                    anyPlaying = true;
                    break;
                }
            }

            if (anyPlaying)
            {
                for (auto &s : videoSlots)
                    s.player->pause();
            }
            else
            {
                for (auto &s : videoSlots)
                    s.player->play();
            }
        }
        break;
    }

    case Qt::Key_Delete:
        if (!videoSlots.empty()) {
            if (selectedIndex != -1)
                removeVideo(selectedIndex);
            else
                removeVideo(static_cast<int>(videoSlots.size()) - 1);
        }
        break;

    case Qt::Key_R:
        on_actionReplay_triggered(selectedIndex);
        break;

    case Qt::Key_F:
    {
        if (selectedIndex == -1)
        {
            isFullscreen = !isFullscreen;
            isFullscreen ? showFullScreen() : showNormal();
        }
        else
        {
            if (!fullscreenWidget)
            {
                fullscreenIndex = selectedIndex;
                fullscreenWidget = videoSlots[selectedIndex].wrapper;

                grid->removeWidget(fullscreenWidget);

                fullscreenWidget->setParent(nullptr);
                fullscreenWidget->showFullScreen();
            }
            else
            {
                fullscreenWidget->hide();
                fullscreenWidget->setParent(container);
                rebuildGrid();

                fullscreenWidget = nullptr;
                fullscreenIndex = -1;
            }
        }

        break;
    }

    case Qt::Key_Escape:
    {
        if (fullscreenWidget)
        {
            fullscreenWidget->hide();
            fullscreenWidget->setParent(container);
            rebuildGrid();

            fullscreenWidget = nullptr;
            fullscreenIndex = -1;
        }
        else
        {
            selectedIndex = -1;
            updateSelectionVisuals();
            showNormal();
        }
        break;
    }

    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    {
        if (videoSlots.empty())
            break;

        int n = static_cast<int>(videoSlots.size());
        int cols = static_cast<int>(std::ceil(std::sqrt(n)));

        if (selectedIndex == -1)
            selectedIndex = 0;

        int row = selectedIndex / cols;
        int col = selectedIndex % cols;

        switch (event->key()) {
        case Qt::Key_Left:  col--; break;
        case Qt::Key_Right: col++; break;
        case Qt::Key_Up:    row--; break;
        case Qt::Key_Down:  row++; break;
        }

        int newIndex = row * cols + col;

        if (newIndex >= 0 && newIndex < n)
            selectedIndex = newIndex;

        updateSelectionVisuals();
        break;
    }

    default:
        QMainWindow::keyPressEvent(event);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::DragEnter) {
        auto *e = static_cast<QDragEnterEvent*>(event);
        if (e->mimeData()->hasUrls())
            e->acceptProposedAction();
        return true;
    }

    if (event->type() == QEvent::Drop) {
        auto *e = static_cast<QDropEvent*>(event);
        for (const QUrl &url : e->mimeData()->urls()) {
            QString file = url.toLocalFile();
            if (!file.isEmpty())
                addVideo(file);
        }
        return true;
    }

    if (event->type() == QEvent::MouseButtonPress) {
        for (int i = 0; i < static_cast<int>(videoSlots.size()); ++i) {
            if (videoSlots[i].wrapper == obj) {
                selectedIndex = i;
                updateSelectionVisuals();
                this->setFocus();
                break;
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::updateSelectionVisuals()
{
    for (int i = 0; i < static_cast<int>(videoSlots.size()); ++i) {
        if (i == selectedIndex) {
            videoSlots[i].wrapper->setStyleSheet(
                "border: 3px solid #00aaff;"
                );
        } else {
            videoSlots[i].wrapper->setStyleSheet(
                "border: none;"
                );
        }
    }
}
