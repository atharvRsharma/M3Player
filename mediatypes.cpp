#include <mediatypes.h>

#include <QtMultimediaWidgets/QVideoWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSlider>
#include <QFileInfo>
#include <QMediaDevices>
#include <QLabel>
#include <QGridLayout>
#include <QMenu>
#include <QMediaMetaData>
#include <QPdfView>
#include <QPdfPageSelector>
#include <QLineEdit>
#include <QPdfDocument>
#include <QGraphicsView>
#include <QPdfSearchModel>
#include <QGraphicsWidget>
#include <QPdfBookmarkModel>
#include <QPdfPageNavigator>
#include <QListView>
#include <QScrollBar>
#include <QComboBox>
#include <QLineEdit>
#include <QCoreApplication>


std::unique_ptr<MediaSlot> makeSlot(const QString &path, QWidget *parent, QObject *thisInstance) {
    static const QStringList vid = {"mp4", "mkv", "avi", "mov"};
    static const QStringList aud = {"mp3", "m4a", "flac", "ogg", "wav"};
    static const QStringList img = {"png","jpg","jpeg","webp","gif"};
    static const QStringList pdf = {"pdf"};

    QString ext = QFileInfo(path).suffix().toLower();

    if (vid.contains(ext)) {
        auto slot = std::make_unique<VideoSlot>();
        slot->load(path, parent, thisInstance);
        return slot;
    }
    if (aud.contains(ext)) {
        auto slot = std::make_unique<AudioSlot>();
        slot->load(path, parent, thisInstance);
        return slot;
    }
    if (img.contains(ext)) {
        auto slot = std::make_unique<ImageSlot>();
        slot->load(path, parent, thisInstance);
        return slot;
    }

    if (pdf.contains(ext)) {
        auto slot = std::make_unique<PdfSlot>();
        slot->load(path, parent, thisInstance);
        return slot;
    }

    return nullptr;
}


//=============================================================================================================
//=============================================================================================================

//VIDEOVIDEOVIDEO======================================================================================================================

void VideoSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {
    wrapper = new QWidget(parent);
    settings = new QWidget(wrapper);
    video = new QVideoWidget(wrapper);
    slider = new QSlider(Qt::Horizontal, wrapper);
    player = new QMediaPlayer(wrapper);
    audio = new QAudioOutput(wrapper);
    border = new QWidget(wrapper);
    subtitleTracks = new QComboBox(wrapper);
    audioTracks = new QComboBox(wrapper);
    videoTracks = new QComboBox(wrapper);



    auto *layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(3,3,3,3);
    layout->addWidget(video);
    layout->addWidget(slider);
    layout->addWidget(subtitleTracks);
    layout->addWidget(videoTracks);
    layout->addWidget(audioTracks);
    layout->addWidget(settings);

    slider->hide();

    player->setAudioOutput(audio);
    player->setVideoOutput(video);
    audio->setVolume(1.0f);

    slider->setRange(0, 0);
    slider->setFocusPolicy(Qt::NoFocus);
    video->setFocusPolicy(Qt::NoFocus);
    video->setAcceptDrops(true);
    video->installEventFilter(thisInstance);
    slider->hide();

    for (QObject *child : video->findChildren<QObject*>())
        static_cast<QWidget*>(child)->installEventFilter(thisInstance);

    wrapper->setAcceptDrops(true);
    wrapper->installEventFilter(thisInstance);
    wrapper->setAttribute(Qt::WA_Hover);

    slider->setEnabled(false);

    border->setAttribute(Qt::WA_TransparentForMouseEvents);
    border->setGeometry(wrapper->rect());
    border->raise();

    // settings->setGeometry()


    QObject::connect(player, &QMediaPlayer::durationChanged, slider, &QSlider::setMaximum);
    QObject::connect(player, &QMediaPlayer::positionChanged, slider, &QSlider::setValue);
    QObject::connect(player, &QMediaPlayer::seekableChanged, slider, &QSlider::setEnabled);
    QObject::connect(slider, &QSlider::sliderMoved, player, &QMediaPlayer::setPosition);
    // QObject::connect(player, &QMediaPlayer::seekableChanged, thisInstance, [this] (int sec) {
    //     seek(sec);
    // });

    QObject::connect(player, &QMediaPlayer::tracksChanged, thisInstance, [this]() { updateTracks(); });

    QObject::connect(subtitleTracks, &QComboBox::currentIndexChanged, thisInstance, [this](int stream)
                     { selectSubtitleStream(stream); });

    QObject::connect(videoTracks, &QComboBox::currentIndexChanged, thisInstance, [this](int stream)
                     { selectVideoStream(stream); });

    QObject::connect(audioTracks, &QComboBox::currentIndexChanged, thisInstance, [this](int stream)
                     { selectAudioStream(stream); });


    player->setSource(QUrl::fromLocalFile(path));

    player->pause();
}


