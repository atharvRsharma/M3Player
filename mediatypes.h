#ifndef MEDIATYPES_H
#define MEDIATYPES_H




#include <memory>
#include <QSize>
#include <QImage>
#include <QPixmap>

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
class QString;




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


    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
    void play() override;
    void pause() override;
    void stop() override;
    void toggleMute() override;
    void replay() override;
    void toggleMediaControls(bool x) override;

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

    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
    void play() override;
    void pause() override;
    void stop() override;
    void toggleMute() override;

    void replay() override;
    void toggleMediaControls(bool x) override;

    QString type() const override { return "audio"; }

    ~AudioSlot() { stop(); }
};

struct ImageSlot : MediaSlot {
    QSize        lastSize;
    QGraphicsPixmapItem *item;
    QPixmap      pixmap;
    QGraphicsView *viewer;
    QGraphicsScene *scene;

    qreal zoomFactor = 1.0f;

    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
    void zoom(qreal x) override;
    QString type() const override { return "image"; }
};

struct PdfSlot : MediaSlot {
    QPdfDocument    *doc;
    QPdfView        *view;
    QPdfPageSelector *pageSelector;
    QPdfSearchModel *searchModel;
    QLineEdit *searchField;


    qreal zoomFactor = 1.0f;

    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;

    QString type() const override { return "pdf"; }

    // void zoom(qreal x) override {
    //     qreal newZoom = zoomFactor * x;
    //     if (newZoom > 10.0) return;
    //     zoomFactor = newZoom;
    //     viewer->scale(x, x);
    // }

};

std::unique_ptr<MediaSlot> makeSlot(const QString &path, QWidget *parent, QObject *thisInstance);

#endif // MEDIATYPES_H
