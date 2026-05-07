#ifndef MEDIATYPES_H
#define MEDIATYPES_H


#include <memory>

#include <QString>
#include <QSize>
#include <QImage>
#include <QPixmap>
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
class QPdfView;
class QPdfSearchModel;
class QPdfBookmarkModel;
class QPdfPageNavigator;
class QComboBox;
QT_END_NAMESPACE




struct MediaSlot {
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
    virtual QMediaPlayer::PlaybackState getPlayerState() const { return QMediaPlayer::StoppedState; }
    virtual QString type() const = 0;

    QWidget *wrapper = nullptr;
    QWidget *border  = nullptr;
    QWidget *overlay = nullptr;
    QWidget *settings = nullptr;
};

struct VideoSlot : MediaSlot {
    QMediaPlayer *player;
    QAudioOutput *audio;
    QVideoWidget *video;
    QSlider      *slider;
    QComboBox    *audioTracks;
    QComboBox    *videoTracks;
    QComboBox    *subtitleTracks;

    const qint64 seekStep = 5000;
    float currentVolume = 1.0f;

    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
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

    void selectSubtitleStream(int stream);
    void selectVideoStream(int stream);
    void selectAudioStream(int stream);
    QString trackName(const QMediaMetaData &metaData, int index);
    void updateTracks();

    QMediaPlayer::PlaybackState getPlayerState() const override {
        return player->playbackState();
    }


    QString type() const override { return "video"; }

    ~VideoSlot() { stop(); }
};

struct AudioSlot : MediaSlot {
    QMediaPlayer *player;
    QAudioOutput *audio;
    QSlider      *slider;

    QLabel       *cover,
                 *title,
                 *artist;

    QSize        lastSize;
    QImage       coverImage;

    const qint64 seekStep = 5000;
    float currentVolume = 1.0f;


    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
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

    QMediaPlayer::PlaybackState getPlayerState() const override {
        return player->playbackState();
    }


    QString type() const override { return "audio"; }

    ~AudioSlot() { stop(); }
};

struct ImageSlot : MediaSlot {
    QGraphicsPixmapItem     *item;
    QGraphicsView           *viewer;
    QGraphicsScene          *scene;
    QSize                   lastSize;
    QPixmap                 pixmap;

    qreal zoomFactor = 1.0f;

    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
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

    qreal factor;

    void forward() override;
    void backward() override;


    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
    void zoom(qreal x) override;
    void scroll(int x) override;
    void toggleMediaControls(bool x) override;
    void undo();
    void redo();
    void reset();

    QString type() const override { return "pdf"; }
};

// struct PdfSlotMinimal : MediaSlot {
//     QPdfDocument    *doc;
//     QPdfView        *viewer;
//     QPdfPageSelector *pageSelector;
//     QPdfSearchModel *searchModel;
//     QLineEdit *searchField;


//     qreal zoomFactor = 1.0f;

//     void load(const QString &path, QWidget *parent, QObject *thisInstance) override;

//     QString type() const override { return "pdf"; }

//     // void zoom(qreal x) override {
//     //     qreal newZoom = zoomFactor * x;
//     //     if (newZoom > 10.0) return;
//     //     zoomFactor = newZoom;
//     //     viewer->scale(x, x);
//     // }

// };

std::unique_ptr<MediaSlot> makeSlot(const QString &path, QWidget *parent, QObject *thisInstance);

#endif // MEDIATYPES_H
