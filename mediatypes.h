#ifndef MEDIATYPES_H
#define MEDIATYPES_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QGraphicsWidget>
#include <QGraphicsView>
#include <QSlider>
#include <QLabel>
#include <QPdfDocument>

#include <memory>



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
    void play() override { player->play(); }
    void pause() override { player->pause(); }
    void stop() override { player->stop(); }
    void toggleMute() override {
        if (!audio->isMuted()) audio->setMuted(true);
        else audio->setMuted(false);
    }
    void replay() override { player->setPosition(0); player->play(); }
    void toggleMediaControls(bool x) override {
        slider->setVisible(x);
    }

    QString type() const override { return "video"; }

    ~VideoSlot() { stop(); }
};

struct AudioSlot : MediaSlot {
    QMediaPlayer *player;
    QAudioOutput *audio;
    QSlider      *slider;
    QLabel *cover, *title, *artist;

    QSize        lastSize;
    QImage coverImage;

    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
    void play() override { player->play(); }
    void pause() override { player->pause(); }
    void stop() override { player->stop(); }
    void toggleMute() override {
        if (!audio->isMuted()) audio->setMuted(true);
        else audio->setMuted(false);
    }
    void replay() override { player->setPosition(0); player->play(); }
    void toggleMediaControls(bool x) override {
        slider->setVisible(x);
        artist->setVisible(x);
        title->setVisible(x);
        int sliderHeight = x ? slider->sizeHint().height() : 0;
        int overlayHeight = 60;
        overlay->setGeometry(0, wrapper->height() - overlayHeight - sliderHeight, wrapper->width(), overlayHeight);
    }

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
    void zoom(qreal x) override {
        qreal newZoom = zoomFactor * x;
        if (newZoom > 10.0) return;
        zoomFactor = newZoom;
        viewer->scale(x, x);
    }
    QString type() const override { return "image"; }
};

struct PdfSlot : MediaSlot {
    QPdfDocument    *doc;
    QGraphicsView   *viewer;
    QGraphicsScene  *scene;

    qreal zoomFactor = 1.0f;

    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
    void zoom(qreal x) override {
        qreal newZoom = zoomFactor * x;
        if (newZoom > 10.0) return;
        zoomFactor = newZoom;
        viewer->scale(x, x);
    }
    QString type() const override { return "pdf"; }
};

std::unique_ptr<MediaSlot> makeSlot(const QString &path, QWidget *parent, QObject *thisInstance);

#endif // MEDIATYPES_H
