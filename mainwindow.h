#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QSlider>
#include <QGridLayout>
#include <QMenu>
#include <vector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct VideoSlot {
    QMediaPlayer  *player;
    QAudioOutput  *audio;
    QWidget       *wrapper;
    QVideoWidget  *video;
    QSlider       *slider;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void on_actionOpen_triggered();
    void on_actionPlay_triggered(int index);
    void on_actionPause_triggered();
    void on_actionStop_triggered();
    void on_actionReplay_triggered(int index);

private:
    void addVideo(const QString &path);
    void removeVideo(int index);
    void rebuildGrid();
    void updateSelectionVisuals();

    Ui::MainWindow *ui;

    QWidget     *container;
    QGridLayout *grid;
    QWidget* fullscreenWidget = nullptr;
    int fullscreenIndex = -1;

    std::vector<VideoSlot> videoSlots;

    int selectedIndex = -1;
    bool isFullscreen = false;
};

#endif