void VideoSlot::play() { player->play(); }

void VideoSlot::pause() { player->pause(); }

void VideoSlot::stop() { player->stop(); }

void VideoSlot::replay() { player->setPosition(0); player->play(); }

void VideoSlot::toggleMediaControls(bool x) {
    slider->setVisible(x);
}

void VideoSlot::toggleMute() {
    if (!audio->isMuted()) audio->setMuted(true);
    else audio->setMuted(false);
}

void VideoSlot::setVolume(float x) {
    audio->setVolume(audio->volume() + x);
}

void VideoSlot::forward() {
    qint64 current = player->position();

    if (current + seekStep < player->duration()) {
        player->setPosition(player->position() + seekStep);
    }

    else {
        VideoSlot::stop();
    }
}

float VideoSlot::getVolume() const  { return currentVolume; }

void VideoSlot::backward() {
    qint64 current = player->position();

    if (current - seekStep > 0) {
        player->setPosition(player->position() - seekStep);
    }

    else {
        VideoSlot::replay();
    }
}

void VideoSlot::adjustVolume(float delta) {
    currentVolume = std::clamp(currentVolume + delta, 0.0f, 1.0f);
    audio->setVolume(currentVolume);
}

void VideoSlot::selectSubtitleStream(int stream)
{
    stream = subtitleTracks->currentData().toInt();
    player->setActiveSubtitleTrack(stream);
}

void VideoSlot::selectVideoStream(int stream)
{
    stream = videoTracks->currentData().toInt();
    player->setActiveVideoTrack(stream);
}

void VideoSlot::selectAudioStream(int stream)
{
    stream = audioTracks->currentData().toInt();
    player->setActiveAudioTrack(stream);
}

void VideoSlot::updateTracks() {
    subtitleTracks->clear();
    audioTracks->clear();
    videoTracks->clear();

    const auto subTracks = player->subtitleTracks();
    const auto audTracks = player->audioTracks();
    const auto vidTracks = player->videoTracks();

    for (int i = 0; i < vidTracks.size(); ++i)
        videoTracks->addItem(trackName(vidTracks.at(i), i), i);
    videoTracks->setCurrentIndex(player->activeVideoTrack() + 1);

    for (int i = 0; i < audTracks.size(); ++i)
        audioTracks->addItem(trackName(audTracks.at(i), i), i);
    audioTracks->setCurrentIndex(player->activeAudioTrack() + 1);

    for (int i = 0; i < subTracks.size(); ++i)
        subtitleTracks->addItem(trackName(subTracks.at(i), i), i);
    subtitleTracks->setCurrentIndex(player->activeSubtitleTrack() + 1);
}

QString VideoSlot::trackName(const QMediaMetaData &metaData, int index) {
    QString name;
    QString title = metaData.stringValue(QMediaMetaData::Title);
    QLocale::Language lang = metaData.value(QMediaMetaData::Language).value<QLocale::Language>();
    using namespace Qt::StringLiterals;

    if (title.isEmpty()) {
        if (lang == QLocale::Language::AnyLanguage)
            name = QCoreApplication::translate("VideoSlot", "Track %1").arg(index + 1);
        else
            name = QLocale::languageToString(lang);
    } else {
        if (lang == QLocale::Language::AnyLanguage)
            name = title;
        else
            name = title + " - ["_L1 + QLocale::languageToString(lang) + u']';
    }
    return name;
}

void VideoSlot::seek(int sec) {
    player->setPosition(sec);
}

//\\VIDEOVIDEOVIDEO=======================================================================================================

//AUDIOAUDIOAUDIO====================================================================================================================

void AudioSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {
    wrapper      = new QWidget(parent);
    player       = new QMediaPlayer(wrapper);
    audio        = new QAudioOutput(wrapper);
    slider       = new QSlider(Qt::Horizontal, wrapper);
    cover        = new QLabel(wrapper);
    title        = new QLabel(wrapper);
    artist       = new QLabel(wrapper);
    overlay      = new QWidget(wrapper);
    border       = new QWidget(wrapper);

    auto *layout = new QVBoxLayout(wrapper);
    auto *metaDataLayout = new QVBoxLayout(overlay);

    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(cover);
    metaDataLayout->addWidget(title);
    metaDataLayout ->addWidget(artist);
    layout->addWidget(slider);

    title->hide();
    artist->hide();
    slider->hide();

    player->setAudioOutput(audio);

    audio->setVolume(1.0f);
    slider->setRange(0, 0);
    slider->setFocusPolicy(Qt::NoFocus);

    cover->setFocusPolicy(Qt::NoFocus);
    cover->setAcceptDrops(true);
    cover->setAlignment(Qt::AlignCenter);
    cover->installEventFilter(thisInstance);
    cover->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    for (QObject *child : cover->findChildren<QObject*>())
        static_cast<QWidget*>(child)->installEventFilter(thisInstance);

    wrapper->setAcceptDrops(true);
    wrapper->installEventFilter(thisInstance);
    wrapper->setAttribute(Qt::WA_Hover);

    slider->setEnabled(false);

    border->setAttribute(Qt::WA_TransparentForMouseEvents);
    border->raise();

    int sliderHeight = slider->sizeHint().height();
    int overlayHeight = 60;
    overlay->setGeometry(0, wrapper->height() - overlayHeight - sliderHeight, wrapper->width(), overlayHeight);
    overlay->raise();


    QObject::connect(player, &QMediaPlayer::durationChanged, slider, &QSlider::setMaximum);
    QObject::connect(player, &QMediaPlayer::positionChanged, slider, &QSlider::setValue);
    QObject::connect(player, &QMediaPlayer::seekableChanged, slider, &QSlider::setEnabled);
    QObject::connect(slider, &QSlider::sliderMoved, player, &QMediaPlayer::setPosition);

    QObject::connect(player, &QMediaPlayer::metaDataChanged, [this]{
        if(QVariant thumbnail = player->metaData().value(QMediaMetaData::ThumbnailImage); !thumbnail.isNull()) {
            coverImage = thumbnail.value<QImage>();
            cover->setPixmap(QPixmap::fromImage(coverImage).scaled(cover->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }

        if(QString titleName = player->metaData().stringValue(QMediaMetaData::Title); !titleName.isEmpty()) {
            title->setText(titleName);
        }

        if(QString artistName = player->metaData().stringValue(QMediaMetaData::ContributingArtist); !artistName.isEmpty()) {
            artist->setText("   " + artistName);
        }
    });

    player->setSource(QUrl::fromLocalFile(path));
    player->pause();
}

void AudioSlot::play() { player->play(); }

void AudioSlot::pause() { player->pause(); }

void AudioSlot::stop() { player->stop(); }

void AudioSlot::replay() { player->setPosition(0); player->play(); }

void AudioSlot::toggleMute() {
    if (!audio->isMuted()) audio->setMuted(true);
    else audio->setMuted(false);
}

void AudioSlot::toggleMediaControls(bool x) {
    slider->setVisible(x);
    artist->setVisible(x);
    title->setVisible(x);
    int sliderHeight = x ? slider->sizeHint().height() : 0;
    int overlayHeight = 60;
    overlay->setGeometry(0, wrapper->height() - overlayHeight - sliderHeight, wrapper->width(), overlayHeight);
}

void AudioSlot::setVolume(float x) {
    audio->setVolume(x);
}

void AudioSlot::forward() {
    qint64 current = player->position();

    if (current + seekStep < player->duration()) {
        player->setPosition(player->position() + seekStep);
    }

    else {
        AudioSlot::stop();
    }
}

void AudioSlot::backward() {
    qint64 current = player->position();

    if (current - seekStep > 0) {
        player->setPosition(player->position() - seekStep);
    }

    else{
        AudioSlot::replay();
    }
}

float AudioSlot::getVolume() const  { return currentVolume; }

void AudioSlot::adjustVolume(float delta) {
    currentVolume = std::clamp(currentVolume + delta, 0.0f, 1.0f);
    audio->setVolume(currentVolume);
}

void AudioSlot::seek(int sec) {
    player->setPosition(sec);
}

//\\AUDIOAUDIOAUDIO=======================================================================================================

//IMAGEIMAGEIMAGE=================================================================================================================

void ImageSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {
    wrapper      = new QWidget(parent);
    pixmap       = QPixmap(path);
    border       = new QWidget(wrapper);
    viewer       = new QGraphicsView(wrapper);
    scene        = new QGraphicsScene(viewer);
    item         = new QGraphicsPixmapItem(pixmap);
    auto *layout = new QVBoxLayout(wrapper);

    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(viewer);

    item->setPos(0, 0);
    scene->addItem(item);

    viewer->setDragMode(QGraphicsView::ScrollHandDrag);

    viewer->setAcceptDrops(true);
    viewer->installEventFilter(thisInstance);
    viewer->viewport()->installEventFilter(thisInstance);
    viewer->viewport()->setAcceptDrops(true);

    viewer->setScene(scene);
    viewer->setAlignment(Qt::AlignTop);
    viewer->setTransformationAnchor(QGraphicsView::AnchorViewCenter);

    QRectF contentRect = scene->itemsBoundingRect();
    viewer->setSceneRect(contentRect.adjusted(-5000, -1000, 5000, 1000));


    wrapper->setAcceptDrops(true);
    wrapper->installEventFilter(thisInstance);
    wrapper->setAttribute(Qt::WA_Hover);

    border->setAttribute(Qt::WA_TransparentForMouseEvents);
    border->setGeometry(wrapper->rect());
    border->raise();
}

void ImageSlot::zoom(qreal x)  {
    qreal newZoom = zoomFactor * x;
    if (newZoom > 10.0) return;
    zoomFactor = newZoom;
    viewer->scale(x, x);
}

//\\IMAGEIMAGEIMAGE==================================================================================================================

//PDFPDFPDPFPDFPDF===NORMAL==============================================================================================================

void PdfSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {
    wrapper         = new QWidget(parent);
    viewer          = new QPdfView(wrapper);
    doc             = new QPdfDocument(wrapper);
    border          = new QWidget(wrapper);
    pageSelector    = new QPdfPageSelector(wrapper);
    searchModel     = new QPdfSearchModel(wrapper);
    //searchField     = new QLineEdit(wrapper);
    bookmarkModel   = new QPdfBookmarkModel(wrapper);
    zoomSelector    = new QComboBox(wrapper);


    auto *layout    = new QVBoxLayout(wrapper);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(viewer);
    layout->addWidget(pageSelector);
    layout->addWidget(zoomSelector);
    pageSelector->setVisible(false);
    //layout->addWidget(searchField);

    wrapper->setAcceptDrops(true);
    wrapper->installEventFilter(thisInstance);
    wrapper->setAttribute(Qt::WA_Hover);

    zoomSelector->setEditable(true);
    zoomSelector->addItem(QLatin1String("Fit Width"));
    zoomSelector->addItem(QLatin1String("Fit Page"));
    zoomSelector->addItem(QLatin1String("12%"));
    zoomSelector->addItem(QLatin1String("25%"));
    zoomSelector->addItem(QLatin1String("33%"));
    zoomSelector->addItem(QLatin1String("50%"));
    zoomSelector->addItem(QLatin1String("66%"));
    zoomSelector->addItem(QLatin1String("75%"));
    zoomSelector->addItem(QLatin1String("100%"));
    zoomSelector->addItem(QLatin1String("125%"));
    zoomSelector->addItem(QLatin1String("150%"));
    zoomSelector->addItem(QLatin1String("200%"));
    zoomSelector->addItem(QLatin1String("400%"));


    viewer->viewport()->installEventFilter(thisInstance);
    viewer->viewport()->setAcceptDrops(true);

    border->setAttribute(Qt::WA_TransparentForMouseEvents);
    border->setGeometry(wrapper->rect());
    border->raise();

    if (!doc) {
        return;
    }
    doc->load(path);

    viewer->setDocument(doc);
    bookmarkModel->setDocument(doc);
    pageSelector->setDocument(doc);
    searchModel->setDocument(doc);
    nav = viewer->pageNavigator();
    viewer->setPageMode(QPdfView::PageMode::MultiPage);
    viewer->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    viewer->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    //QObject::connect(pageSelector, &QPdfPageSelector::currentPageChanged, thisInstance, &PdfSlot::goTo);
    //const auto documentTitle = doc->metaData(QPdfDocument::MetaDataField::Title).toString();
    QObject::connect(pageSelector, &QPdfPageSelector::currentPageChanged, thisInstance,
                    [this](int page) {
                         nav->jump(page, {}, nav->currentZoom());
                    });

    QObject::connect(zoomSelector, &QComboBox::currentTextChanged, thisInstance,
                     [this](const QString &text) {
                         if (text == QLatin1String("Fit Width")) {
                            viewer->setZoomMode(QPdfView::ZoomMode::FitToWidth);
                         } else if (text == QLatin1String("Fit Page")) {
                            viewer->setZoomMode(QPdfView::ZoomMode::FitInView);
                         } else {
                            factor = 1.0;

                            QString withoutPercent(text);
                            withoutPercent.remove(QLatin1Char('%'));

                            bool ok = false;
                            const int zoomLevel = withoutPercent.toInt(&ok);
                            if (ok)
                                factor = zoomLevel / 100.0;

                            viewer->setZoomMode(QPdfView::ZoomMode::Custom);
                            viewer->setZoomFactor(factor);
                         }
                     });


}

void PdfSlot::zoom(qreal x) {
    viewer->setZoomMode(QPdfView::ZoomMode::Custom);
    viewer->setZoomFactor(viewer->zoomFactor() * x);
}

void PdfSlot::forward() {
    nav->jump(nav->currentPage() + 1, {}, nav->currentZoom());
}

void PdfSlot::backward() {
    nav->jump(nav->currentPage() - 1, {}, nav->currentZoom());
}

void PdfSlot::scroll(int x) {
    viewer->verticalScrollBar()->setValue( viewer->verticalScrollBar()->value() + (x));
}

void PdfSlot::toggleMediaControls(bool x) {
    pageSelector->setVisible(x);
}

void PdfSlot::undo() {
    viewer->pageNavigator()->back();
}


void PdfSlot::redo() {
    viewer->pageNavigator()->forward();
}

void PdfSlot::reset() {
    zoomSelector->setCurrentIndex(8);
}

//\\PDFPDFPDPFPDFPDF===NORMAL==============================================================================================================



//PDFPDFPDPFPDFPDF===MINIMAL==============================================================================================================

// void PdfSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {    minimal
//     wrapper      = new QWidget(parent);
//     viewer       = new QGraphicsView(wrapper);
//     scene        = new QGraphicsScene(viewer);
//     doc          = new QPdfDocument(wrapper);
//     border       = new QWidget(wrapper);

//     pageSelector = new QPdfPageSelector(wrapper);
//     searchModel = new QPdfSearchModel(wrapper);
//     searchField = new QLineEdit(wrapper);
//     //renderer     = new QPdfPageRenderer(wrapper);
//     auto *layout = new QVBoxLayout(wrapper);

//     layout->setContentsMargins(0,0,0,0);
//     layout->addWidget(viewer);

//     if (!doc) {
//         return;
//     }

//     doc->load(path);
//     // renderer->setDocument(doc);
//     // renderer->setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);

//     QPdfDocumentRenderOptions opts;
//     opts.setRenderFlags(QPdfDocumentRenderOptions::RenderFlag::Annotations);
//     //renderer->requestPage(0, QSize(800, 1000));

//     //qDebug() << "page count:" << doc->pageCount() << "status:" << doc->status();
//     int pageCt = doc->pageCount();

//     qreal yOffset = 0;

//     for (int i{}; i < pageCt; ++i) {
//         QSizeF pageSize = doc->pagePointSize(i);
//         QSize renderSize = (pageSize * 2.0).toSize();

//         auto img = doc->render(i, renderSize, opts);
//         QImage filledImg(renderSize, QImage::Format_RGB32);
//         filledImg.fill(Qt::white);
//         QPainter p(&filledImg);
//         p.drawImage(0, 0, img);
//         p.end();
//         QPixmap pix = QPixmap::fromImage(filledImg);
//         auto *item = new QGraphicsPixmapItem(pix);
//         item->setPos(0, yOffset);
//         scene->addItem(item);
//         yOffset += pix.height() + 10;
//     }

//     //viewer->setSceneRect(QRectF());

//     viewer->setDragMode(QGraphicsView::ScrollHandDrag);

//     viewer->setAcceptDrops(true);
//     viewer->installEventFilter(thisInstance);
//     viewer->viewport()->installEventFilter(thisInstance);
//     viewer->viewport()->setAcceptDrops(true);
//     wrapper->setAcceptDrops(true);
//     wrapper->installEventFilter(thisInstance);
//     wrapper->setAttribute(Qt::WA_Hover);
//     viewer->setScene(scene);
//     viewer->setBackgroundBrush(Qt::black);
//     viewer->setAlignment(Qt::AlignTop);
//     viewer->setTransformationAnchor(QGraphicsView::AnchorViewCenter);

//     QRectF contentRect = scene->itemsBoundingRect();
//     viewer->setSceneRect(contentRect.adjusted(-5000, -500, 5000, 500));

//     border->setAttribute(Qt::WA_TransparentForMouseEvents);
//     border->setGeometry(wrapper->rect());
//     border->raise();
// }

