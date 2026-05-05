#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mediatypes.h"

#include <QFileDialog>
#include <QDragEnterEvent>
#include <QGridLayout>
#include <QSlider>
#include <QDropEvent>
#include <QMimeData>
#include <QKeyEvent>
#include <QDir>
#include <QMenu>
#include <QGraphicsWidget>
#include <QLabel>
#include <QAction>
#include <cmath>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    volSlider = new QSlider(Qt::Horizontal, this);
    volSlider->setRange(0, 100);
    volSlider->setValue(100);
    volSlider->setFixedWidth(100);
    volSlider->move(500, 0);

    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->toolBar_2->addWidget(spacer);

    action = ui->toolBar_2->addWidget(volSlider);
    action->setVisible(false);


    connect(ui->actionPlay, &QAction::triggered, this, &MainWindow::play);
    connect(ui->actionPause, &QAction::triggered, this, &MainWindow::pause);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFiles);
    connect(volSlider, &QSlider::valueChanged, this, &MainWindow::changeVolume);

    installEventFilter(this);

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
    delete ui;
}

void MainWindow::openFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Select Media"),
        QDir::homePath()
        );

    for (const QString &f : files) addMedia(f);
}

void MainWindow::pause()
{
    if(!selectedIndices.empty()){
        for(int i = 0; i < (int)selectedIndices.size(); ++i) mediaSlots[selectedIndices[i]]->pause();
    }

}

void MainWindow::play()
{
    if(!selectedIndices.empty()){
        for(int i = 0; i < (int)selectedIndices.size(); ++i) mediaSlots[selectedIndices[i]]->play();
    }
}

void MainWindow::replay()
{
    if(!selectedIndices.empty()){
        for(int i = 0; i < (int)selectedIndices.size(); ++i) mediaSlots[selectedIndices[i]]->replay();
    }
}

