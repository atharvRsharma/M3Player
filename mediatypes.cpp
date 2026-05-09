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


#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/unsynchronizedlyricsframe.h>
#include <taglib/mpegfile.h>


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
    subtitleTracks = new QComboBox(wrapper);
    audioTracks = new QComboBox(wrapper);
    videoTracks = new QComboBox(wrapper);



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



    // QObject::connect(player, &QMediaPlayer::seekableChanged, thisInstance, [this] (int sec) {
    //     seek(sec);
    // });
    connectSlots(thisInstance);

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

void VideoSlot::showSettings(QWidget* settingsOverlay) {
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
    lyrics       = new QLabel(wrapper);
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


    connectSlots(thisInstance);

    if (QString lyric = getLyrics(path); !lyric.isEmpty()) {
        lyrics->setText(lyric);
        lyrics->setVisible(false);
    }

    player->setSource(QUrl::fromLocalFile(path));

    player->pause();
}

QString AudioSlot::getLyrics(const QString &filePath) {
    TagLib::MPEG::File file(filePath.toUtf8().constData());
    auto *tag = file.ID3v2Tag();
    if (!tag) return {};


    auto frames = tag->frameListMap()["USLT"];
    if (frames.isEmpty()) return {};

    auto *frame = dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame*>(frames.front());
    if (!frame) return {};

    return QString::fromStdWString(frame->text().toWString());
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
    item->setTransformationMode(Qt::SmoothTransformation);

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
    if (newZoom < 0.5 || newZoom > 10.0) return;
    zoomFactor = newZoom;
    viewer->scale(x, x);
}

//\\IMAGEIMAGEIMAGE==================================================================================================================

//PDFPDFPDPFPDFPDF===NORMAL==============================================================================================================

void PdfSlot::load(const QString &path, QWidget *parent, QObject *thisInstance) {
    wrapper           = new QWidget(parent);
    viewer            = new QPdfView(wrapper);
    doc               = new QPdfDocument(wrapper);
    border            = new QWidget(wrapper);
    pageSelector      = new QPdfPageSelector(wrapper);
    searchModel       = new QPdfSearchModel(wrapper);
    searchField       = new QLineEdit(wrapper);
    zoomSelector      = new QComboBox(wrapper);
    bookmarkModel     = new QPdfBookmarkModel(wrapper);
    sidePanel         = new QWidget(wrapper);
    indexTabButton    = new QPushButton("idx", sidePanel);

    bookmarkTree      = new QTreeView(sidePanel);

    findBar           = new QWidget(wrapper);
    findPrev          = new QPushButton("<-", findBar);
    findNext          = new QPushButton("->", findBar);
    findClose         = new QPushButton("x", findBar);

    navBar            = new QWidget(wrapper);
    prevPage          = new QPushButton("↑", navBar);
    nextPage          = new QPushButton("↓", navBar);
    sidePanelButton   = new QPushButton("=", navBar);






    if (auto *spin = pageSelector->findChild<QSpinBox*>())
        spin->setButtonSymbols(QAbstractSpinBox::NoButtons);


    findBar->setMaximumWidth(420);
    findBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);


    sidePanel->hide();
    bookmarkTree->hide();
    sidePanelButton->setMaximumWidth(40);

    navBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    navBar->setFixedHeight(36);
    navBar->raise();
    navBar->hide();


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





    pageSelector->setFixedHeight(32);
    pageSelector->setMaximumWidth(90);
    zoomSelector->setFixedHeight(32);
    zoomSelector->setMinimumWidth(120);


    sidePanelButton->setMinimumWidth(40);


    navLayout->addStretch();
    navLayout->addWidget(prevPage);
    navLayout->addWidget(pageSelector);
    navLayout->addWidget(nextPage);


    prevPage->setMaximumWidth(30);
    nextPage->setMaximumWidth(30);


    auto *sidePanelLayout = new QVBoxLayout(sidePanel);
    sidePanelLayout->setContentsMargins(0, 0, 0, 0);
    sidePanelLayout->setSpacing(0);
    sidePanelLayout->setAlignment(Qt::AlignTop);
    sidePanel->setWindowOpacity(0.85);
    sidePanel->setStyleSheet("background-color: #111111;");

    auto *tabBar = new QWidget(sidePanel);
    tabBar->setFixedHeight(32);

    auto *tabLayout = new QHBoxLayout(tabBar);
    tabLayout->setContentsMargins(4, 2, 4, 2);
    tabLayout->setSpacing(0);
    tabLayout->addStretch();
    tabLayout->addWidget(indexTabButton);
    tabLayout->addStretch();
    indexTabButton->setFixedSize(40, 28);

    sidePanelLayout->addWidget(tabBar);
    sidePanelLayout->addWidget(bookmarkTree, 1);

    auto *layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    layout->addWidget(navBar);
    layout->addWidget(viewer, 1);

    wrapper->setAcceptDrops(true);
    wrapper->installEventFilter(thisInstance);
    wrapper->setAttribute(Qt::WA_Hover);

    initComboBox();

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
    viewer->setSearchModel(searchModel);

    searchField->setPlaceholderText(QString("Find in document"));
    searchField->setMaximumWidth(400);
    findBar->raise();
    findBar->hide();
    connectSlots(thisInstance);
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

        zoom(factor);
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
        int previewWidth = r.width() / 4;
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
        bookmarkTree->setVisible(true);
        bookmarkTree->setGeometry(sidePanel->rect());
    }
    else  bookmarkTree->hide();
}

void PdfSlot::connectSlots(QObject* thisInstance) {
    QObject::connect(findNext, &QPushButton::clicked, thisInstance, [this]() {
        nextResult();
    });
    QObject::connect(findPrev, &QPushButton::clicked, thisInstance, [this]() {
        prevResult();
    });
    QObject::connect(findClose, &QPushButton::clicked, thisInstance, [this]() {
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

    QObject::connect(bookmarkTree, &QTreeView::clicked, thisInstance, [this](const QModelIndex &index) {
        int page = index.data(int(QPdfBookmarkModel::Role::Page)).toInt();
        nav->jump(page, {}, nav->currentZoom());
    });



    QObject::connect(prevPage, &QPushButton::clicked, thisInstance, [this]() {
        nav->jump(nav->currentPage() - 1, {}, nav->currentZoom());
    });
    QObject::connect(nextPage, &QPushButton::clicked, thisInstance, [this]() {
        nav->jump(nav->currentPage() + 1, {}, nav->currentZoom());
    });
    QObject::connect(sidePanelButton, &QPushButton::clicked, thisInstance, [this] {
        enableSidePanel();
    });
    QObject::connect(indexTabButton, &QPushButton::clicked, thisInstance, [this] {
        showIndexTab();
    });
}

PdfSlot::~PdfSlot() {
    doc->close();
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

