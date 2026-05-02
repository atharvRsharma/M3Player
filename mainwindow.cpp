#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mediatypes.h"

#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QKeyEvent>
#include <QDir>
#include <QMenu>
#include <cmath>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

void MainWindow::on_actionOpen_triggered()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Select Media"),
        QDir::homePath()
        );

    for (const QString &f : files) addMedia(f);
}

void MainWindow::on_actionPause_triggered()
{
    if(!selectedIndices.empty()){
        for(int i = 0; i < (int)selectedIndices.size(); ++i) mediaSlots[selectedIndices[i]]->pause();
    }

}

void MainWindow::on_actionPlay_triggered()
{
    if(!selectedIndices.empty()){
        for(int i = 0; i < (int)selectedIndices.size(); ++i) mediaSlots[selectedIndices[i]]->play();
    }
}

void MainWindow::on_actionReplay_triggered()
{
    if(!selectedIndices.empty()){
        for(int i = 0; i < (int)selectedIndices.size(); ++i) mediaSlots[selectedIndices[i]]->replay();
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    auto findSlotIndex = [&]() -> int {
        auto *w = qobject_cast<QWidget*>(obj);
        while (w) {
            for (int i = 0; i < (int)mediaSlots.size(); ++i)
                if (mediaSlots[i]->wrapper == w || mediaSlots[i]->border == w) return i;
            w = w->parentWidget();
        }
        return -1;
    };

    if (event->type() == QEvent::DragEnter) {

        auto *e = static_cast<QDragEnterEvent*>(event);
        if (e->mimeData()->hasUrls()) e->acceptProposedAction();
        return true;
    }

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

    if (event->type() == QEvent::HoverLeave) {
        if (justClicked) { justClicked = false; return true; }
        hoveredIndex = -1;
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
                if (!ctrl) selectedIndices.clear();
                if (std::find(selectedIndices.begin(), selectedIndices.end(), i) == selectedIndices.end())
                    selectedIndices.emplace_back(i);
                highlight();
                setFocus();
                return true;
            }

            else {
                if (!ctrl) {
                    // check if obj belongs to any slot before clearing
                    bool isSlotWidget = false;
                    for (auto &s : mediaSlots)
                        if (s->border == obj || s->wrapper == obj) { isSlotWidget = true; break; }
                    if (!isSlotWidget) {
                        selectedIndices.clear();
                        highlight();
                    }
                }
            }
        }

        else if (e->button() == Qt::RightButton){
            QMenu menu;
            QAction *closeAct = menu.addAction("Close");
            QAction *selectedAct = menu.exec(QCursor::pos());
            if (selectedAct == closeAct && !selectedIndices.empty()) {
                for(int j = selectedIndices.size() - 1; j > -1; --j) {
                    removeMedia(selectedIndices[j]);
                }
            }

            else if (hoveredIndex != -1) removeMedia(hoveredIndex);
            return true;
        }
    }

    if (event->type() == QEvent::Resize) {
        for (int i = 0; i < (int)mediaSlots.size(); ++i) {
            auto *img = dynamic_cast<ImageSlot*>(mediaSlots[i].get());
            if (img && img->wrapper == obj) {
                QSize newSize = img->wrapper->size();
                if (newSize != img->lastSize) {
                    img->lastSize = newSize;
                    img->imageLabel->setPixmap(img->pixmap.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
                img->border->setGeometry(img->wrapper->rect());
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
            static bool isPaused  = false;
            if (!selectedIndices.empty()) {
                if(!isPaused) {
                    on_actionPause_triggered();
                    isPaused = true;
                }
                else{
                    on_actionPlay_triggered();
                    isPaused = false;
                }
            }
        }

        if (e->key() == Qt::Key_Delete) {
            if (!selectedIndices.empty()) {
                for(int j = selectedIndices.size() - 1; j > -1; --j) {
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
            on_actionReplay_triggered();
        }

        if (e->key() == Qt::Key_F) {
            if (fullscreenIndex == -1) enterFullscreen(selectedIndices[0]);
            else exitFullscreen();
        }

        if(e->key() == Qt::Key_Escape) {
            if (fullscreenIndex != -1) exitFullscreen();
        }

        if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down || e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) {
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

            if (fullscreenIndex == -1) highlight();

            else {
                exitFullscreen();
                enterFullscreen(selectedIndices[0]);
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
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::enterFullscreen(int index)
{
    fullscreenIndex = index;
    grid->removeWidget(mediaSlots[index]->wrapper);
    mediaSlots[index]->wrapper->setParent(container);
    mediaSlots[index]->wrapper->setGeometry(container->rect());
    mediaSlots[index]->wrapper->raise();
    mediaSlots[index]->wrapper->show();
    for (int i = 0; i < (int)mediaSlots.size(); ++i) {
        if (i != index) {
            mediaSlots[i]->pause();
            mediaSlots[i]->wrapper->hide();
        }
    }
}

void MainWindow::exitFullscreen()
{
    mediaSlots[fullscreenIndex]->wrapper->setParent(container);
    rebuildGrid();
    for (int i = 0; i < (int)mediaSlots.size(); ++i) mediaSlots[i]->play();
    fullscreenIndex = -1;
}


void MainWindow::addMedia(const QString &path) {
    auto slot = makeSlot(path, container, this);
    if (!slot) return;
    mediaSlots.push_back(std::move(slot));
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
            mediaSlots[i]->wrapper->setStyleSheet("border: 3px solid #00aaff;");
        else if (hovered)
            mediaSlots[i]->wrapper->setStyleSheet("border: 3px solid #555555;");
        else
            mediaSlots[i]->wrapper->setStyleSheet("border: none;");
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



