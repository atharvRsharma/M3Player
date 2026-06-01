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
#include <QFontMetrics>
#include <QPainter>
#include <QPdfSearchModel>
#include <QStyledItemDelegate>
#include <QToolBar>
#include <QItemSelectionModel>
#include <QTimer>
#include <QPushButton>
#include <QSpinBox>
#include <QTreeView>
#include <QPdfPageRenderer>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QtPdf/QPdfSelection>
#include <QPdfLinkModel>
#include <QDesktopServices>
#include <QMimeDatabase>

#include "miniz.h"
//#include <poppler/qt6/poppler-qt6.h>



#ifndef Q_OS_ANDROID
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/unsynchronizedlyricsframe.h>
#include <taglib/mpegfile.h>
#endif


std::unique_ptr<MediaSlot> makeSlot(const QString &path, QWidget *parent, QObject *thisInstance) {


    static const QStringList vid = {"mp4", "mkv", "avi", "mov", "m3u8", "webm"};
    static const QStringList aud = {"mp3", "m4a", "flac", "ogg", "wav"};
    static const QStringList img = {"png","jpg","jpeg","webp","gif"};
    static const QStringList pdf = {"pdf"};
    static const QStringList comic = {"cbz"};

    QString ext = QFileInfo(QUrl(path).path()).suffix().toLower();
    // qDebug() << "makeSlot path:" << path << "| ext:" << ext;

#ifdef Q_OS_ANDROID
    if (ext.isEmpty()) {
        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForUrl(QUrl(path));
        QString name = mime.name();
        qDebug() << "mime:" << name;
        if (name.startsWith("video/"))       ext = "mp4";
        else if (name.startsWith("audio/"))  ext = "mp3";
        else if (name.startsWith("image/"))  ext = "jpg";
        else if (name == "application/pdf")  ext = "pdf";
    }
#endif

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
    if (comic.contains(ext)) {
        auto slot = std::make_unique<ComicSlot>();
        slot->load(path, parent, thisInstance);
        return slot;
    }

    return nullptr;
}

//=============================================================================================================
//=============================================================================================================

//VIDEOVIDEOVIDEO======================================================================================================================

void VideoSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {
    initWidgets(parent);

    subtitleOverlay->setAttribute(Qt::WA_TranslucentBackground);
    subtitleOverlay->setAttribute(Qt::WA_ShowWithoutActivating);

    externalSubtitleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    externalSubtitleLabel->setWordWrap(true);
    externalSubtitleLabel->setStyleSheet("color: white; background: rgba(0,0,0,120); padding: 4px; font-size: 16px;");

    auto *externalSubtitleOverlay = new QVBoxLayout(subtitleOverlay);
    externalSubtitleOverlay->setContentsMargins(0,0,0,0);
    externalSubtitleOverlay->addWidget(externalSubtitleLabel);

    subtitleOverlay->hide();

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
    externalSubtitleLabel->raise();
    border->raise();

    connectSlots(thisInstance);

    QUrl url = path.startsWith("http") ? QUrl(path) : QUrl::fromLocalFile(path);
    player->setSource(url);

    player->pause();
}

