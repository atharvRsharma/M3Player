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
#include <QPdfView>

#include <memory>



struct MediaSlot {
    virtual ~MediaSlot() = default;
    virtual void load(const QString &path, QWidget *parent, QObject *thisInstance) = 0;
    virtual void play() {}
    virtual void pause() {}
    virtual void stop() {}
    virtual void toggleMute() {}
    virtual void replay() {}
    virtual QString type() const = 0;

    QWidget  *wrapper = nullptr;
    QWidget  *border  = nullptr;
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

    QString type() const override { return "video"; }

    ~VideoSlot() { stop(); }
};

struct AudioSlot : MediaSlot {
    QMediaPlayer *player;
    QAudioOutput *audio;
    QSlider      *slider;
    QLabel *cover, *title, *artist;

    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
    void play() override { player->play(); }
    void pause() override { player->pause(); }
    void stop() override { player->stop(); }
    void toggleMute() override {
        if (!audio->isMuted()) audio->setMuted(true);
        else audio->setMuted(false);
    }
    void replay() override { player->setPosition(0); player->play(); }

    QString type() const override { return "audio"; }

    ~AudioSlot() { stop(); }
};

struct ImageSlot : MediaSlot {
    QLabel      *imageLabel;
    QSize        lastSize;
    QPixmap      pixmap;
    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;
    QString type() const override { return "image"; }
};

struct PdfSlot : MediaSlot {
    QPdfDocument    *doc;
    QGraphicsView   *viewer;
    QGraphicsScene  *scene;

    void load(const QString &path, QWidget *parent, QObject *thisInstance) override;

    QString type() const override { return "pdf"; }
};

std::unique_ptr<MediaSlot> makeSlot(const QString &path, QWidget *parent, QObject *thisInstance);

#endif // MEDIATYPES_H
