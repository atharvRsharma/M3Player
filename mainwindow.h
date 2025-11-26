#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QSlider>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QLabel>
#include <QStackedLayout>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dropEvent(QDropEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;


    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void on_actionOpen_triggered();
    void on_actionStop_triggered();
    void on_actionPlay_triggered();
    void on_actionPause_triggered();

    void on_actionNext_triggered();

private:
    Ui::MainWindow *ui;
    QMediaPlayer *player;
    QAudioOutput *audio;
    QVideoWidget *video;

    QLabel *dropZone;
    QSlider *slider;
};
#endif
