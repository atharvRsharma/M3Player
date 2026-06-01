#include "mainwindow.h"
#include "qtimer.h"
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
#include <QScrollArea>
#include <QStyle>
#include <QLineEdit>
#include <QComboBox>
#include <QTreeView>
#include <QPdfPageNavigator>
#include <QScrollBar>
#include <QGraphicsView>
#include <QPdfView>
#include <QInputDialog>
#include <QProcess>
#include <QVideoWidget>

#include <cmath>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    volSlider = new QSlider(Qt::Horizontal, this);
    volSlider->setRange(0, 100);
    volSlider->setValue(100);
    volSlider->setFixedWidth(180);

    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->toolBar_2->addWidget(spacer);

    QWidget *spacer1 = new QWidget();
    spacer1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->toolBar->addWidget(spacer1);

    action = ui->toolBar_2->addWidget(volSlider);
    action->setVisible(false);

    actionSettings = new QAction(QIcon(":/images/icons/menu.png"), "", this);
    ui->toolBar->addAction(actionSettings);
    // ui->toolBar_2->hide();

    connect(ui->actionPlay,     &QAction::triggered, this, &MainWindow::play);
    connect(ui->actionPause,    &QAction::triggered, this, &MainWindow::pause);
    connect(ui->actionOpen, &QAction::triggered, this, [this]() {
        qDebug() << "actionOpen triggered";
        openFiles();
    });
    connect(ui->actionLink,     &QAction::triggered, this, &MainWindow::openLinks);
    connect(actionSettings,     &QAction::triggered, this, &MainWindow::toggleSettings);
    connect(volSlider,          &QSlider::valueChanged, this, &MainWindow::changeVolume);

    installEventFilter(this);

    container = new QWidget(this);
    grid = new QGridLayout(container);
    grid->setSpacing(4);
    grid->setContentsMargins(0, 0, 0, 0);
    container->setAcceptDrops(true);
    container->installEventFilter(this);
    container->setAttribute(Qt::WA_AcceptTouchEvents);
    setCentralWidget(container);
    setFocusPolicy(Qt::StrongFocus);

    rebuildDebounce = new QTimer(this);
    rebuildDebounce->setSingleShot(true);
    rebuildDebounce->setInterval(150);
    connect(rebuildDebounce, &QTimer::timeout, this, [this]() {
        for (auto &p : pendingPaths) {
            auto slot = makeSlot(p, container, this);
            if (slot) mediaSlots.push_back(std::move(slot));
        }
        pendingPaths.clear();
        rebuildGrid();
    });

    settings = new QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint);
    QRect r = container->rect();
    QPoint topRightGlobal = container->mapToGlobal(QPoint(r.width(), 0));
    int settingsWidth = r.width() / 4;
    settings->setGeometry(
        topRightGlobal.x() - settingsWidth,
        topRightGlobal.y(),
        settingsWidth,
        r.height()
        );
    settings->hide();
}
MainWindow::~MainWindow()
{
    delete ui;
}

// void MainWindow::openFiles() {
//     QStringList files = QFileDialog::getOpenFileNames(
//         this,
//         tr("Select Media"),
//         QDir::homePath()
//         );

//     for (const QString &f : files) addMedia(f);
// }

void MainWindow::openFiles() {
#ifdef Q_OS_ANDROID
    QList<QUrl> urls = QFileDialog::getOpenFileUrls(
        this,
        tr("Select Media"),
        QUrl()
        );
    qDebug() << "openFiles got urls:" << urls;
    for (const QUrl &u : urls) addMedia(u.toString());
#else
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Select Media"),
        QDir::homePath()
        );
    for (const QString &f : files) addMedia(f);
#endif
}

void MainWindow::openCommandLineArgs(const QString &path) {
    pendingPaths.append(path);
    rebuildDebounce->start();
}