void MainWindow::changeVolume(int value) {
    if (fullscreenIndex != -1)
        mediaSlots[fullscreenIndex]->setVolume(value / 100.0f);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    auto findSlotIndex = [&]() -> int {
        auto *w = qobject_cast<QWidget*>(obj);
        while (w) {
            for (int i = 0; i < (int)mediaSlots.size(); ++i )
                if (mediaSlots[i]->wrapper == w || mediaSlots[i]->border == w) return i;
            w = w->parentWidget();
        }
        return -1;
    };

    if (event->type() == QEvent::DragEnter) {
        auto *e = static_cast<QDragEnterEvent*>(event);
        e->acceptProposedAction();
        e->accept();
        return true;
    }

    if (event->type() == QEvent::DragMove) {
        auto *e = static_cast<QDragMoveEvent*>(event);
        e->acceptProposedAction();
        e->accept();
        return true;
    }

    if (event->type() == QEvent::Drop) {
        auto *e = static_cast<QDropEvent*>(event);
        for (const QUrl &url : e->mimeData()->urls()) {
            QString file = url.toLocalFile();
            if (!file.isEmpty())
                addMedia(file);
        }
        return true;
    }

    if (event->type() == QEvent::HoverEnter) {
        int i = findSlotIndex();
        if (i != -1) {
            hoveredIndex = i;
            mediaSlots[hoveredIndex]->toggleMediaControls(true);
            highlight();
            setFocus();
        }
        return true;
    }

    if (event->type() == QEvent::HoverLeave) {
        if (justClicked) { justClicked = false; return true; }
        int oldHovered = hoveredIndex;
        hoveredIndex = -1;
        bool onlyHovered = std::find(selectedIndices.begin(), selectedIndices.end(), oldHovered) == selectedIndices.end();
        if (oldHovered >= 0 && oldHovered < (int)mediaSlots.size() && onlyHovered)
            mediaSlots[oldHovered]->toggleMediaControls(false);

        highlight();
        setFocus();
        return true;
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto *e = static_cast<QMouseEvent*>(event);
        if (e->button() == Qt::LeftButton) {
            int i = findSlotIndex();
            bool ctrl = e->modifiers() & Qt::ControlModifier;
            if (i != -1) {
                justClicked = true;
                if (!ctrl) {
                    for (int j : selectedIndices)
                        mediaSlots[j]->toggleMediaControls(false);
                    selectedIndices.clear();
                }
                if (std::find(selectedIndices.begin(), selectedIndices.end(), i) == selectedIndices.end())
                    selectedIndices.emplace_back(i);

                for (int j : selectedIndices)
                    mediaSlots[j]->toggleMediaControls(true);

                highlight();
                setFocus();
                if (mediaSlots[i]->type() == "pdf" || mediaSlots[i]->type() == "image") return false;
                return true;
            }

            else {
                if (!ctrl) {
                    for (int j : selectedIndices)
                        mediaSlots[j]->toggleMediaControls(false);
                    selectedIndices.clear();
                    highlight();
                }
            }
        }

        else if (e->button() == Qt::RightButton){
            int i = findSlotIndex();
            QMenu menu;
            QAction *closeAct = menu.addAction("Close");
            QAction *selectedAct = menu.exec(QCursor::pos());
            if (hoveredIndex != -1 && hoveredIndex == i) removeMedia(hoveredIndex);
            if (selectedAct == closeAct && !selectedIndices.empty()) {
                for(size_t j = selectedIndices.size() - 1; j >= 0; --j) {
                    if (selectedIndices[j] == i) removeMedia(selectedIndices[j]);
                }
            }

            return true;
        }
    }

    if (event->type() == QEvent::Resize) {
        for (int i = 0; i < (int)mediaSlots.size(); ++i) {
            if (mediaSlots[i]->type() == "image" && mediaSlots[i]->wrapper == obj) {
                auto *img = dynamic_cast<ImageSlot*>(mediaSlots[i].get());
                QSize newSize = img->wrapper->size();
                if (newSize != img->lastSize) {
                    img->lastSize = newSize;
                    img->item->setPixmap(img->pixmap.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
                img->border->setGeometry(img->wrapper->rect());
                return true;
            }

            if (mediaSlots[i]->type() == "audio" && mediaSlots[i]->wrapper == obj) {
                auto *aud = static_cast<AudioSlot*>(mediaSlots[i].get());
                QSize newSize = aud->wrapper->size();
                if (newSize != aud->lastSize) {
                    aud->lastSize = newSize;
                    if (!aud->coverImage.isNull())
                        aud->cover->setPixmap(QPixmap::fromImage(aud->coverImage).scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
                int sliderHeight = aud->slider->isVisible() ? aud->slider->height() : 0;
                aud->overlay->setGeometry(0, aud->wrapper->height() - 60 - sliderHeight, aud->wrapper->width(), 60);
                aud->border->setGeometry(aud->wrapper->rect());
                return true;
            }

            if (mediaSlots[i]->wrapper == obj) {
                mediaSlots[i]->border->setGeometry(mediaSlots[i]->wrapper->rect());
                break;
            }
        }
    }



    if (event->type() == QEvent::KeyPress) {
        auto *e = static_cast<QKeyEvent*>(event);

        if (e->key() == Qt::Key_Space) {
            static bool isPaused  = true;
            if (!selectedIndices.empty()) {
                if(!isPaused) {
                    pause();
                    isPaused = true;
                }
                else{
                    play();
                    isPaused = false;
                }
            }
        }

        if (e->key() == Qt::Key_Delete) {
            if (!selectedIndices.empty()) {
                for(int j = (int)selectedIndices.size() - 1; j >= 0; --j) {
                    removeMedia(selectedIndices[j]);
                }
            }
        }

        if (e->key() == Qt::Key_M) {
            if (!selectedIndices.empty()) {
                for(int j{}; j < (int)selectedIndices.size(); ++j) {
                    mediaSlots[selectedIndices[j]]->toggleMute();
                }
            }
        }

        if (e->key() == Qt::Key_R) {
            replay();
        }

        if (e->key() == Qt::Key_F) {
            if (fullscreenIndex == -1) {
                enterFullscreen(selectedIndices[0]);

            }
            else exitFullscreen();
        }

        if(e->key() == Qt::Key_Escape) {
            if (fullscreenIndex != -1) exitFullscreen();
        }

        if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down || e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) {
            bool ctrl = e->modifiers() & Qt::ControlModifier;
            if(ctrl) {
                if (mediaSlots.size() > 1) {
                    int n = static_cast<int>(mediaSlots.size());
                    int cols = static_cast<int>(std::ceil(std::sqrt(n)));

                    if (selectedIndices.empty()) selectedIndices.emplace_back(0);

                    int row = selectedIndices[0] / cols;
                    int col = selectedIndices[0] % cols;

                    if (e->key() == Qt::Key_Up)         row--;
                    else if (e->key() == Qt::Key_Down)  row++;
                    else if (e->key() == Qt::Key_Right) col++;
                    else if (e->key() == Qt::Key_Left)  col--;

                    int newIndex = row * cols + col;

                    if (newIndex >= 0 && newIndex < n) selectedIndices[0] = newIndex;

                    if (fullscreenIndex == -1) {
                        mediaSlots[selectedIndices[0]]->toggleMediaControls(true);
                        highlight();
                    }

                    else {
                        exitFullscreen();
                        enterFullscreen(selectedIndices[0]);
                    }
                }
            }

            else {
                if (fullscreenIndex != -1) {
                    if (e->key() == Qt::Key_Right) mediaSlots[fullscreenIndex]->forward();
                    else if (e->key() == Qt::Key_Left) mediaSlots[fullscreenIndex]->backward();

                    else if (e->key() == Qt::Key_Up) {
                        mediaSlots[fullscreenIndex]->adjustVolume(0.3);
                        volSlider->setValue(mediaSlots[fullscreenIndex]->getVolume() * 100);
                    }

                    else if (e->key() == Qt::Key_Down) {
                        mediaSlots[fullscreenIndex]->adjustVolume(-0.3);
                        volSlider->setValue(mediaSlots[fullscreenIndex]->getVolume() * 100);
                    }
                }

                if (mediaSlots.size() == 1 && (mediaSlots[selectedIndices[0]] == mediaSlots[0])) {
                    if (e->key() == Qt::Key_Right) mediaSlots[0]->forward();
                    else if (e->key() == Qt::Key_Left) mediaSlots[0]->backward();

                    else if (e->key() == Qt::Key_Up) {
                        mediaSlots[fullscreenIndex]->adjustVolume(0.3);
                        volSlider->setValue(mediaSlots[0]->getVolume() * 100);
                    }

                    else if (e->key() == Qt::Key_Down) {
                        mediaSlots[fullscreenIndex]->adjustVolume(-0.3);
                        volSlider->setValue(mediaSlots[0]->getVolume() * 100);
                    }
                }
            }

            return true;
        }

        if (e->key() == Qt::Key_A) {
            bool ctrl = e->modifiers() & Qt::ControlModifier;
            if (ctrl) {
                if (selectedIndices.size() ==  mediaSlots.size()) {
                    selectedIndices.clear();
                }

                else {
                    selectedIndices.clear();
                    for (int i{}; i < (int)mediaSlots.size(); ++i) selectedIndices.emplace_back(i);
                }
                highlight();
            }
        }

        if (e->key() == Qt::Key_Minus || e->key() == Qt::Key_Plus) {
            if (fullscreenIndex != -1) {
                qreal factor = e->key() == Qt::Key_Minus ? 0.8 :  e->key() == Qt::Key_Plus ? 1.2 : 1;
                mediaSlots[fullscreenIndex]->zoom(factor);
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::enterFullscreen(int index)
{
    action->setVisible(true);
    ui->toolBar_2->update();
    fullscreenIndex = index;
    grid->removeWidget(mediaSlots[index]->wrapper);
    mediaSlots[index]->wrapper->setParent(container);
    mediaSlots[index]->wrapper->setGeometry(container->rect());
    mediaSlots[index]->wrapper->raise();
    mediaSlots[index]->wrapper->show();
    for (int i = 0; i < (int)mediaSlots.size(); ++i) {
        if (i != index) {
            if (mediaSlots[i]->getPlayerState() == QMediaPlayer::PlayingState){
                playingIndices.emplace_back(i);
                mediaSlots[i]->pause();
            }
            mediaSlots[i]->wrapper->hide();
        }
    }
}

void MainWindow::exitFullscreen()
{
    action->setVisible(false);
    ui->toolBar_2->update();
    mediaSlots[fullscreenIndex]->wrapper->setParent(container);
    rebuildGrid();
    for (int i = 0; i < (int)playingIndices.size(); ++i) mediaSlots[playingIndices[i]]->play();
    playingIndices.clear();
    fullscreenIndex = -1;
}


void MainWindow::addMedia(const QString &path) {
    auto slot = makeSlot(path, container, this);
    if (!slot) return;
    mediaSlots.push_back(std::move(slot));
    //if (fullscreenIndex == -1) rebuildGrid();
    rebuildGrid();
}

void MainWindow::rebuildGrid()
{

    for (auto &s : mediaSlots) grid->removeWidget(s->wrapper);

    int n = static_cast<int>(mediaSlots.size());
    for (int r = 0; r < grid->rowCount(); ++r) grid->setRowStretch(r, 0);
    for (int c = 0; c < grid->columnCount(); ++c) grid->setColumnStretch(c, 0);

    if (n == 0) return;

    int cols = static_cast<int>(std::ceil(std::sqrt(n)));
    int rows = static_cast<int>(std::ceil(static_cast<double>(n) / cols));


    for (int i = 0; i < n; ++i) {
        int r = i / cols;
        int c = i % cols;
        grid->addWidget(mediaSlots[i]->wrapper, r, c);
        mediaSlots[i]->wrapper->show();
    }
    for (int r = 0; r < rows; ++r) grid->setRowStretch(r, 1);

    for (int c = 0; c < cols; ++c) grid->setColumnStretch(c, 1);
}


void MainWindow::highlight()
{

    for (int i = 0; i < (int)mediaSlots.size(); ++i) {
        bool selected = std::find(selectedIndices.begin(), selectedIndices.end(), i) != selectedIndices.end();
        bool hovered  = (i == hoveredIndex);

        if (selected)
            mediaSlots[i]->border->setStyleSheet("border: 3px solid #00aaff;");
        else if (hovered)
            mediaSlots[i]->border->setStyleSheet("border: 3px solid #555555;");
        else
            mediaSlots[i]->border->setStyleSheet("border: none;");
    }
}

void MainWindow::removeMedia(int index){
    if (index < 0 || index >= static_cast<int>(mediaSlots.size())) return;

    mediaSlots[index]->wrapper->hide();
    grid->removeWidget(mediaSlots[index]->wrapper);
    mediaSlots.erase(mediaSlots.begin() + index);
    selectedIndices.clear();
    hoveredIndex = -1;
    container->update();
    container->repaint();
    rebuildGrid();
}

//TODO removal of media thru context menus is borderline unusable, need to figure out right->left
//sequece and get exact position of context menu and its options

