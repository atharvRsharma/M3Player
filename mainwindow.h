#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QApplication>
#include <QSlider>
#include <QProgressBar>
#include <QAudioOutput>
#include <QVideoWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionOpen_triggered();

    void on_actionStop_triggered();

    void on_actionPlay_triggered();

    void on_actionPause_triggered();


private:
    Ui::MainWindow *ui;
    QMediaPlayer *player;
    QAudioOutput *audio;
    QVideoWidget *video;
    QSlider *slider;
    QProgressBar *bar;
    QStringList fileNames;
};
#endif // MAINWINDOW_H
