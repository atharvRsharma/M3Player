#include <mediatypes.h>

#include <QFileInfo>
#include <QMediaDevices>
#include <QGridLayout>
#include <QMenu>
#include <QMediaMetaData>

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


void VideoSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {
    wrapper = new QWidget(parent);
    video = new QVideoWidget(wrapper);
    slider = new QSlider(Qt::Horizontal, wrapper);
    player = new QMediaPlayer(wrapper);
    audio = new QAudioOutput(wrapper);



    auto *layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(video);
    layout->addWidget(slider);

    player->setAudioOutput(audio);
    player->setVideoOutput(video);
    audio->setVolume(1.0f);

    slider->setRange(0, 0);
    slider->setFocusPolicy(Qt::NoFocus);
    video->setFocusPolicy(Qt::NoFocus);
    video->setAcceptDrops(true);
    video->installEventFilter(thisInstance);

    for (QObject *child : video->findChildren<QObject*>())
        static_cast<QWidget*>(child)->installEventFilter(thisInstance);

    // wrapper->setContextMenuPolicy(Qt::NoContextMenu);
    wrapper->setAcceptDrops(true);
    wrapper->installEventFilter(thisInstance);
    wrapper->setAttribute(Qt::WA_Hover);


    slider->setEnabled(false);
    QObject::connect(player, &QMediaPlayer::durationChanged, slider, &QSlider::setMaximum);
    QObject::connect(player, &QMediaPlayer::positionChanged, slider, &QSlider::setValue);
    QObject::connect(player, &QMediaPlayer::seekableChanged, slider, &QSlider::setEnabled);
    QObject::connect(slider, &QSlider::sliderMoved, player, &QMediaPlayer::setPosition);


    player->setSource(QUrl::fromLocalFile(path));
    player->play();
}

void AudioSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {
    wrapper = new QWidget(parent);
    player = new QMediaPlayer(wrapper);
    audio = new QAudioOutput(wrapper);
    slider = new QSlider(Qt::Horizontal, wrapper);
    cover = new QLabel(wrapper);
    title = new QLabel(wrapper);
    artist = new QLabel(wrapper);

    auto *layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(cover);
    layout->addWidget(title);
    layout->addWidget(artist);
    layout->addWidget(slider);

    player->setAudioOutput(audio);

    audio->setVolume(1.0f);
    slider->setRange(0, 0);
    slider->setFocusPolicy(Qt::NoFocus);

    cover->setFocusPolicy(Qt::NoFocus);
    cover->setAcceptDrops(true);
    cover->installEventFilter(thisInstance);

    for (QObject *child : cover->findChildren<QObject*>())
        static_cast<QWidget*>(child)->installEventFilter(thisInstance);

    // wrapper->setContextMenuPolicy(Qt::NoContextMenu);
    wrapper->setAcceptDrops(true);
    wrapper->installEventFilter(thisInstance);

    wrapper->setAttribute(Qt::WA_Hover);

    slider->setEnabled(false);
    QObject::connect(player, &QMediaPlayer::durationChanged, slider, &QSlider::setMaximum);
    QObject::connect(player, &QMediaPlayer::positionChanged, slider, &QSlider::setValue);
    QObject::connect(player, &QMediaPlayer::seekableChanged, slider, &QSlider::setEnabled);
    QObject::connect(slider, &QSlider::sliderMoved, player, &QMediaPlayer::setPosition);

    QObject::connect(player, &QMediaPlayer::metaDataChanged, [this]{
        if(QVariant thumbnail = player->metaData().value(QMediaMetaData::ThumbnailImage); !thumbnail.isNull()) {
            cover->setPixmap(QPixmap::fromImage(thumbnail.value<QImage>()).scaled(200, 200, Qt::KeepAspectRatio));
        }

        if(QString titleName = player->metaData().stringValue(QMediaMetaData::Title); !titleName.isEmpty()) {
            title->setText(titleName);
        }

        if(QString artistName = player->metaData().stringValue(QMediaMetaData::AlbumArtist); !artistName.isEmpty()) {
            artist->setText(artistName);
        }
    });

    player->setSource(QUrl::fromLocalFile(path));
    player->play();
}

void ImageSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {
    wrapper = new QWidget(parent);
    imageLabel = new QLabel(wrapper);
    pixmap = QPixmap(path);

    auto *layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(imageLabel);

    imageLabel->setPixmap(pixmap.scaled(wrapper->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    imageLabel->setScaledContents(false);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    wrapper->setAcceptDrops(true);
    wrapper->installEventFilter(thisInstance);
    wrapper->setAttribute(Qt::WA_Hover);
}

void PdfSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {
    wrapper = new QWidget(parent);
    view = new QGraphicsView(wrapper);
    scene = new QGraphicsScene(view);
    auto *layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(view);
    document = Poppler::Document::load(path);
    if (!document || document->isLocked() || document->isEncrypted()) {
        return;
    }
    int pageCt = document->numPages();
    qreal yOffset = 0;
    for (int i{}; i < pageCt; ++i) {
        std::unique_ptr<Poppler::Page> p = document->page(i);
        QImage img = p->renderToImage(300);
        QPixmap pix = QPixmap::fromImage(img);
        auto *item = new QGraphicsPixmapItem(pix);
        item->setPos(0, yOffset);
        scene->addItem(item);
        yOffset += pix.height() + 10;
    }
    view->setScene(scene);
    wrapper->setAcceptDrops(true);
    wrapper->installEventFilter(thisInstance);
    wrapper->setAttribute(Qt::WA_Hover);
}