void MainWindow::openLinks() {
    QString url = QInputDialog::getText(this, "enter url", "url: ");
    if (url.isEmpty()) return;

    QProcess *proc = new QProcess();
    proc->start("yt-dlp.exe", {"--remote-components", "ejs:github",
                               "--js-runtimes", "node",
                               "--cookies-from-browser", "firefox",
                               "-f", "b",
                               "-g", url});

    connect(proc, &QProcess::finished, this, [=]() {
        QString out = proc->readAllStandardOutput().trimmed();
        QString err = proc->readAllStandardError().trimmed();
        qDebug() << "raw out:" << out;
        qDebug() << "err:" << err;
        QStringList urls = out.split('\n', Qt::SkipEmptyParts);
        qDebug() << "url count:" << urls.size();
        for (const QString &streamUrl : urls) {
            QString u = streamUrl.trimmed();
            qDebug() << "adding:" << u;
            if (!u.isEmpty())
                addMedia(u);
        }
        proc->deleteLater();
    });
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
            if (file.endsWith(".srt", Qt::CaseInsensitive)) {
                int target = hoveredIndex != -1 ? hoveredIndex :
                                 !selectedIndices.empty() ? selectedIndices[0] : -1;
                if (target != -1 && mediaSlots[target]->type() == "video") {
                    auto *vid = static_cast<VideoSlot*>(mediaSlots[target].get());
                    vid->loadExternalSubtitles(file);
                }
            } else {
                if (!file.isEmpty()) {
                    if (QFileInfo(file).isDir()) {
                        for (const auto &sub : QDir(file).entryInfoList(QDir::Files))
                            addMedia(sub.filePath());
                    } else {
                        addMedia(file);
                    }
                }
            }
        }
        return true;
    }

    if (event->type() == QEvent::MouseMove) {
        if (fullscreenIndex != -1 && mediaSlots[fullscreenIndex]->type() == "pdf") {
            auto *pdf = static_cast<PdfSlot*>(mediaSlots[fullscreenIndex].get());
            if (obj == pdf->viewer->viewport()) {
                auto *e = static_cast<QMouseEvent*>(event);
                if (pdf->processLinks(e->pos(), false))
                    pdf->viewer->viewport()->setCursor(Qt::PointingHandCursor);
                else
                    pdf->viewer->viewport()->setCursor(Qt::ArrowCursor);
                return false;
            }
        }
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

    if (event->type() == QEvent::MouseButtonDblClick) {
        auto *e = static_cast<QMouseEvent*>(event);
        if (e->button() == Qt::LeftButton) {
            int i = findSlotIndex();
            if (fullscreenIndex == -1 && i != -1)
                enterFullscreen(i);
            else if (fullscreenIndex != -1)
                exitFullscreen();
            return true;
        }
    }


    if (event->type() == QEvent::MouseButtonPress) {
        auto *e = static_cast<QMouseEvent*>(event);

        if (e->button() == Qt::LeftButton) {
            int i = findSlotIndex();
            bool ctrl = e->modifiers() & Qt::ControlModifier;

            if (fullscreenIndex != -1) {
                if (mediaSlots[fullscreenIndex]->type() == "pdf") {
                    auto *pdf = static_cast<PdfSlot*>(mediaSlots[fullscreenIndex].get());

                    if (obj == pdf->thumbnailView->viewport() && pdf->thumbnailView->isVisible()) {
                        QPointF sp = pdf->thumbnailView->mapToScene(e->pos());
                        pdf->syncPageToThumbnail(sp);
                        return true;
                    }

                    if (obj == pdf->viewer->viewport()) {
                        pdf->dragStart = e->pos();
                        pdf->isDragging = true;
                        return true;
                    }
                }
            }

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

        else if (e->button() == Qt::RightButton) {
            int i = findSlotIndex();
            QMenu menu;
            QAction *closeAct = menu.addAction("Close");
            QAction *selectedAct = menu.exec(QCursor::pos());

            if (selectedAct == closeAct) {
                if (i != -1)
                    removeMedia(i);
                else if (!selectedIndices.empty()) {
                    for (int j = (int)selectedIndices.size() - 1; j >= 0; --j)
                        removeMedia(selectedIndices[j]);
                }
            }
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        auto *e = static_cast<QMouseEvent*>(event);

        if (fullscreenIndex != -1) {
            if(mediaSlots[fullscreenIndex]->type() == "pdf") {
                auto *pdf = static_cast<PdfSlot*>(mediaSlots[fullscreenIndex].get());
                pdf->viewer->setCursor(Qt::ArrowCursor);
                if (e->button() == Qt::LeftButton && pdf->isDragging) {
                    pdf->dragEnd = e->pos();
                    pdf->isDragging = false;

                    if (pdf->dragStart != pdf->dragEnd) {
                        QString text = pdf->getSelectedText();
                        qDebug() << text;
                        QApplication::clipboard()->setText(text);
                    } else {
                        pdf->processLinks(e->pos());
                    }
                }
            }
        }
    }

    if (event->type() == QEvent::Resize) {
        if (fullscreenIndex == -1) {
            for (int i = 0; i < (int)mediaSlots.size(); ++i) {
                if (mediaSlots[i]->type() == "image" && mediaSlots[i]->wrapper == obj) {
                    auto *img = static_cast<ImageSlot*>(mediaSlots[i].get());
                    img->viewer->fitInView(img->item, Qt::KeepAspectRatio);
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

                if (mediaSlots[i]->type() == "video") {
                    auto *vid = static_cast<VideoSlot*>(mediaSlots[i].get());
                    if (obj == vid->wrapper) {
                        int sliderH = vid->slider->isVisible() ? vid->slider->height() : 0;
                        vid->externalSubtitleLabel->setGeometry(
                            3,
                            vid->wrapper->height() - sliderH - 80 - 3,
                            vid->wrapper->width() - 6,
                            60
                            );
                        vid->externalSubtitleLabel->raise();
                    }
                }

                if (mediaSlots[i]->wrapper == obj) {
                    mediaSlots[i]->border->setGeometry(mediaSlots[i]->wrapper->rect());
                    break;
                }
            }
        }
        else {
            if (mediaSlots[fullscreenIndex]->type() == "pdf" && mediaSlots[fullscreenIndex]->wrapper == obj) {
                auto *pdf = static_cast<PdfSlot*>(mediaSlots[fullscreenIndex].get());
                int findBarH = pdf->findBar->sizeHint().height();
                if (pdf->findBar->isVisible()) {
                    pdf->findBar->setGeometry(0, pdf->wrapper->height() - findBarH,
                                              pdf->findBar->maximumWidth(), findBarH);
                }

                if (pdf->sidePanel->isVisible()) {
                    QRect r = pdf->wrapper->rect();
                    int navH = pdf->navBar->sizeHint().height();
                    int actualBarH = pdf->findBar->isVisible() ? findBarH : 0;
                    pdf->sidePanel->setGeometry(3, navH + 2, r.width() / 5,
                                                r.height() - navH - 4 - actualBarH);
                }

                pdf->border->setGeometry(pdf->wrapper->rect());
                return true;
            }
            mediaSlots[fullscreenIndex]->wrapper->setGeometry(container->rect());
            mediaSlots[fullscreenIndex]->border->setGeometry(mediaSlots[fullscreenIndex]->wrapper->rect());
        }
    }



    if (event->type() == QEvent::KeyPress) {
        auto *e = static_cast<QKeyEvent*>(event);

        if (e->key() == Qt::Key_Space) {
            for(int i = 0; i < (int)selectedIndices.size(); ++i) {
                auto state = mediaSlots[selectedIndices[i]]->getPlayerState();
                if (state == QMediaPlayer::PlayingState)
                    mediaSlots[selectedIndices[i]]->pause();
                else
                    mediaSlots[selectedIndices[i]]->play();
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
            if (e->key() == Qt::Key_F && e->modifiers() & Qt::ControlModifier) {
                if (fullscreenIndex != -1 && mediaSlots[fullscreenIndex]->type() == "pdf") {
                    auto *pdf = static_cast<PdfSlot*>(mediaSlots[fullscreenIndex].get());
                    pdf->enableSearch(true);
                }
            }
            else if(e->key() == Qt::Key_F && !e->modifiers()) {
                if (selectedIndices.empty()) return true;
                if (fullscreenIndex == -1) {
                    enterFullscreen(selectedIndices[0]);
                    // mediaSlots[fullscreenIndex]->toggleMediaControls(true);
                }
                else exitFullscreen();
            }
        }

        if(e->key() == Qt::Key_Escape) {
            if (fullscreenIndex != -1 && mediaSlots[fullscreenIndex]->type() == "pdf") {
                auto *pdf = static_cast<PdfSlot*>(mediaSlots[fullscreenIndex].get());
                pdf->enableSearch(false);
            }
            else if (fullscreenIndex != -1) exitFullscreen();

        }


        if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down || e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) {
            bool ctrl = e->modifiers() & Qt::ControlModifier;
            int vertValue = e->key() == Qt::Key_Up ? 1 : e->key() == Qt::Key_Down ? -1 : 0;
            int horValue  = e->key() == Qt::Key_Right ? 1 : e->key() == Qt::Key_Left ? -1 : 0;
            int n = static_cast<int>(mediaSlots.size());
            int cols = static_cast<int>(std::ceil(std::sqrt(n)));

            if (selectedIndices.empty()) selectedIndices.emplace_back(0);
            int oldIndex = selectedIndices[0];
            int row = oldIndex / cols;
            int col = oldIndex % cols;
            row -= vertValue;
            col += horValue;
            int newIndex = row * cols + col;

            if (fullscreenIndex != -1) {
                if (ctrl) {
                    if (newIndex >= 0 && newIndex < n) {
                        selectedIndices[0] = newIndex;
                        exitFullscreen();
                        enterFullscreen(selectedIndices[0]);
                    }
                }
                else {
                    if (vertValue != 0) {
                        if (mediaSlots[fullscreenIndex]->type() == "pdf")
                            mediaSlots[fullscreenIndex]->scroll(vertValue * -40);
                        else {
                            mediaSlots[fullscreenIndex]->adjustVolume(vertValue * 0.05f);
                            volSlider->setValue(mediaSlots[fullscreenIndex]->getVolume() * 100);
                        }
                    }
                    if (horValue != 0)
                        horValue > 0 ? mediaSlots[fullscreenIndex]->forward() : mediaSlots[fullscreenIndex]->backward();
                }
            }
            else if (fullscreenIndex == -1 && !ctrl) {
                if (newIndex >= 0 && newIndex < n) {
                    mediaSlots[oldIndex]->toggleMediaControls(false);
                    selectedIndices[0] = newIndex;
                    mediaSlots[newIndex]->toggleMediaControls(true);
                }
                highlight();
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

        if(e->key() == Qt::Key_C) {
            toggleSettings();
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

void MainWindow::toggleSettings() {
    if (fullscreenIndex != -1) {

        if(!settings->isVisible()) {
            QRect r = container->rect();

            QPoint topRightGlobal =
                container->mapToGlobal(QPoint(r.width(), 0));

            int settingsWidth = r.width() / 4;

            settings->setGeometry(
                topRightGlobal.x() - settingsWidth,
                topRightGlobal.y(),
                settingsWidth,
                r.height()
                );
            settings->setVisible(true);
            mediaSlots[fullscreenIndex]->showSettings(settings);
        }
        else settings->setVisible(false);
    }
}

void MainWindow::enterFullscreen(int index)
{
    if (mediaSlots[index]->type() != "pdf" || mediaSlots[index]->type() != "image") action->setVisible(true);
    else action->setVisible(false);
    ui->toolBar_2->update();
    fullscreenIndex = index;

    if (mediaSlots[index]->type() == "pdf") {
        auto *pdf = static_cast<PdfSlot*>(mediaSlots[index].get());
        pdf->navBar->show();
    }

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
            if (mediaSlots[i]->type() == "video") {
                auto *vid = static_cast<VideoSlot*>(mediaSlots[i].get());
                vid->subtitleOverlay->hide();
            }
        }
    }
}

void MainWindow::exitFullscreen() {
    if (fullscreenIndex != -1) {
        if (fullscreenIndex != -1 && mediaSlots[fullscreenIndex]->type() == "pdf") {
            auto *pdf = static_cast<PdfSlot*>(mediaSlots[fullscreenIndex].get());
            pdf->navBar->hide();
            pdf->sidePanel->hide();
            pdf->findBar->hide();
        }
        settings->hide();

        action->setVisible(false);
        ui->toolBar_2->update();
        mediaSlots[fullscreenIndex]->wrapper->setParent(container);
        rebuildGrid();
        for (int i = 0; i < (int)playingIndices.size(); ++i) mediaSlots[playingIndices[i]]->play();
        playingIndices.clear();
        if(settings->isVisible()) settings->setVisible(false);
        fullscreenIndex = -1;
    }
}


void MainWindow::addMedia(const QString &path) {
    auto slot = makeSlot(path, container, this);
    if (!slot) return;
    mediaSlots.push_back(std::move(slot));
    //if (fullscreenIndex == -1) rebuildGrid();
    if(fullscreenIndex != -1) {
        exitFullscreen();
        return;
    }
    rebuildGrid();
}

void MainWindow::rebuildGrid() {
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

    QTimer::singleShot(0, container, [this]() {
        for (auto &s : mediaSlots)
            s->border->setGeometry(s->wrapper->rect());
        highlight();
    });
}


void MainWindow::highlight() {
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



void MainWindow::removeMedia(int index) {
    if (index < 0 || index >= static_cast<int>(mediaSlots.size())) return;
    if (fullscreenIndex != -1) exitFullscreen();

    auto* w = mediaSlots[index]->wrapper;
    grid->removeWidget(w);
    w->hide();
    mediaSlots.erase(mediaSlots.begin() + index);
    w->deleteLater();

    selectedIndices.erase(
        std::remove_if(selectedIndices.begin(), selectedIndices.end(),
                       [index](int i) { return i == index; }),
        selectedIndices.end()
        );
    for (int &i : selectedIndices)
        if (i > index) --i;

    if (hoveredIndex == index) hoveredIndex = -1;
    else if (hoveredIndex > index) --hoveredIndex;

    if (fullscreenIndex > index) --fullscreenIndex;

    if (volSlider->isVisible()) volSlider->setVisible(false);
    container->update();
    rebuildGrid();
}