void VideoSlot::initWidgets(QWidget *parent) {
    wrapper = new QWidget(parent);
    video = new QVideoWidget(wrapper);
    slider = new QSlider(Qt::Horizontal, wrapper);
    player = new QMediaPlayer(wrapper);
    audio = new QAudioOutput(wrapper);
    border = new QWidget(wrapper);
    subtitleTracks = new QComboBox(wrapper);
    audioTracks = new QComboBox(wrapper);
    videoTracks = new QComboBox(wrapper);

    QWidget *mainWin = parent;
    while (mainWin->parentWidget()) mainWin = mainWin->parentWidget();

    subtitleOverlay = new QWidget(mainWin, Qt::Tool | Qt::FramelessWindowHint);
    externalSubtitleLabel = new QLabel(subtitleOverlay);
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

void VideoSlot::selectSubtitleStream(int index) {
    player->setActiveSubtitleTrack(-1);
    subtitlesEnabled = false;
    subtitleOverlay->hide();
    externalSubtitleLabel->hide();

    if (index == 0) return;

    int data = subtitleTracks->itemData(index).toInt();
    if (data == -99) {
        subtitlesEnabled = true;
    } else {
        player->setActiveSubtitleTrack(data);
    }
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

    subtitleTracks->addItem("off", -1);

    const auto subTracks = player->subtitleTracks();
    const auto audTracks = player->audioTracks();
    const auto vidTracks = player->videoTracks();

    for (int i = 0; i < vidTracks.size(); ++i)
        videoTracks->addItem(trackName(vidTracks.at(i), i), i);

    for (int i = 0; i < audTracks.size(); ++i)
        audioTracks->addItem(trackName(audTracks.at(i), i), i);

    for (int i = 0; i < subTracks.size(); ++i)
        subtitleTracks->addItem(trackName(subTracks.at(i), i), i);

    if (!externalSubPath.isEmpty())
        subtitleTracks->addItem("External: " + QFileInfo(externalSubPath).fileName(), -99);

    if (subtitleTracks->count() == 1) subtitleTracks->addItem("no tracks available", -2);
    if (audioTracks->count() == 0)   audioTracks->addItem("no tracks available");
    if (videoTracks->count() == 0)   videoTracks->addItem("no tracks available");
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

void VideoSlot::showSettings(QWidget* settingsOverlay) {
    if (settingsOverlay->layout()) return;
    auto *layout = new QVBoxLayout(settingsOverlay);
    layout->addWidget(subtitleTracks);
    layout->addWidget(videoTracks);
    layout->addWidget(audioTracks);
}

void VideoSlot::connectSlots(QObject* thisInstance) {
    QObject::connect(player, &QMediaPlayer::durationChanged, slider, &QSlider::setMaximum);
    QObject::connect(player, &QMediaPlayer::positionChanged, slider, &QSlider::setValue);
    QObject::connect(player, &QMediaPlayer::seekableChanged, slider, &QSlider::setEnabled);
    QObject::connect(slider, &QSlider::sliderMoved, player, &QMediaPlayer::setPosition);

    QObject::connect(player, &QMediaPlayer::tracksChanged, thisInstance, [this]() { updateTracks(); });

    QObject::connect(subtitleTracks, &QComboBox::currentIndexChanged, thisInstance, [this](int stream)
                     { selectSubtitleStream(stream); });

    QObject::connect(videoTracks, &QComboBox::currentIndexChanged, thisInstance, [this](int stream)
                     { selectVideoStream(stream); });

    QObject::connect(audioTracks, &QComboBox::currentIndexChanged, thisInstance, [this](int stream)
                     { selectAudioStream(stream); });

    QObject::connect(player, &QMediaPlayer::positionChanged, thisInstance, [this](qint64 pos) {
        if (externalSubtitleLabel) updateExternalSubtitle(pos);
    });

    QObject::connect(slider, &QSlider::sliderPressed, thisInstance, [this]() {
        player->setPosition(slider->sliderPosition());
    });
}



// external sub related fns

void VideoSlot::repositionExternalSubtitleOverlay() {
    if (!subtitlesEnabled || !subtitleOverlay) return;
    QPoint globalPos = video->mapToGlobal(QPoint(0, video->height() - 80));
    subtitleOverlay->setGeometry(globalPos.x(), globalPos.y(), video->width(), 60);
}

void VideoSlot::loadExternalSubtitles(const QString &srtPath) {
    subtitlesEnabled = false;
    subtitleOverlay->hide();
    player->setActiveSubtitleTrack(-1);
    subtitles.clear();
    externalSubPath = srtPath;

    QFile f(srtPath);
    if (!f.open(QIODevice::ReadOnly)) return;

    QTextStream in(&f);
    while (!in.atEnd()) {
        in.readLine();
        QString timeLine = in.readLine();
        if (timeLine.isEmpty()) continue;

        auto toMs = [](const QString &t) -> qint64 {
            auto parts = t.split(QRegularExpression("[:,]"));
            if (parts.size() < 4) return 0;
            return parts[0].toLongLong() * 3600000
                   + parts[1].toLongLong() * 60000
                   + parts[2].toLongLong() * 1000
                   + parts[3].toLongLong();
        };

        auto times = timeLine.split(" --> ");
        if (times.size() < 2) continue;
        qint64 start = toMs(times[0].trimmed());
        qint64 end   = toMs(times[1].trimmed());

        QString text;
        QString line;
        while (!(line = in.readLine()).isEmpty())
            text += line + "\n";

        subtitles.append({start, end, text.trimmed()});
    }

    updateTracks();
    // auto-select the external entry(try consolidating the fn, this is beyond stupid and overengineered)
    subtitleTracks->setCurrentIndex(subtitleTracks->count() - 1);
}

void VideoSlot::updateExternalSubtitle(qint64 pos) {
    if (!subtitlesEnabled || subtitles.isEmpty()) {
        externalSubtitleLabel->hide();
        subtitleOverlay->hide();
        return;
    }
    for (auto &[start, end, text] : subtitles) {
        if (pos >= start && pos <= end) {
            repositionExternalSubtitleOverlay();
            externalSubtitleLabel->setText(text);
            externalSubtitleLabel->show();
            subtitleOverlay->show();
            return;
        }
    }
    externalSubtitleLabel->hide();
    subtitleOverlay->hide();
}

VideoSlot::~VideoSlot() {
    subtitleOverlay->deleteLater();
    player->setSource(QUrl());
    stop();
    player->setAudioOutput(nullptr);
    player->setVideoOutput(nullptr);
}

//\\VIDEOVIDEOVIDEO=======================================================================================================

//AUDIOAUDIOAUDIO====================================================================================================================

void AudioSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {
    initWidgets(parent);

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


    connectSlots(thisInstance);

    if (QString lyric = getLyrics(path); !lyric.isEmpty()) {
        lyrics->setText(lyric);
        lyrics->setVisible(false);
    }

    QUrl url = path.startsWith("http") ? QUrl(path) : QUrl::fromLocalFile(path);
    player->setSource(url);

    player->pause();
}

void AudioSlot::initWidgets(QWidget *parent) {
    wrapper      = new QWidget(parent);
    player       = new QMediaPlayer(wrapper);
    audio        = new QAudioOutput(wrapper);
    slider       = new QSlider(Qt::Horizontal, wrapper);
    cover        = new QLabel(wrapper);
    title        = new QLabel(wrapper);
    lyrics       = new QLabel(wrapper);
    artist       = new QLabel(wrapper);
    overlay      = new QWidget(wrapper);
    border       = new QWidget(wrapper);
}

QString AudioSlot::getLyrics(const QString &filePath) {
#ifndef Q_OS_ANDROID
    TagLib::MPEG::File file(filePath.toUtf8().constData());
    auto *tag = file.ID3v2Tag();
    if (!tag) return {};
    auto frames = tag->frameListMap()["USLT"];
    if (frames.isEmpty()) return {};
    auto *frame = dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame*>(frames.front());
    if (!frame) return {};
    return QString::fromStdWString(frame->text().toWString());
#else

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QByteArray header = f.read(10);
    if (header.size() < 10 || header.left(3) != "ID3") return {};

    quint32 tagSize = ((quint8)header[6] << 21)
                      | ((quint8)header[7] << 14)
                      | ((quint8)header[8] << 7)
                      | ((quint8)header[9]);

    QByteArray tagData = f.read(tagSize);
    int i = 0;

    while (i + 10 <= tagData.size()) {
        QString frameId = QString::fromLatin1(tagData.mid(i, 4));
        quint32 frameSize = ((quint8)tagData[i+4] << 24)
                            | ((quint8)tagData[i+5] << 16)
                            | ((quint8)tagData[i+6] << 8)
                            | ((quint8)tagData[i+7]);
        i += 10;

        if (frameId == "USLT" && frameSize > 4 && i + (int)frameSize <= tagData.size()) {
            QByteArray frameData = tagData.mid(i, frameSize);
            quint8 encoding = (quint8)frameData[0];

            int offset = 4;
            if (encoding == 0 || encoding == 3) {
                while (offset < frameData.size() && frameData[offset] != '\0') ++offset;
                ++offset;
                QByteArray lyrics = frameData.mid(offset);
                return encoding == 3
                           ? QString::fromUtf8(lyrics)
                           : QString::fromLatin1(lyrics);
            } else if (encoding == 1 || encoding == 2) {

                while (offset + 1 < frameData.size() &&
                       !(frameData[offset] == '\0' && frameData[offset+1] == '\0')) ++offset;
                offset += 2;
                QByteArray lyrics = frameData.mid(offset);
                return QString::fromUtf16(
                    reinterpret_cast<const char16_t*>(lyrics.constData()),
                    lyrics.size() / 2);
            }
        }
        i += frameSize;
    }
    return {};
#endif
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

void AudioSlot::showSettings(QWidget* settingsOverlay) {
    if (settingsOverlay->layout()) return;

    lyrics->setParent(nullptr);
    lyrics->setWordWrap(true);
    lyrics->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    auto *scrollArea = new QScrollArea(settingsOverlay);
    scrollArea->setWidget(lyrics);
    scrollArea->setWidgetResizable(true);

    auto *layout = new QVBoxLayout(settingsOverlay);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scrollArea);
}

void AudioSlot::connectSlots(QObject* thisInstance) {
    QObject::connect(player, &QMediaPlayer::durationChanged, slider, &QSlider::setMaximum);
    QObject::connect(player, &QMediaPlayer::positionChanged, slider, &QSlider::setValue);
    QObject::connect(player, &QMediaPlayer::seekableChanged, slider, &QSlider::setEnabled);
    QObject::connect(slider, &QSlider::sliderMoved, player, &QMediaPlayer::setPosition);

    QObject::connect(slider, &QSlider::sliderPressed, thisInstance, [this] {
        player->setPosition(slider->sliderPosition());
    });

    QObject::connect(player, &QMediaPlayer::metaDataChanged, thisInstance, [this] {
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
}


AudioSlot::~AudioSlot() {
    cover->deleteLater();
    title->deleteLater();
    artist->deleteLater();
    lyrics->deleteLater();
    player->setSource(QUrl());
    stop();
    player->setAudioOutput(nullptr);
}

//\\AUDIOAUDIOAUDIO=======================================================================================================

//IMAGEIMAGEIMAGE=================================================================================================================

void ImageSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {
    filePath = path;
    initWidgets(parent);

    auto *layout = new QVBoxLayout(wrapper);

    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(viewer);

    item->setPos(0, 0);
    scene->addItem(item);
    item->setTransformationMode(Qt::SmoothTransformation);

    viewer->setDragMode(QGraphicsView::ScrollHandDrag);

    viewer->setAcceptDrops(true);
    viewer->installEventFilter(thisInstance);
    viewer->viewport()->installEventFilter(thisInstance);
    viewer->viewport()->setAcceptDrops(true);

    viewer->setScene(scene);
#ifdef Q_OS_ANDROID
    viewer->setViewport(new QWidget());
    viewer->setRenderHint(QPainter::SmoothPixmapTransform);
#endif
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

void ImageSlot::initWidgets(QWidget *parent) {
    wrapper      = new QWidget(parent);
    pixmap       = QPixmap(filePath);
    border       = new QWidget(wrapper);
    viewer       = new QGraphicsView(wrapper);
    scene        = new QGraphicsScene(viewer);
    item         = new QGraphicsPixmapItem(pixmap);
}

void ImageSlot::zoom(qreal x)  {
    qreal newZoom = zoomFactor * x;
    if (newZoom < 0.5 || newZoom > 10.0) return;
    zoomFactor = newZoom;
    viewer->scale(x, x);
}

//\\IMAGEIMAGEIMAGE==================================================================================================================

//PDFPDFPDPFPDFPDF===NORMAL==============================================================================================================

void PdfSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {
    initWidgets(parent);


    initLayoutSetInstall(thisInstance);
    initComboBox();

    doc->load(path);
    if (doc->status() != QPdfDocument::Status::Ready) return;

    filePath = path;

    pageCount->setText("of " + QString::number(doc->pageCount()));
    bookmarkModel->setDocument(doc);
    pageSelector->setDocument(doc);
    searchModel->setDocument(doc);
    linkModel->setDocument(doc);

    connectSlots(thisInstance);
}

void PdfSlot::initLayoutSetInstall(QObject *thisInstance) {
    if (auto *spin = pageSelector->findChild<QSpinBox*>())
        spin->setButtonSymbols(QAbstractSpinBox::NoButtons);

    cacheFlushTimer->setSingleShot(true);
    cacheFlushTimer->setInterval(3000);

    wrapper->setAcceptDrops(true);
    wrapper->installEventFilter(thisInstance);
    wrapper->setAttribute(Qt::WA_Hover);

    border->setAttribute(Qt::WA_TransparentForMouseEvents);
    border->setGeometry(wrapper->rect());
    border->raise();

    viewer->setDocument(doc);
    viewer->viewport()->installEventFilter(thisInstance);
    viewer->viewport()->setAcceptDrops(true);
    viewer->setPageMode(QPdfView::PageMode::MultiPage);
    viewer->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    viewer->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    viewer->setSearchModel(searchModel);

    navBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    navBar->setFixedHeight(36);
    navBar->hide();

    findBar->setMaximumWidth(420);
    findBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    findBar->hide();

    sidePanel->setStyleSheet("background-color: #111111;");
    sidePanel->setWindowOpacity(0.55);
    sidePanel->hide();
    bookmarkTree->hide();
    thumbnailView->hide();
    thumbnailView->viewport()->installEventFilter(thisInstance);

    linkHighlight->setAttribute(Qt::WA_TransparentForMouseEvents);
    linkHighlight->setStyleSheet("background: rgba(255, 220, 0, 60); border: 1px solid rgba(255, 180, 0, 180);");
    linkHighlight->hide();

    searchField->setPlaceholderText(QString("Find in document"));
    searchField->setMaximumWidth(400);

    pageSelector->setFixedHeight(32);
    pageSelector->setMaximumWidth(90);
    zoomSelector->setFixedHeight(32);
    zoomSelector->setMinimumWidth(120);
    sidePanelButton->setFixedSize(40, 40);
    prevPage->setFixedSize(30, 30);
    nextPage->setFixedSize(30, 30);
    indexTabButton->setFixedSize(40, 28);
    thumbnailTabButton->setFixedSize(40, 28);

    auto *findLayout = new QHBoxLayout(findBar);
    findLayout->setContentsMargins(4, 2, 4, 2);
    findLayout->setSpacing(4);
    findLayout->addWidget(searchField);
    findLayout->addWidget(findPrev);
    findLayout->addWidget(findNext);
    findLayout->addWidget(findClose);


    auto *navLayout = new QHBoxLayout(navBar);
    navLayout->setContentsMargins(8, 2, 8, 2);
    navLayout->setSpacing(4);
    navLayout->addWidget(sidePanelButton);
    navLayout->addWidget(zoomSelector);
    navLayout->addStretch();
    navLayout->addWidget(prevPage);
    navLayout->addWidget(pageSelector);
    navLayout->addWidget(pageCount);
    navLayout->addWidget(nextPage);


    auto *tabBar = new QWidget(sidePanel);
    tabBar->setFixedHeight(32);

    auto *tabLayout = new QHBoxLayout(tabBar);
    tabLayout->setContentsMargins(4, 2, 4, 2);
    tabLayout->setSpacing(0);
    // tabLayout->addStretch();
    tabLayout->addWidget(indexTabButton);
    tabLayout->addWidget(thumbnailTabButton);
    tabLayout->addStretch();


    auto *sidePanelLayout = new QVBoxLayout(sidePanel);
    sidePanelLayout->setContentsMargins(0, 0, 0, 0);
    sidePanelLayout->setSpacing(0);
    sidePanelLayout->setAlignment(Qt::AlignTop);
    sidePanelLayout->addWidget(tabBar);
    sidePanelLayout->addWidget(bookmarkTree, 1);
    sidePanelLayout->addWidget(thumbnailView, 1);

    auto *layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    layout->addWidget(navBar);
    layout->addWidget(viewer, 1);
}

void PdfSlot::initWidgets(QWidget *parent) {
    wrapper           = new QWidget(parent);
    viewer            = new QPdfView(wrapper);
    doc               = new QPdfDocument(wrapper);
    border            = new QWidget(wrapper);
    pageSelector      = new QPdfPageSelector(wrapper);
    searchModel       = new QPdfSearchModel(wrapper);
    searchField       = new QLineEdit(wrapper);
    zoomSelector      = new QComboBox(wrapper);
    linkModel         = new QPdfLinkModel(wrapper);
    bookmarkModel     = new QPdfBookmarkModel(wrapper);
    sidePanel         = new QWidget(wrapper);
    cacheFlushTimer   = new QTimer(wrapper);
    indexTabButton    = new QPushButton("idx", sidePanel);
    thumbnailTabButton = new QPushButton("pg", sidePanel);

    bookmarkTree      = new QTreeView(sidePanel);

    findBar           = new QWidget(wrapper);
    findPrev          = new QPushButton("<-", findBar);
    findNext          = new QPushButton("->", findBar);
    findClose         = new QPushButton("x", findBar);

    navBar            = new QWidget(wrapper);
    pageCount         = new QLabel(navBar);
    prevPage          = new QPushButton("↑", navBar);
    nextPage          = new QPushButton("↓", navBar);
    sidePanelButton   = new QPushButton("=", navBar);

    thumbnailView       = new QGraphicsView(sidePanel);
    thumbnailScene      = new QGraphicsScene(thumbnailView);

    linkHighlight = new QWidget(viewer->viewport());
    nav = viewer->pageNavigator();


}

QPointF PdfSlot::toPdfPoint(QPoint viewportPos, int page) {
    qreal dpi   = viewer->screen()->logicalDotsPerInch();
    qreal scale = nav->currentZoom() * dpi / 72.0;

    qreal pageWidthPx  = doc->pagePointSize(page).width() * scale;
    qreal viewportWidth = viewer->viewport()->width();
    qreal xOffset = std::max((qreal)viewer->documentMargins().left(),
                             (viewportWidth - pageWidthPx) / 2.0);

    qreal yOffsetPx = viewer->documentMargins().top();
    for (int p = 0; p < page; ++p)
        yOffsetPx += doc->pagePointSize(p).height() * scale + viewer->pageSpacing();

    QScrollBar *hb = viewer->horizontalScrollBar();
    QScrollBar *vb = viewer->verticalScrollBar();

    qreal pdfX = (viewportPos.x() + hb->value() - xOffset) / scale;
    qreal pdfY = (viewportPos.y() + vb->value() - yOffsetPx) / scale;

    return QPointF(pdfX, pdfY);
}

QString PdfSlot::getSelectedText() {
    int page = nav->currentPage();
    QPointF start = toPdfPoint(dragStart, page);
    QPointF end   = toPdfPoint(dragEnd,   page);
    qDebug() << start;
    qDebug() << end;
    QPdfSelection sel = doc->getSelection(page, start, end);
    if(sel.isValid())
        return sel.text();
    else {
        start += QPointF(100, 0);
        sel = doc->getSelection(page, start, end);
        return sel.text();
    }
}

bool PdfSlot::hasTextAt(QPointF pt, int page) {
    QPdfSelection sel = doc->getSelection(page, pt, pt + QPointF(1.0, 0));
    return sel.isValid();
}


void PdfSlot::searchResultsChanged(const QModelIndex &current, const QModelIndex &previous) {
    Q_UNUSED(previous);
    if (!current.isValid())
        return;
    const int page = current.data(int(QPdfSearchModel::Role::Page)).toInt();
    const QPointF location = current.data(int(QPdfSearchModel::Role::Location)).toPointF();
    viewer->pageNavigator()->jump(page, location);
    viewer->setCurrentSearchResultIndex(current.row());
}

void PdfSlot::nextResult() {
    int count = searchModel->rowCount(QModelIndex());
    if (count == 0) return;
    currentResultIndex = (currentResultIndex + 1) % count;
    jumpToResult(currentResultIndex);
}

void PdfSlot::prevResult() {
    int count = searchModel->rowCount(QModelIndex());
    if (count == 0) return;
    currentResultIndex = (currentResultIndex - 1 + count) % count;
    jumpToResult(currentResultIndex);
}

void PdfSlot::jumpToResult(int i) {
    QModelIndex idx = searchModel->index(i, 0);
    if (!idx.isValid()) return;
    const int page = idx.data(int(QPdfSearchModel::Role::Page)).toInt();
    const QPointF loc = idx.data(int(QPdfSearchModel::Role::Location)).toPointF();
    nav->jump(page, loc);
    viewer->setCurrentSearchResultIndex(i);
}

void PdfSlot::menuZoom(const QString &text) {
    if (text == QLatin1String("Fit Width")) {
        viewer->setZoomMode(QPdfView::ZoomMode::FitToWidth);
    }

    else if (text == QLatin1String("Fit Page")) {
        viewer->setZoomMode(QPdfView::ZoomMode::FitInView);
    }

    else {
        factor = 1.0;

        QString withoutPercent(text);
        withoutPercent.remove(QLatin1Char('%'));

        bool ok = false;
        const int zoomLevel = withoutPercent.toInt(&ok);
        if (ok)
            factor = zoomLevel / 100.0;

        viewer->setZoomFactor(factor);
    }
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

void PdfSlot::reset() {
    zoomSelector->setCurrentIndex(8);
}

void PdfSlot::enableSearch(bool x) {
    findBar->setVisible(x);
    if (x) {
        int barH = findBar->sizeHint().height();
        findBar->setGeometry(0, wrapper->height() - barH,
                             findBar->maximumWidth(), barH);
        if (sidePanel->isVisible()) {
            QRect r = wrapper->rect();
            int navH = navBar->sizeHint().height();
            sidePanel->setGeometry(3, navH + 2, r.width() / 4,
                                   r.height() - navH - 4 - barH);
        }
        searchField->setFocus();
        searchField->selectAll();
    } else {
        if (sidePanel->isVisible()) {
            QRect r = wrapper->rect();
            int navH = navBar->sizeHint().height();
            sidePanel->setGeometry(3, navH + 2, r.width() / 4,
                                   r.height() - navH - 4);
        }
        searchField->clear();
        searchModel->setSearchString("");
        currentResultIndex = -1;
    }
}


void PdfSlot::enableSidePanel() {
    if (!sidePanel->isVisible()) {
        QRect r = wrapper->rect();
        int previewWidth = r.width() / 5;
        int navH = navBar->sizeHint().height();
        int barH = findBar->isVisible() ? findBar->sizeHint().height() : 0;
        sidePanel->setGeometry(3, navH + 2, previewWidth,
                               r.height() - navH - 4 - barH);
        bookmarkTree->setModel(bookmarkModel);
        sidePanel->setVisible(true);
    }
    else sidePanel->setVisible(false);
}


void PdfSlot::initComboBox() {
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
}


void PdfSlot::showIndexTab() {
    if(!bookmarkTree->isVisible()) {
        if(thumbnailView->isVisible()) thumbnailView->setVisible(false);
        bookmarkTree->setVisible(true);
    }
    else  bookmarkTree->hide();
}


void PdfSlot::showThumbnailTab() {
    if (!thumbnailView->isVisible()) {
        if (bookmarkTree->isVisible()) bookmarkTree->setVisible(false);
        if (!thumbnailsLoaded) {
            populateThumbnailTab();
            thumbnailsLoaded = true;
        }
        thumbnailView->setVisible(true);
    }
    else thumbnailView->hide();
}


void PdfSlot::populateThumbnailTab() {
    int pageCt = doc->pageCount();
    auto *watcher = new QFutureWatcher<QList<QPixmap>>();
    QObject::connect(watcher, &QFutureWatcher<QList<QPixmap>>::finished, watcher, [&, watcher]() {
        auto pixmaps = watcher->result();
        qreal yOffset = 0;
        for (auto &pix : pixmaps) {
            auto *item = new QGraphicsPixmapItem(pix);
            item->setPos(0, yOffset);
            item->setTransformationMode(Qt::SmoothTransformation);
            thumbnailScene->addItem(item);
            thumbnailIndices.emplaceBack(item);
            yOffset += pix.height() + 10;
        }
        thumbnailView->setScene(thumbnailScene);
        thumbnailView->setAlignment(Qt::AlignTop);
        QRectF contentRect = thumbnailScene->itemsBoundingRect();
        thumbnailView->setSceneRect(contentRect);
        watcher->deleteLater();
    });

    auto future = QtConcurrent::run([this, pageCt]() {
        QPdfDocument thumbDoc;
        thumbDoc.load(filePath);
        QList<QPixmap> pixmaps;
        for (int i = 0; i < pageCt; ++i) {
            QSize renderSize = QSize(120, 200);
            auto img = thumbDoc.render(i, renderSize);
            QImage filledImg(renderSize, QImage::Format_RGB32);
            filledImg.fill(Qt::white);
            QPainter p(&filledImg);
            p.drawImage(0, 0, img);
            p.end();
            pixmaps.append(QPixmap::fromImage(filledImg));
        }
        return pixmaps;
    });

    watcher->setFuture(future);
}

void PdfSlot::syncThumbnailToPage() {
    if (thumbnailIndices.isEmpty()) return;
    int page = nav->currentPage();
    if (page < 0 || page >= thumbnailIndices.size()) return;
    auto destination = thumbnailIndices[page];
    thumbnailView->centerOn(destination);
}

void PdfSlot::syncPageToThumbnail(QPointF pagePos) {
    for (int i = 0; i < thumbnailIndices.size(); ++i) {
        if (thumbnailIndices[i]->boundingRect()
                .translated(thumbnailIndices[i]->pos())
                .contains(pagePos)) {
            nav->jump(i, {}, nav->currentZoom());
            break;
        }
    }
}

void PdfSlot::connectSlots(QObject* thisInstance) {
    QObject::connect(findNext, &QPushButton::clicked, thisInstance, [this] {
        nextResult();
    });
    QObject::connect(findPrev, &QPushButton::clicked, thisInstance, [this] {
        prevResult();
    });
    QObject::connect(findClose, &QPushButton::clicked, thisInstance, [this] {
        enableSearch(false);
    });

    QObject::connect(pageSelector, &QPdfPageSelector::currentPageChanged, thisInstance,
                     [this](int page) {
                         nav->jump(page, {}, nav->currentZoom());
    });


    QObject::connect(zoomSelector, &QComboBox::currentTextChanged, thisInstance,
                     [this](const QString &text) {
                         menuZoom(text);
    });


    QObject::connect(searchField, &QLineEdit::textEdited, thisInstance, [this](const QString &text) {
        searchModel->setSearchString(text);
        currentResultIndex = -1;
    });

    QObject::connect(nav, &QPdfPageNavigator::currentPageChanged, thisInstance, [this] {
        syncThumbnailToPage();
    });

    QObject::connect(bookmarkTree, &QTreeView::clicked, thisInstance, [this](const QModelIndex &index) {
        int page = index.data(int(QPdfBookmarkModel::Role::Page)).toInt();
        nav->jump(page, {}, nav->currentZoom());
    });

    QObject::connect(prevPage, &QPushButton::clicked, thisInstance, [this] {
        nav->jump(nav->currentPage() - 1, {}, nav->currentZoom());
    });
    QObject::connect(nextPage, &QPushButton::clicked, thisInstance, [this] {
        nav->jump(nav->currentPage() + 1, {}, nav->currentZoom());
    });
    QObject::connect(sidePanelButton, &QPushButton::clicked, thisInstance, [this] {
        enableSidePanel();
    });
    QObject::connect(indexTabButton, &QPushButton::clicked, thisInstance, [this] {
        showIndexTab();
    });
    QObject::connect(thumbnailTabButton, &QPushButton::clicked, thisInstance, [this] {
        showThumbnailTab();
    });

    QObject::connect(viewer->verticalScrollBar(), &QScrollBar::valueChanged,
                      thisInstance, [this]() {
                          pageSelector->setCurrentPage(nav->currentPage());
                          cacheFlushTimer->start();
                          evictValue += 7;
                      });

    QObject::connect(cacheFlushTimer, &QTimer::timeout,
                     thisInstance, [this]() {
                         if (evictValue > evictThreshold) {
                             int scrollPos = viewer->verticalScrollBar()->value();
                             viewer->setDocument(nullptr);
                             viewer->setDocument(doc);
                             linkModel->setDocument(doc);
                             viewer->verticalScrollBar()->setValue(scrollPos);
                         }
                         evictValue = 0;
                     });
}


bool PdfSlot::processLinks(QPoint clickPos, bool shouldExecute) {
    int page = nav->currentPage();
    linkModel->setPage(page);

    qreal dpi   = viewer->screen()->logicalDotsPerInch();
    qreal scale = nav->currentZoom() * dpi / 72.0;

    qreal yOffsetPx = viewer->documentMargins().top();
    for (int p = 0; p < page; ++p)
        yOffsetPx += doc->pagePointSize(p).height() * scale + viewer->pageSpacing();


    QScrollBar *hb = viewer->horizontalScrollBar();
    QScrollBar *vb = viewer->verticalScrollBar();

    qreal pageWidthPx = doc->pagePointSize(page).width() * scale;
    qreal viewportWidth = viewer->viewport()->width();
    qreal xOffset = std::max((qreal)viewer->documentMargins().left(), (viewportWidth - pageWidthPx) / 2.0);

    for (int i = 0; i < linkModel->rowCount(QModelIndex()); ++i) {
        QModelIndex idx    = linkModel->index(i, 0);
        QRectF linkRect    = idx.data(int(QPdfLinkModel::Role::Rectangle)).toRectF();

        QRectF pixelRect(
            linkRect.x() * scale + xOffset - hb->value(),
            linkRect.y() * scale + yOffsetPx - vb->value(),
            linkRect.width() * scale,
            linkRect.height() * scale
            );

        if (pixelRect.contains(clickPos)) {
            linkHighlight->setGeometry(pixelRect.toRect());
            linkHighlight->show();
            linkHighlight->raise();
            if (shouldExecute) {
                QUrl url = idx.data(int(QPdfLinkModel::Role::Url)).toUrl();
                if (url.isValid()) {
                    QDesktopServices::openUrl(url);
                } else {
                    int targetPage = idx.data(int(QPdfLinkModel::Role::Page)).toInt();
                    QPointF loc    = idx.data(int(QPdfLinkModel::Role::Location)).toPointF();
                    qreal zoom     = idx.data(int(QPdfLinkModel::Role::Zoom)).toReal();
                    nav->jump(targetPage, loc, zoom != 0 ? zoom : nav->currentZoom());
                }
            }
            return true;
        }
    }
    linkHighlight->hide();
    return false;
}

PdfSlot::~PdfSlot() {
    doc->close();
}

//\\PDFPDFPDPFPDFPDF===NORMAL==============================================================================================================


//COMICCOMICCOMIC==========================================================================================================================


void ComicSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {
    initWidgets(parent);
    auto *layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(viewer);

    viewer->setScene(scene);
    viewer->setDragMode(QGraphicsView::ScrollHandDrag);
    viewer->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    viewer->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    viewer->setBackgroundBrush(Qt::black);

    wrapper->installEventFilter(thisInstance);
    wrapper->setAcceptDrops(true);
    wrapper->setAttribute(Qt::WA_Hover);
    viewer->installEventFilter(thisInstance);
    viewer->viewport()->installEventFilter(thisInstance);

    border->setAttribute(Qt::WA_TransparentForMouseEvents);
    border->setGeometry(wrapper->rect());
    border->raise();

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, path.toUtf8().constData(), 0))
        return;

    int fileCount = (int)mz_zip_reader_get_num_files(&zip);
    QList<QPair<QString, QByteArray>> files;

    for (int i = 0; i < fileCount; ++i) {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&zip, i, &stat)) continue;

        QString name = QString::fromUtf8(stat.m_filename);
        QString ext  = QFileInfo(name).suffix().toLower();
        if (!QStringList{"jpg","jpeg","png","webp"}.contains(ext)) continue;

        size_t size;
        void *data = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
        if (!data) continue;

        QByteArray ba((const char*)data, (int)size);
        mz_free(data);
        files.append({name, ba});
    }
    mz_zip_reader_end(&zip);

    std::sort(files.begin(), files.end(), [](auto &a, auto &b) {
        return a.first < b.first;
    });

    for (auto &[name, data] : files)
        pageData.append(data);

    totalPages = pageData.size();
    if (totalPages == 0) return;

    showPage(0);
}

void ComicSlot::initWidgets(QWidget *parent) {
    wrapper = new QWidget(parent);
    viewer  = new QGraphicsView(wrapper);
    scene   = new QGraphicsScene(viewer);
    border  = new QWidget(wrapper);
}

void ComicSlot::showPage(int index) {
    if (index < 0 || index >= totalPages) return;
    currentPage = index;
    scene->clear();

    QImage img;
    img.loadFromData(pageData[currentPage]);
    auto *item = new QGraphicsPixmapItem(QPixmap::fromImage(img));
    item->setTransformationMode(Qt::SmoothTransformation);
    scene->addItem(item);
    scene->setSceneRect(scene->itemsBoundingRect());

    QTimer::singleShot(0, viewer, [this, item]() {
        viewer->fitInView(item, Qt::KeepAspectRatio);
    });
}

void ComicSlot::forward()  { showPage(currentPage + 1); }

void ComicSlot::backward() { showPage(currentPage - 1); }

void ComicSlot::scroll(int x) {
    viewer->verticalScrollBar()->setValue(viewer->verticalScrollBar()->value() + x);
}

void ComicSlot::zoom(qreal x) {
    qreal n = zoomFactor * x;
    if (n < 0.5 || n > 10.0) return;
    zoomFactor = n;
    viewer->scale(x, x);
}

//\\COMICCOMICCOMIC==========================================================================================================================


//PLAINTEXTPLAINTEXTPLAINTEXT=================================================================================================================

void PlaintextSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {

}

//\\PLAINTEXTPLAINTEXTPLAINTEXT=================================================================================================================

