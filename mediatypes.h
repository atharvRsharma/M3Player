#ifndef MEDIATYPES_H
#define MEDIATYPES_H


#include <memory>

#include <QString>
#include <QSize>
#include <QSet>
#include <QImage>
#include <QPixmap>
#include <QHash>
#include <QMediaPlayer>


QT_BEGIN_NAMESPACE
class QWidget;
class QPdfView;
class QPdfSearchModel;
class QPdfPageSelector;
class QLineEdit;
class QPdfDocument;
class QMediaPlayer;
class QAudioOutput;
class QVideoWidget;
class QLabel;
class QGraphicsView;
class QGraphicsPixmapItem;
class QGraphicsScene;
class QSlider;
class QObject;
class QPdfBookmarkModel;
class QPdfPageNavigator;
class QComboBox;
class QStyleOptionViewItem;
class QToolBar;
class QListView;
class QStyledItemDelegate;
class QTimer;
class QPushButton;
class QTreeView;
class QPdfPageRenderer;
class QPdfLinkModel;
QT_END_NAMESPACE


struct MediaSlot  {
    virtual ~MediaSlot() = default;
    virtual void load(const QString &path, QWidget *parent, QObject *thisInstance) = 0;
    virtual void play() {}
    virtual void pause() {}
    virtual void stop() {}
    virtual void toggleMute() {}
    virtual void replay() {}
    virtual void toggleMediaControls(bool) {}
    virtual void zoom(qreal) {}
    virtual void setVolume(float) {}
    virtual void forward() {}
    virtual void backward() {}
    virtual float getVolume() const { return 0.0f; }
    virtual void adjustVolume(float) {};
    virtual void scroll(int) {};
    virtual void seek(int) {}
    virtual void showSettings(QWidget*) {};
    virtual void connectSlots(QObject*) {}
    virtual void initWidgets(QWidget*) = 0;
    virtual QMediaPlayer::PlaybackState getPlayerState() const { return QMediaPlayer::StoppedState; }
    virtual QString type() const = 0;

    QWidget *wrapper = nullptr;
    QWidget *border  = nullptr;
    QWidget *overlay = nullptr;
};

struct VideoSlot : MediaSlot {
    QMediaPlayer *player;
    QAudioOutput *audio;
    QVideoWidget *video;
    QSlider      *slider;
    QComboBox    *audioTracks;
    QComboBox    *videoTracks;
    QComboBox    *subtitleTracks;
    QLabel       *externalSubtitleLabel;
    QTimer       *subTimer;
    QWidget      *subtitleOverlay;


    QList<std::tuple<qint64, qint64, QString>> subtitles;
    QString externalSubPath;

    const qint64 seekStep   = 5000;
    float currentVolume     = 1.0f;
    bool subtitlesEnabled   = true;

    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
    void initWidgets(QWidget *parent) override;
    void play() override;
    void pause() override;
    void stop() override;
    void toggleMute() override;
    void replay() override;
    void toggleMediaControls(bool x) override;
    float getVolume() const override;
    void setVolume(float x) override;
    void forward() override;
    void backward() override;
    void adjustVolume(float delta) override;
    void seek(int sec) override;
    void showSettings(QWidget* settingsOverlay) override;
    void connectSlots(QObject* thisInstance) override;

    // specific fns
    void loadExternalSubtitles(const QString &srtPath);
    void updateExternalSubtitle(qint64 pos);
    void repositionExternalSubtitleOverlay();

    QMediaPlayer::PlaybackState getPlayerState() const override {
        return player->playbackState();
    }

    QString type() const override { return "video"; }
    ~VideoSlot();

private:
    // pvt local fns
    void selectSubtitleStream(int stream);
    void selectVideoStream(int stream);
    void selectAudioStream(int stream);
    QString trackName(const QMediaMetaData &metaData, int index);
    void updateTracks();
};

struct AudioSlot : MediaSlot {
    QMediaPlayer *player;
    QAudioOutput *audio;
    QSlider      *slider;

    QLabel       *cover,
                 *title,
                 *artist,
                 *lyrics;

    QSize        lastSize;
    QImage       coverImage;

    const qint64 seekStep = 5000;
    float currentVolume = 1.0f;


    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
    void initWidgets(QWidget *parent) override;
    void play() override;
    void pause() override;
    void stop() override;
    void toggleMute() override;

