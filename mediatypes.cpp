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
#include <QPdfPageSelector>
#include <QLineEdit>
#include <QPdfDocument>
#include <QGraphicsView>
#include <QGraphicsWidget>




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
    video = new QVideoWidget(wrapper);
    slider = new QSlider(Qt::Horizontal, wrapper);
    player = new QMediaPlayer(wrapper);
    audio = new QAudioOutput(wrapper);
    border = new QWidget(wrapper);



    auto *layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(3,3,3,3);
    layout->addWidget(video);
    layout->addWidget(slider);

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

    QObject::connect(player, &QMediaPlayer::durationChanged, slider, &QSlider::setMaximum);
    QObject::connect(player, &QMediaPlayer::positionChanged, slider, &QSlider::setValue);
    QObject::connect(player, &QMediaPlayer::seekableChanged, slider, &QSlider::setEnabled);
    QObject::connect(slider, &QSlider::sliderMoved, player, &QMediaPlayer::setPosition);


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

//VIDEOVIDEOVIDEO=======================================================================================================

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

//AUDIOAUDIOAUDIO=======================================================================================================


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


//IMAGEIMAGEIMAGE==================================================================================================================


//PDFPDFPDPFPDFPDF===NORMAL==============================================================================================================

void PdfSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {

}

//PDFPDFPDPFPDFPDF===NORMAL==============================================================================================================


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

