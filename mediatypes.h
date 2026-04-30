#ifndef MEDIATYPES_H
#define MEDIATYPES_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QSlider>
#include <QLabel>
#include <memory>



struct MediaSlot {
    virtual ~MediaSlot() = default;
    virtual void load(const QString &path, QWidget *parent, QObject *thisInstance) = 0;
    virtual void play() {}
    virtual void pause() {}
    virtual void stop() {}
    virtual QString type() const = 0;

    QWidget  *wrapper = nullptr;

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


std::unique_ptr<MediaSlot> makeSlot(const QString &path, QWidget *parent, QObject *thisInstance);

#endif // MEDIATYPES_H