    void replay() override;
    void toggleMediaControls(bool x) override;
    float getVolume() const override;
    void setVolume(float x) override;
    void forward() override;
    void backward() override;
    void adjustVolume(float delta) override;
    void seek(int sec) override;
    void showSettings(QWidget* settingsOverlay) override;
    void connectSlots(QObject* thisInstance) override;

    QMediaPlayer::PlaybackState getPlayerState() const override {
        return player->playbackState();
    }


    QString type() const override { return "audio"; }
    ~AudioSlot();

private:
    // pvt local fns
    QString getLyrics(const QString &filePath);

};

struct ImageSlot : MediaSlot {
    QGraphicsPixmapItem     *item;
    QGraphicsView           *viewer;
    QGraphicsScene          *scene;
    QSize                   lastSize;
    QPixmap                 pixmap;
    QString                 filePath;

    qreal zoomFactor = 1.0f;

    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
    void initWidgets(QWidget *parent) override;
    void zoom(qreal x) override;

    QString type() const override { return "image"; }
};

struct PdfSlot : MediaSlot {
    QPdfDocument        *doc;
    QPdfView            *viewer;
    QPdfPageSelector    *pageSelector;
    QPdfSearchModel     *searchModel;
    QLineEdit           *searchField;
    QPdfBookmarkModel   *bookmarkModel;
    QPdfPageNavigator   *nav;
    QComboBox           *zoomSelector;
    QWidget             *sidePanel;
    QTreeView           *bookmarkTree;
    QPdfLinkModel       *linkModel;
    QLabel              *pageCount;
    QTimer              *cacheFlushTimer;
    QWidget             *linkHighlight;

    QWidget             *findBar;
    QPushButton         *findNext;
    QPushButton         *findPrev;
    QPushButton         *findClose;

    QWidget             *navBar;
    QPushButton         *prevPage;
    QPushButton         *nextPage;
    QPushButton         *sidePanelButton;

    QPushButton         *thumbnailTabButton;
    QGraphicsView       *thumbnailView;
    QGraphicsScene      *thumbnailScene;
    QPixmap             pixmap;

    QPushButton         *indexTabButton;

    QList<QGraphicsPixmapItem*> thumbnailIndices;
    QString             pendingSearch;
    QString             filePath;
    qreal               factor;
    QPoint              dragStart;
    QPoint              dragEnd;
    int                 evictValue              = 0;
    int                 evictThreshold          = 120;
    int                 currentResultIndex      = -1;
    bool                sidePanelOpen           = false;
    bool                thumbnailsLoaded        = false;
    bool                isDragging              = false;


    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
    void initWidgets(QWidget *parent) override;
    void forward() override;
    void backward() override;
    void zoom(qreal x) override;
    void scroll(int x) override;
    void connectSlots(QObject* thisInstance) override;
    void initLayoutSetInstall(QObject *thisInstance);

    QString type() const override { return "pdf"; }
    ~PdfSlot();

    // special fns
    void enableSearch(bool x);
    bool processLinks(QPoint clickPos = QPoint(), bool shouldExecute = true);
    QString getSelectedText();
    bool hasTextAt(QPointF pt, int page);
    void syncThumbnailToPage();
    void syncPageToThumbnail(QPointF pagePos);

private:
    // pvt local fns
    void initComboBox();
    void reset();
    void nextResult();
    void prevResult();
    void menuZoom(const QString &text);
    void jumpToResult(int i);
    void searchResultsChanged(const QModelIndex &current, const QModelIndex &previous);
    void enableSidePanel();
    void showIndexTab();
    void showThumbnailTab();
    void populateThumbnailTab();
    QPointF toPdfPoint(QPoint viewportPos, int page);
};

struct ComicSlot : MediaSlot {
    QGraphicsView       *viewer;
    QGraphicsScene      *scene;
    QList<QByteArray>   pageData;
    qreal               zoomFactor = 1.0;
    int                 currentPage = 0;
    int                 totalPages  = 0;

    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
    void initWidgets(QWidget *parent) override;
    void forward() override;
    void backward() override;
    void zoom(qreal x) override;
    void scroll(int x) override;
    //void connectSlots(QObject* thisInstance) override;
    QString type() const override { return "comic"; }

private:
    void showPage(int index);


signals:
    void pageChanged(int newPage);
};

struct PlaintextSlot : MediaSlot {
    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
};


std::unique_ptr<MediaSlot> makeSlot(const QString &path, QWidget *parent, QObject *thisInstance);

#endif // MEDIATYPES_H
